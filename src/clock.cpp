/**
 * @file clock.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#include <labrat/lbot/clock.hpp>
#include <labrat/lbot/config.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/utils/thread.hpp>
#include <labrat/lbot/utils/types.hpp>
#include <labrat/lbot/msg/timestamp.hpp>
#include <labrat/lbot/msg/timesync.hpp>
#include <labrat/lbot/msg/timesync_status.hpp>

#include <sstream>
#include <cmath>
#include <array>
#include <mutex>
#include <atomic>
#include <queue>
#include <compare>
#include <string>
#include <vector>
#include <iomanip>

inline namespace labrat {
namespace lbot {

struct TimesyncInternal {
  std::chrono::steady_clock::duration request;
  std::chrono::steady_clock::duration response;
};

class TimestampMessage : public MessageBase<labrat::lbot::Timestamp, Clock::time_point> {
public:
  static void convertFrom(const Converted &source, Storage &destination) {
    const Clock::duration duration = source.time_since_epoch();
    destination.value = std::make_unique<foxglove::Time>(std::chrono::duration_cast<std::chrono::seconds>(duration).count(),
      (std::chrono::duration_cast<std::chrono::nanoseconds>(duration) % std::chrono::seconds(1)).count());
  }

  static void convertTo(const Storage &source, Converted &destination) {
    destination = Clock::time_point(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(source.value->sec()) + std::chrono::nanoseconds(source.value->nsec())));
  }
};

class TimesyncMessage : public MessageBase<labrat::lbot::Timesync, TimesyncInternal> {
public:
  static void convertFrom(const Converted &source, Storage &destination) {
    destination.request = std::make_unique<foxglove::Time>(std::chrono::duration_cast<std::chrono::seconds>(source.request).count(),
      (std::chrono::duration_cast<std::chrono::nanoseconds>(source.request) % std::chrono::seconds(1)).count());
  }

  static void convertTo(const Storage &source, Converted &destination) {
    destination.request = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::seconds(source.request->sec()) + std::chrono::nanoseconds(source.request->nsec()));
    destination.response = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::seconds(source.response->sec()) + std::chrono::nanoseconds(source.response->nsec()));
  }
};

class Clock::Private {
public:
  class Node;
  class SenderNode;
  class SynchronizedNode;
  class SteppedNode;

  static void synchronize(duration offset, i32 drift, std::chrono::steady_clock::duration now);
  static void setTime(time_point time);

  Clock::Mode mode;
  bool is_initialized = false;
  std::condition_variable is_initialized_condition;
  std::atomic_flag exit_flag;

  Clock::duration current_offset;
  i32 current_drift;
  std::atomic<std::chrono::steady_clock::duration> last_sync;

  std::atomic<Clock::time_point> current_time;

  std::vector<std::shared_ptr<Node>> nodes;

  std::priority_queue<Clock::WaiterRegistration> waiter_queue;
  std::mutex mutex;

  std::chrono::steady_clock::duration update_interval;
  std::chrono::steady_clock::duration max_round_trip_time;
  std::chrono::steady_clock::duration max_timejump;
  i32 max_drift;
};

static Clock::Private priv;

class Clock::Private::Node : public UniqueNode {};

class Clock::Private::SenderNode : public Clock::Private::Node {
public:
  SenderNode() {
    sender = addSender<TimestampMessage>("/time");
    thread = TimerThread(&SenderNode::timerFunction, std::chrono::milliseconds(10), "clock", 1, this);
  }

private:
  void timerFunction() {
    sender->put(Clock::now());
  }

  Sender<TimestampMessage>::Ptr sender;
  TimerThread thread;
};

class Clock::Private::SynchronizedNode : public Clock::Private::Node {
public:
  SynchronizedNode() {
    lbot::Config::Ptr config = lbot::Config::get();

    sender_request = addSender<TimesyncMessage>("/synchronized_time/request");
    sender_status = addSender<TimesyncStatus>("/synchronized_time/status");
    
    receiver_response = addReceiver<const TimesyncMessage>("/synchronized_time/response");
    receiver_response->setCallback(&SynchronizedNode::receiverCallbackWrapper, this);

    priv.update_interval = std::chrono::milliseconds(config->getParameterFallback("/lbot/synchronized_time/update_interval", 100).get<int>());
    priv.max_round_trip_time = std::chrono::milliseconds(config->getParameterFallback("/lbot/synchronized_time/max_round_trip_time", 20).get<int>());
    priv.max_timejump = priv.update_interval / 2;
    priv.max_drift = config->getParameterFallback("/lbot/synchronized_time/max_drift", 0.1).get<double>() * 1E6;

    filter_index = 0;
    first_flag = true;

    thread = LoopThread(&SynchronizedNode::timerFunction, "timesync", 1, this);
  }

private:
  void timerFunction() {
    const TimesyncInternal timesync = {.request = std::chrono::steady_clock::now().time_since_epoch()};
    sender_request->put(timesync);

    std::this_thread::sleep_for(priv.update_interval);
  }

  inline void receiverCallback(const TimesyncInternal &message) {
    const std::chrono::steady_clock::duration now = std::chrono::steady_clock::now().time_since_epoch();

    const std::chrono::steady_clock::duration round_trip_time = now - message.request;
    
    if (round_trip_time >= priv.max_round_trip_time) {
      getLogger().logWarning() << "High round trip time detected. Skipping timesync.";
      return;
    }

    const bool filter_valid = filter_timestamps[filter_index] != std::chrono::steady_clock::duration();
    const std::chrono::steady_clock::duration offset = message.response - message.request - (round_trip_time / 2);

    std::lock_guard guard(mutex);

    const std::chrono::steady_clock::duration filter_offset_total = offset - filter_offsets[filter_index];
    filter_offsets[filter_index] = offset;

    const std::chrono::steady_clock::duration total_filter_duration = now - filter_timestamps[filter_index];
    filter_timestamps[filter_index] = now;

    const i32 drift = filter_valid ? (filter_offset_total.count() * (i64)1E6) / total_filter_duration.count() : 0;

    const std::chrono::steady_clock::duration offset_clamped = first_flag ? offset : std::clamp(offset, last_offset - priv.max_timejump, last_offset + priv.max_timejump);
    const i32 drift_clamped = std::clamp(drift, -priv.max_drift, priv.max_drift);

    if (offset_clamped != offset) {
      getLogger().logWarning() << "Timejump detected.";
    }

    if (drift_clamped != drift) {
      getLogger().logWarning() << "High timedrift detected.";
    }

    priv.synchronize(offset_clamped, drift_clamped, now);

    filter_index = (filter_index + 1) % filter_size;
    first_flag = false;
    last_offset = offset;

    Message<TimesyncStatus> status;
    status.offset = std::chrono::duration_cast<std::chrono::nanoseconds>(offset).count();
    status.drift = (float)drift / 1E6;
    status.round_trip_time = std::chrono::duration_cast<std::chrono::nanoseconds>(round_trip_time).count();

    sender_status->put(status);
  }

  static void receiverCallbackWrapper(const TimesyncInternal &message, Clock::Private::SynchronizedNode *self) {
    self->receiverCallback(message);
  }

  static const i32 filter_size = 8;

  std::array<std::chrono::steady_clock::duration, filter_size> filter_timestamps;
  std::array<std::chrono::steady_clock::duration, filter_size> filter_offsets;
  i32 filter_index;

  std::chrono::steady_clock::duration last_offset;
  bool first_flag;

  std::mutex mutex;

  Sender<TimesyncMessage>::Ptr sender_request;
  Sender<TimesyncStatus>::Ptr sender_status;
  Receiver<const TimesyncMessage>::Ptr receiver_response;

  LoopThread thread;
};

class Clock::Private::SteppedNode : public Clock::Private::Node {
public:
  SteppedNode() {
    receiver = addReceiver<const TimestampMessage>("/stepped_time/input");
    receiver->setCallback(&SteppedNode::receiverCallback);
  }

private:
  static void receiverCallback(const Clock::time_point &message) {
    priv.setTime(message);
  }

  Receiver<const TimestampMessage>::Ptr receiver;
};

void Clock::initialize() {
  if (priv.is_initialized) {
    throw ClockException("Clock is already initialized");
  }

  const std::string mode_name = lbot::Config::get()->getParameterFallback("/lbot/clock_mode", "system").get<std::string>();

  if (mode_name == "system") {
    priv.mode = Mode::system;
    priv.is_initialized = true;
  } else if (mode_name == "steady") {
    priv.mode = Mode::steady;
    priv.is_initialized = true;
  } else if (mode_name == "synchronized") {
    priv.mode = Mode::synchronized;
  } else if (mode_name == "stepped") {
    priv.mode = Mode::stepped;
  } else {
    throw InvalidArgumentException("Invalid clock mode"); 
  }

  priv.is_initialized_condition.notify_all();
  priv.exit_flag.clear();

  if (priv.mode == Mode::synchronized) {
    priv.nodes.emplace_back(Manager::get()->addNode<Private::SynchronizedNode>("timesync"));
  } else if (priv.mode == Mode::stepped) {
    priv.nodes.emplace_back(Manager::get()->addNode<Private::SteppedNode>("timestep"));
  }

  priv.nodes.emplace_back(Manager::get()->addNode<Private::SenderNode>("time sender"));
}

void Clock::waitUntilInitialized() {
  if (!priv.is_initialized) {
    std::mutex mutex;
    std::unique_lock lock(mutex);

    priv.is_initialized_condition.wait(lock);
  }
}

void Clock::deinitialize() {
  cleanup();

  priv.is_initialized = false;
  priv.is_initialized_condition.notify_all();
}

bool Clock::initialized() {
  return priv.is_initialized;
}
  
Clock::time_point Clock::now() noexcept {
  if (!priv.is_initialized) {
    return {};
  }

  switch (priv.mode) {
    case Mode::system: {
      return time_point(std::chrono::duration_cast<duration>(std::chrono::system_clock::now().time_since_epoch()));
    }

    case Mode::steady: {
      return time_point(std::chrono::duration_cast<duration>(std::chrono::steady_clock::now().time_since_epoch()));
    }

    case Mode::synchronized: {
      const SynchronizationParameters parameters = getSynchronizationParameters();
      const duration now = std::chrono::steady_clock::now().time_since_epoch();
      const time_point new_estimate(std::chrono::duration_cast<duration>(now + parameters.offset + (now - parameters.last_sync) * parameters.drift / (i64)1E6));

      static thread_local Clock::time_point last_synchronized_estimate;

      // Ensure that time is never going backwards. 
      if (new_estimate > last_synchronized_estimate) {
        last_synchronized_estimate = new_estimate;
      }

      return last_synchronized_estimate;
    }

    case Mode::stepped: {
      return time_point(priv.current_time.load());
    }

    default: {
      return {};
    }
  }
}

std::string Clock::format(const time_point time) {
  std::stringstream stream;

  if (priv.is_initialized && priv.mode == Mode::system) {
    const std::chrono::system_clock::time_point time_local = std::chrono::time_point<std::chrono::system_clock>(std::chrono::duration_cast<std::chrono::system_clock::duration>(time.time_since_epoch()));
    const std::time_t c_time = std::chrono::system_clock::to_time_t(time_local);

    stream << std::put_time(std::localtime(&c_time), "%T");
  } else {
    const Clock::duration duration = time.time_since_epoch();

    stream.setf(std::ios_base::showbase);
    stream.fill('0');
    stream << std::setw(2) << std::chrono::duration_cast<std::chrono::hours>(duration).count() % 24;
    stream << ":";
    stream << std::setw(2) << std::chrono::duration_cast<std::chrono::minutes>(duration).count() % 60;
    stream << ":";
    stream << std::setw(2) << std::chrono::duration_cast<std::chrono::seconds>(duration).count() % 60;
  }

  return stream.str();
}

Clock::Mode Clock::getMode() {
  return priv.mode;
}

Clock::SynchronizationParameters Clock::getSynchronizationParameters() {
  SynchronizationParameters result;
  std::chrono::steady_clock::duration last_sync_before = priv.last_sync.load(std::memory_order_acquire);
  
  while (true) {
    result.offset = priv.current_offset;
    result.drift = priv.current_drift;

    const std::chrono::steady_clock::duration last_sync_after = priv.last_sync.load(std::memory_order_seq_cst);

    if (last_sync_before == last_sync_after) {
      result.last_sync = last_sync_after;
      return result;
    }

    last_sync_before = last_sync_after;
  }
}

Clock::WaiterRegistration Clock::registerWaiter(const time_point wakeup_time, std::shared_ptr<std::condition_variable> condition) {
  WaiterRegistration result = {
    .wakeup_time = wakeup_time,
    .condition = condition,
    .status = std::make_shared<std::cv_status>(std::cv_status::no_timeout),
    .waitable = true
  };

  {
    std::lock_guard guard(priv.mutex);

    if (wakeup_time > priv.current_time.load(std::memory_order_acquire) && !priv.exit_flag.test(std::memory_order_acquire)) {
      priv.waiter_queue.emplace(result);
      return result;
    }
  }

  result.waitable = false;

  return result;
}

void Clock::cleanup() {
  priv.nodes.clear();

  priv.exit_flag.test_and_set(std::memory_order_seq_cst);

  if (priv.mode == Mode::stepped) {
    std::lock_guard guard(priv.mutex);

    while (!priv.waiter_queue.empty()) {
      WaiterRegistration top = priv.waiter_queue.top();
      top.condition->notify_all();

      priv.waiter_queue.pop();
    }
  }
}

void Clock::Private::synchronize(duration offset, i32 drift, std::chrono::steady_clock::duration now) {
  priv.current_offset = offset;
  priv.current_drift = drift;
  priv.last_sync.store(now, std::memory_order_release);

  if (!(priv.is_initialized || priv.exit_flag.test(std::memory_order_acquire))) {
    priv.is_initialized = true;
    priv.is_initialized_condition.notify_all();
  }
}


void Clock::Private::setTime(time_point time) {
  if (priv.is_initialized && time < priv.current_time.load(std::memory_order_relaxed)) {
    throw ClockException("Updated time is in the past");
  }

  priv.current_time.store(time, std::memory_order_seq_cst);

  if (!(priv.is_initialized || priv.exit_flag.test(std::memory_order_acquire))) {
    priv.is_initialized = true;
    priv.is_initialized_condition.notify_all();
  }

  {
    std::lock_guard guard(priv.mutex);

    while (!priv.waiter_queue.empty()) {
      WaiterRegistration top = priv.waiter_queue.top();

      if (top.wakeup_time > time) {
        break;
      }

      *top.status = std::cv_status::timeout;
      top.condition->notify_all();

      priv.waiter_queue.pop();
    }
  }
}


inline constexpr std::strong_ordering operator<=>(const Clock::WaiterRegistration& lhs, const Clock::WaiterRegistration& rhs) {
  // Reversed so that earlier times show up at the top of the priority queue.
  return rhs.wakeup_time <=> lhs.wakeup_time;
}

}  // namespace lbot
}  // namespace labrat
