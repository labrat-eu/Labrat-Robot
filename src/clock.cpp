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
#include <chrono>
#include <cmath>
#include <array>
#include <mutex>

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

class Clock::Node : public UniqueNode {};

class Clock::SenderNode : public Clock::Node {
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

class Clock::SynchronizedNode : public Clock::Node {
public:
  SynchronizedNode() {
    sender_request = addSender<TimesyncMessage>("/synchronized_time/request");
    sender_status = addSender<TimesyncStatus>("/synchronized_time/status");
    
    receiver_response = addReceiver<const TimesyncMessage>("/synchronized_time/response");
    receiver_response->setCallback(&SynchronizedNode::receiverCallbackWrapper, this);

    filter_index = 0;
    first_flag = true;

    thread = LoopThread(&SynchronizedNode::timerFunction, "timesync", 1, this);
  }

private:
  void timerFunction() {
    const TimesyncInternal timesync = {.request = std::chrono::steady_clock::now().time_since_epoch()};
    sender_request->put(timesync);

    std::this_thread::sleep_for(update_interval);
  }

  inline void receiverCallback(const TimesyncInternal &message) {
    const std::chrono::steady_clock::duration now = std::chrono::steady_clock::now().time_since_epoch();

    const std::chrono::steady_clock::duration round_trip_time = now - message.request;
    static const std::chrono::steady_clock::duration max_round_trip_time = std::chrono::milliseconds(20);
    
    if (round_trip_time >= max_round_trip_time) {
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

    static const std::chrono::steady_clock::duration max_timejump = update_interval / 2;
    static const i32 max_drift = 0.1 * 1E6;

    const std::chrono::steady_clock::duration offset_clamped = first_flag ? offset : std::clamp(offset, last_offset - max_timejump, last_offset + max_timejump);
    const i32 drift_clamped = std::clamp(drift, -max_drift, max_drift);

    if (offset_clamped != offset) {
      getLogger().logWarning() << "Timejump detected.";
    }

    if (drift_clamped != drift) {
      getLogger().logWarning() << "High timedrift detected.";
    }

    Clock::synchronize(offset_clamped, drift_clamped, now);

    filter_index = (filter_index + 1) % filter_size;
    first_flag = false;
    last_offset = offset;

    Message<TimesyncStatus> status;
    status.offset = std::chrono::duration_cast<std::chrono::nanoseconds>(offset).count();
    status.drift = (float)drift / 1E6;
    status.round_trip_time = std::chrono::duration_cast<std::chrono::nanoseconds>(round_trip_time).count();

    sender_status->put(status);
  }

  static void receiverCallbackWrapper(const TimesyncInternal &message, Clock::SynchronizedNode *self) {
    self->receiverCallback(message);
  }

  static constexpr std::chrono::steady_clock::duration update_interval = std::chrono::milliseconds(100);
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

class Clock::SteppedNode : public Clock::Node {
public:
  SteppedNode() {
    receiver = addReceiver<const TimestampMessage>("/stepped_time/input");
    receiver->setCallback(&SteppedNode::receiverCallback);
  }

private:
  static void receiverCallback(const Clock::time_point &message) {
    Clock::setTime(message);
  }

  Receiver<const TimestampMessage>::Ptr receiver;
};

Clock::Mode Clock::mode;
bool Clock::is_initialized = false;
std::condition_variable Clock::is_initialized_condition;
std::atomic_flag Clock::exit_flag;

Clock::duration Clock::current_offset;
i32 Clock::current_drift;
std::chrono::steady_clock::duration Clock::last_sync;

std::atomic<Clock::time_point> Clock::current_time;

thread_local Clock::time_point Clock::freeze_time;
thread_local u32 Clock::freeze_count = 0;

std::vector<std::shared_ptr<Clock::Node>> Clock::nodes;

std::priority_queue<Clock::WaiterRegistration> Clock::waiter_queue;
std::mutex Clock::mutex;

void Clock::initialize() {
  if (is_initialized) {
    throw ClockException("Clock is already initialized");
  }

  const std::string &mode_name = lbot::Config::get()->getParameterFallback("/lbot/clock_mode", "system").get<std::string>();

  if (mode_name == "system") {
    mode = Mode::system;
    is_initialized = true;
  } else if (mode_name == "steady") {
    mode = Mode::steady;
    is_initialized = true;
  } else if (mode_name == "synchronized") {
    mode = Mode::synchronized;
  } else if (mode_name == "stepped") {
    mode = Mode::stepped;
  } else {
    throw InvalidArgumentException("Invalid clock mode"); 
  }

  is_initialized_condition.notify_all();
  exit_flag.clear();

  if (mode == Mode::synchronized) {
    nodes.emplace_back(Manager::get()->addNode<SynchronizedNode>("timesync"));
  } else if (mode == Mode::stepped) {
    nodes.emplace_back(Manager::get()->addNode<SteppedNode>("timestep"));
  }

  nodes.emplace_back(Manager::get()->addNode<SenderNode>("time sender"));
}

void Clock::waitUntilInitialized() {
  if (!is_initialized) {
    std::mutex mutex;
    std::unique_lock lock(mutex);

    is_initialized_condition.wait(lock);
  }
}

void Clock::deinitialize() {
  cleanup();

  is_initialized = false;
  is_initialized_condition.notify_all();
}

bool Clock::initialized() {
  return is_initialized;
}
  
Clock::time_point Clock::now() noexcept {
  if (freeze_count != 0) {
    return freeze_time;
  }

  if (!is_initialized) {
    return {};
  }

  switch (mode) {
    case Mode::system: {
      return time_point(std::chrono::duration_cast<duration>(std::chrono::system_clock::now().time_since_epoch()));
    }

    case Mode::steady: {
      return time_point(std::chrono::duration_cast<duration>(std::chrono::steady_clock::now().time_since_epoch()));
    }

    case Mode::synchronized: {
      const duration now = std::chrono::steady_clock::now().time_since_epoch();
      return time_point(std::chrono::duration_cast<duration>(now + current_offset + (now - last_sync) * current_drift / (i64)1E6));
    }

    case Mode::stepped: {
      return time_point(current_time.load());
    }

    default: {
      return {};
    }
  }
}

std::string Clock::format(const time_point time) {
  std::stringstream stream;

  if (is_initialized && mode == Mode::system) {
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

void Clock::synchronize(duration offset, i32 drift, std::chrono::steady_clock::duration now) {
  current_offset = offset;
  current_drift = drift;
  last_sync = now;

  if (!(is_initialized || exit_flag.test(std::memory_order_acquire))) {
    is_initialized = true;
    is_initialized_condition.notify_all();
  }
}

void Clock::setTime(time_point time) {
  if (is_initialized && time < current_time.load(std::memory_order_relaxed)) {
    throw ClockException("Updated time is in the past");
  }

  current_time.store(time, std::memory_order_seq_cst);

  if (!(is_initialized || exit_flag.test(std::memory_order_acquire))) {
    is_initialized = true;
    is_initialized_condition.notify_all();
  }

  {
    std::lock_guard guard(mutex);

    while (!waiter_queue.empty()) {
      WaiterRegistration top = waiter_queue.top();

      if (top.wakeup_time > time) {
        break;
      }

      *top.status = std::cv_status::timeout;
      top.condition->notify_all();

      waiter_queue.pop();
    }
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
    std::lock_guard guard(mutex);

    if (wakeup_time > current_time.load(std::memory_order_acquire) && !exit_flag.test(std::memory_order_acquire)) {
      waiter_queue.emplace(result);
      return result;
    }
  }

  result.waitable = false;

  return result;
}

void Clock::cleanup() {
  nodes.clear();

  exit_flag.test_and_set(std::memory_order_seq_cst);

  if (mode == Mode::stepped) {
    std::lock_guard guard(mutex);

    while (!waiter_queue.empty()) {
      WaiterRegistration top = waiter_queue.top();
      top.condition->notify_all();

      waiter_queue.pop();
    }
  }
}

inline constexpr std::strong_ordering operator<=>(const Clock::WaiterRegistration& lhs, const Clock::WaiterRegistration& rhs) {
  // Reversed so that earlier times show up at the top of the priority queue.
  return rhs.wakeup_time <=> lhs.wakeup_time;
}

}  // namespace lbot
}  // namespace labrat
