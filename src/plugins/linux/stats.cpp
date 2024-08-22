/**
 * @file time.cpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#include <labrat/lbot/message.hpp>
#include <labrat/lbot/exception.hpp>
#include <labrat/lbot/node.hpp>
#include <labrat/lbot/plugins/linux/msg/cpu.hpp>
#include <labrat/lbot/plugins/linux/msg/disks.hpp>
#include <labrat/lbot/plugins/linux/msg/memory.hpp>
#include <labrat/lbot/plugins/linux/msg/process.hpp>
#include <labrat/lbot/plugins/linux/stats.hpp>
#include <labrat/lbot/utils/thread.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <forward_list>
#include <charconv>
#include <filesystem>
#include <cmath> 

#include <linux/version.h>
#include <sys/statvfs.h>

inline namespace labrat {
namespace lbot::plugins {

struct CpuRawCounters {
  uint64_t user_time;
  uint64_t nice_time;
  uint64_t system_time;
  uint64_t idle_time;
  uint64_t io_wait;
  uint64_t irq;
  uint64_t soft_irq;
  uint64_t steal;
  uint64_t guest;
  uint64_t guest_nice;

  CpuRawCounters operator-(const CpuRawCounters &rhs) const {
    return CpuRawCounters{
      user_time - rhs.user_time,
      nice_time - rhs.nice_time,
      system_time - rhs.system_time,
      idle_time - rhs.idle_time,
      io_wait - rhs.io_wait,
      irq - rhs.irq,
      soft_irq - rhs.soft_irq,
      steal - rhs.steal,
      guest - rhs.guest,
      guest_nice - rhs.guest_nice,
    };
  }
};

struct ProcessRawCounters {
  uint64_t user_time;
  uint64_t system_time;

  ProcessRawCounters operator-(const ProcessRawCounters &rhs) const {
    return ProcessRawCounters{
      user_time - rhs.user_time,
      system_time - rhs.system_time,
    };
  }
};

inline ProcessState charToProcessState(char c) {
  switch (c) {
    case 'R': {
      return ProcessState::Running;
    }
    case 'S': {
      return ProcessState::Sleeping;
    }
    case 'D': {
      return ProcessState::Waiting;
    }
    case 'Z': {
      return ProcessState::Zombie;
    }
    case 'T': {
      return ProcessState::Stopped;
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33)
    case 't': {
      return ProcessState::Tracing;
    }
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    case 'W': {
      return ProcessState::Paging;
    }
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    case 'X': {
      return ProcessState::Dead;
    }
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,13,0) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,33)
    case 'x': {
      return ProcessState::Dead;
    }
    case 'K': {
      return ProcessState::Wakekill;
    }
    case 'W': {
      return ProcessState::Waking;
    }
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,13,0) && LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0)
    case 'P': {
      return ProcessState::Parked;
    }
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
    case 'I': {
      return ProcessState::Idle;
    }
#endif
    default: {
      return ProcessState::Unknown;
    }
  }
}

class LinuxStatsNode : public UniqueNode {
public:
  LinuxStatsNode();
  ~LinuxStatsNode();

private:
  void loopFast();
  void loopSlow();

  void readCpu();
  void readProcess();
  void readMemory();
  void readDisks();
  
  Sender<Cpu>::Ptr sender_cpu;
  Sender<Process>::Ptr sender_process;
  Sender<Memory>::Ptr sender_memory;
  Sender<Disks>::Ptr sender_disks;

  std::unordered_map<int, CpuRawCounters> cpu_counter_map;
  float total_cpu_time;

  std::unordered_map<int, ProcessRawCounters> process_counter_map;

  TimerThread thread_fast;
  TimerThread thread_slow;
};

LinuxStats::LinuxStats() {
  addNode<LinuxStatsNode>("linux-time");
}

LinuxStats::~LinuxStats() = default;

LinuxStatsNode::LinuxStatsNode() {
  sender_cpu = addSender<Cpu>("/linux/cpu");
  sender_process = addSender<Process>("/linux/process");
  sender_memory = addSender<Memory>("/linux/memory");
  sender_disks = addSender<Disks>("/linux/disks");

  total_cpu_time = std::numeric_limits<float>::quiet_NaN();

  thread_fast = TimerThread(&LinuxStatsNode::loopFast, std::chrono::seconds(2), "linux fast", 1, this);
  thread_slow = TimerThread(&LinuxStatsNode::loopSlow, std::chrono::seconds(30), "linux slow", 1, this);
}

LinuxStatsNode::~LinuxStatsNode() = default;

void LinuxStatsNode::loopFast() {
  readCpu();
  readProcess();
  readMemory();
}

void LinuxStatsNode::loopSlow() {
  readDisks();
}

void LinuxStatsNode::readCpu() {
  Message<Cpu> message;

  if (!cpu_counter_map.empty()) {
    message.cores.reserve(cpu_counter_map.size() - 1);
  }

  std::ifstream info("/proc/stat");

  if (!info) {
    throw SystemException("Failed to open /proc/stat");
  }

  std::array<char, 1024> buffer;
  while (info.getline(buffer.data(), buffer.size())) {
    if (std::strncmp("cpu", buffer.data(), sizeof("cpu") - 1)) {
      continue;
    }

    int id = -1;
    CpuRawCounters counters;
    
    if (buffer.data()[3] == ' ') {
      if (!std::sscanf(buffer.data() + 4, "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &counters.user_time, &counters.nice_time, &counters.system_time, &counters.idle_time, &counters.io_wait, &counters.irq, &counters.soft_irq, &counters.steal, &counters.guest, &counters.guest_nice)) {
        throw SystemException("Failed to parse CPU stats", errno);
      }
    } else {
      if (!std::sscanf(buffer.data() + 3, "%d %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu", &id, &counters.user_time, &counters.nice_time, &counters.system_time, &counters.idle_time, &counters.io_wait, &counters.irq, &counters.soft_irq, &counters.steal, &counters.guest, &counters.guest_nice)) {
        throw SystemException("Failed to parse core stats", errno);
      }
    }

    std::unordered_map<int, CpuRawCounters>::iterator iter = cpu_counter_map.find(id);
    if (iter == cpu_counter_map.end()) {
      cpu_counter_map.emplace(id, counters);
      continue;
    }

    const CpuRawCounters diff = counters - iter->second;
    iter->second = counters;

    const float total_time = diff.user_time + diff.nice_time + diff.system_time + diff.irq + diff.soft_irq + diff.idle_time + diff.io_wait + diff.steal;
    const float user_load = (float)diff.user_time / total_time;
    const float kernel_load = (float)diff.system_time / total_time;
    const float load = user_load + kernel_load;

    if (id == -1) {
      message.load = load;
      message.user_load = user_load;
      message.kernel_load = kernel_load;

      total_cpu_time = total_time;
    } else {
      std::unique_ptr<CoreNative> core = std::make_unique<CoreNative>();
      core->id = id;
      core->load = load;
      core->user_load = user_load;
      core->kernel_load = kernel_load;

      message.cores.emplace_back(std::move(core));
    }
  }

  sender_cpu->put(std::move(message));
}

void LinuxStatsNode::readProcess() {
  const int cpu_count = cpu_counter_map.size() - 1;
  if (cpu_count < 1 || std::isnan(total_cpu_time)) {
    return;
  }

  const float total_time_per_cpu = total_cpu_time / cpu_count; 

  Message<Process> message;

  if (!process_counter_map.empty()) {
    message.threads.reserve(process_counter_map.size());
  }

  for (const std::filesystem::directory_entry &task : std::filesystem::directory_iterator("/proc/self/task/")) {
    std::ifstream info(task.path() / std::filesystem::path("stat"));

    if (!info) {
      continue;
    }

    std::array<char, 1024> buffer;
    if (!info.getline(buffer.data(), buffer.size())) {
      continue;
    }

    std::unique_ptr<ThreadNative> thread = std::make_unique<ThreadNative>();
    std::array<char, 32> name;
    char state;
    int parent_id;
    int group_id;
    int session_id;
    int tty;
    int foreground_group_id;
    unsigned int flags;
    long minor_faults;
    long children_minor_faults;
    long major_faults;
    long children_major_faults;
    ProcessRawCounters counters;
    ProcessRawCounters children_counters;
    long priority;
    long nice;

    if (!std::sscanf(buffer.data(), "%d", &thread->id)) {
      throw SystemException("Failed to parse thread id", errno);
    }

    char *const name_start = std::strchr(buffer.data(), '(') + 1;
    char *const name_end = std::strrchr(buffer.data(), ')');
    if (name_start == nullptr || name_end == nullptr) {
      throw SystemException("Failed to parse thread name");
    }

    *name_end = '\0';
    strncpy(name.data(), name_start, name.size() - 1);
    name.back() = '\0';

    if (!std::sscanf(name_end + 2, "%c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld", &state, &parent_id, &group_id, &session_id, &tty, &foreground_group_id, &flags, &minor_faults, &children_minor_faults, &major_faults, &children_major_faults, &counters.user_time, &counters.system_time, &children_counters.user_time, &children_counters.system_time, &priority, &nice)) {
      throw SystemException("Failed to parse thread stats", errno);
    }

    std::unordered_map<int, ProcessRawCounters>::iterator iter = process_counter_map.find(thread->id);
    if (iter == process_counter_map.end()) {
      process_counter_map.emplace(thread->id, counters);
      continue;
    }

    const ProcessRawCounters diff = counters - iter->second;
    iter->second = counters;

    thread->name = std::string(name.data());
    thread->state = charToProcessState(state);
    thread->user_load = (float)diff.user_time / total_time_per_cpu;
    thread->kernel_load = (float)diff.system_time / total_time_per_cpu;
    thread->load = thread->user_load + thread->kernel_load;

    message.threads.emplace_back(std::move(thread));
  }

  sender_process->put(std::move(message));
}

void LinuxStatsNode::readMemory() {
  std::ifstream info("/proc/meminfo");

  if (!info) {
    throw SystemException("Failed to open /proc/meminfo");
  }

  int64_t total = -1;
  int64_t availale = -1;
  int64_t swap_total = -1;
  int64_t swap_free = -1;

  std::string line;
  while (std::getline(info, line)) {
    std::unique_ptr<DiskNative> disk = std::make_unique<DiskNative>();

    std::istringstream linestream(line);

    std::string name;
    uint64_t value;
    std::string suffix;

    linestream >> name >> value >> suffix;

    if (!(suffix.empty() || suffix == "kB")) {
      throw SystemException("Unsupported suffix");
    }

    if (name == "MemTotal:") {
      total = value;
    } else if (name == "MemAvailable:") {
      availale = value;
    } else if (name == "SwapTotal:") {
      swap_total = value;
    } else if (name == "SwapFree:") {
      swap_free = value;
    }
  }

  if (total == -1 || availale == -1 || swap_total == -1 || swap_free == -1) {
    throw SystemException("Failed to read memory usage");
  }

  Message<Memory> message;
  message.total = total * 1000;
  message.usage = 1.0f - (float)availale / (float)total;
  message.swap_total = swap_total * 1000;
  message.swap_usage = 1.0f - (float)swap_free / (float)swap_total;

  sender_memory->put(std::move(message));
}

void LinuxStatsNode::readDisks() {
  Message<Disks> message;

  std::ifstream info("/proc/mounts");

  if (!info) {
    throw SystemException("Failed to open /proc/mounts");
  }

  std::string line;
  while (std::getline(info, line)) {
    std::unique_ptr<DiskNative> disk = std::make_unique<DiskNative>();

    std::istringstream linestream(line);

    std::string mount_point;
    std::string attributes;
    
    linestream >> disk->name >> mount_point >> disk->file_system >> attributes;

    static const std::unordered_set<std::string> supported_file_systems = {
      "ext2", "ext3", "ext4", "vfat", "ntfs", "zfs", "hfs", "reiserfs", "reiser4", "fuseblk", "exfat", "f2fs", "hfs+", "jfs", "btrfs", "bcachefs", "minix", "nilfs", "xfs", "apfs"
    };

    if (!supported_file_systems.contains(disk->file_system)) {
      continue;
    }

    struct statvfs stats;

    if (statvfs(mount_point.c_str(), &stats)) {
      throw SystemException("Failed to get statvfs", errno);
    }

    disk->size = stats.f_blocks * stats.f_frsize;
    disk->usage = 1.0f - ((float)stats.f_bavail / (float)stats.f_blocks);

    message.disks.emplace_back(std::move(disk));
  }

  sender_disks->put(std::move(message));
}

}  // namespace lbot::plugins
}  // namespace labrat
