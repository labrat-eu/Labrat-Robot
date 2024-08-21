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
#include <labrat/lbot/plugins/linux/stats.hpp>
#include <labrat/lbot/utils/thread.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <forward_list>
#include <charconv>

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

class LinuxStatsNode : public UniqueNode {
public:
  LinuxStatsNode();
  ~LinuxStatsNode();

private:
  void loopFast();
  void loopSlow();

  void readCpu();
  void readMemory();
  void readDisks();
  
  Sender<Cpu>::Ptr sender_cpu;
  Sender<Memory>::Ptr sender_memory;
  Sender<Disks>::Ptr sender_disks;

  std::unordered_map<std::string, CpuRawCounters> cpu_counter_map;

  TimerThread thread_fast;
  TimerThread thread_slow;
};

LinuxStats::LinuxStats() {
  addNode<LinuxStatsNode>("linux-time");
}

LinuxStats::~LinuxStats() = default;

LinuxStatsNode::LinuxStatsNode() {
  sender_cpu = addSender<Cpu>("/linux/cpu");
  sender_memory = addSender<Memory>("/linux/memory");
  sender_disks = addSender<Disks>("/linux/disks");

  thread_fast = TimerThread(&LinuxStatsNode::loopFast, std::chrono::seconds(2), "linux fast", 1, this);
  thread_slow = TimerThread(&LinuxStatsNode::loopSlow, std::chrono::seconds(30), "linux slow", 1, this);
}

LinuxStatsNode::~LinuxStatsNode() = default;

void LinuxStatsNode::loopFast() {
  readCpu();
  readMemory();
}

void LinuxStatsNode::loopSlow() {
  readDisks();
}

void LinuxStatsNode::readCpu() {
  Message<Cpu> message;

  std::ifstream info("/proc/stat");

  if (!info) {
    throw SystemException("Failed to open /proc/stat");
  }

  std::string line;
  while (std::getline(info, line)) {
    if (!line.starts_with("cpu")) {
      continue;
    }

    std::istringstream linestream(line);

    std::string name;
    CpuRawCounters counters;
    
    linestream >> name >> counters.user_time >> counters.nice_time >> counters.system_time >> counters.idle_time >> counters.io_wait >> counters.irq >> counters.soft_irq >> counters.steal >> counters.guest >> counters.guest_nice;

    std::unordered_map<std::string, CpuRawCounters>::iterator iter = cpu_counter_map.find(name);
    if (iter == cpu_counter_map.end()) {
      cpu_counter_map.emplace(name, counters);
      continue;
    }

    const CpuRawCounters diff = counters - iter->second;
    iter->second = counters;

    const float total_time = diff.user_time + diff.nice_time + diff.system_time + diff.irq + diff.soft_irq + diff.idle_time + diff.io_wait + diff.steal;
    const float user_load = (float)diff.user_time / total_time;
    const float kernel_load = (float)diff.system_time / total_time;
    const float load = user_load + kernel_load;

    std::string_view id_view(name.begin() + 3, name.end());

    if (id_view.empty()) {
      message.load = load;
      message.user_load = user_load;
      message.kernel_load = kernel_load;
    } else {
      std::unique_ptr<CoreNative> core = std::make_unique<CoreNative>();

      std::from_chars_result result = std::from_chars(id_view.data(), id_view.data() + id_view.length(), core->id);
      if (result.ec != std::errc()) {
        throw SystemException("Failed to parse CPU id");
      }

      core->load = load;
      core->user_load = user_load;
      core->kernel_load = kernel_load;

      message.cores.emplace_back(std::move(core));
    }
  }

  sender_cpu->put(std::move(message));
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
    std::string dummy1;
    std::string dummy2;

    linestream >> disk->name >> mount_point >> disk->file_system >> attributes >> dummy1 >> dummy2;

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
