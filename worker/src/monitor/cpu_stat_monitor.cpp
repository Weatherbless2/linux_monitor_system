#include "monitor/cpu_stat_monitor.h"
#include "monitor/monitor_structs.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include "monitor_info.grpc.pb.h"
#include "monitor_info.pb.h"

namespace monitor {
void CpuStatMonitor::UpdateOnce(monitor::proto::MonitorInfo* monitor_info) {
    int fd = open("/dev/cpu_stat_monitor", O_RDONLY);
    if (fd < 0) return;

    size_t stat_count = 128; // 假设最多128个CPU
    size_t stat_size = sizeof(struct cpu_stat) * stat_count;
    void* addr = mmap(nullptr, stat_size, PROT_READ, MAP_SHARED, fd, 0);
    //代码中使用mmap()直接将内核态设备数据映射到当前进程的虚拟地址空间，
    //仅需一次数据拷贝（内核态数据到映射内存区域），且映射完成后进程可直接访问该内存，无需再进行数据迁移。
    if (addr == MAP_FAILED) {
        close(fd);
        return;
    }

    struct cpu_stat* stats = static_cast<struct cpu_stat*>(addr);
    for (size_t i = 0; i < stat_count; ++i) {
        if (stats[i].cpu_name[0] == '\0') break;
        auto it = cpu_stat_map_.find(stats[i].cpu_name);
        if (it != cpu_stat_map_.end()) {
          struct CpuStat old = it->second;
          auto cpu_stat_msg = monitor_info->add_cpu_stat();
          float new_cpu_total_time = stats[i].user + stats[i].system +
                                     stats[i].idle + stats[i].nice +
                                     stats[i].iowait + stats[i].irq +
                                     stats[i].softirq + stats[i].steal;
          float old_cpu_total_time = old.user + old.system + old.idle + old.nice +
                                     old.io_wait + old.irq + old.soft_irq +
                                     old.steal;
          float new_cpu_busy_time = stats[i].user + stats[i].system +
                                  stats[i].nice + stats[i].irq +
                                  stats[i].softirq + stats[i].steal;
          float old_cpu_busy_time = old.user + old.system + old.nice + old.irq +
                                  old.soft_irq + old.steal;

          float cpu_percent = (new_cpu_busy_time - old_cpu_busy_time) /
                              (new_cpu_total_time - old_cpu_total_time) * 100.00;
          float cpu_user_percent = (stats[i].user - old.user) /
                                   (new_cpu_total_time - old_cpu_total_time) *
                                   100.00;
          float cpu_system_percent = (stats[i].system - old.system) /
                                     (new_cpu_total_time - old_cpu_total_time) *
                                     100.00;
          float cpu_nice_percent = (stats[i].nice - old.nice) /
                                   (new_cpu_total_time - old_cpu_total_time) *
                                   100.00;
          float cpu_idle_percent = (stats[i].idle - old.idle) /
                                   (new_cpu_total_time - old_cpu_total_time) *
                                   100.00;
          float cpu_io_wait_percent = (stats[i].iowait - old.io_wait) /
                                      (new_cpu_total_time - old_cpu_total_time) *
                                      100.00;
          float cpu_irq_percent = (stats[i].irq - old.irq) /
                                  (new_cpu_total_time - old_cpu_total_time) *
                                  100.00;
          float cpu_soft_irq_percent = (stats[i].softirq - old.soft_irq) /
                                       (new_cpu_total_time - old_cpu_total_time) *
                                       100.00;
          cpu_stat_msg->set_cpu_name(stats[i].cpu_name);
          cpu_stat_msg->set_cpu_percent(cpu_percent);
          cpu_stat_msg->set_usr_percent(cpu_user_percent);
          cpu_stat_msg->set_system_percent(cpu_system_percent);
          cpu_stat_msg->set_nice_percent(cpu_nice_percent);
          cpu_stat_msg->set_idle_percent(cpu_idle_percent);
          cpu_stat_msg->set_io_wait_percent(cpu_io_wait_percent);
          cpu_stat_msg->set_irq_percent(cpu_irq_percent);
          cpu_stat_msg->set_soft_irq_percent(cpu_soft_irq_percent);
        }
        // 将内核结构体数据转换为内部 CpuStat 结构体
        CpuStat& cached = cpu_stat_map_[stats[i].cpu_name];
        cached.cpu_name = stats[i].cpu_name;
        cached.user = stats[i].user;
        cached.nice = stats[i].nice;
        cached.system = stats[i].system;
        cached.idle = stats[i].idle;
        cached.io_wait = stats[i].iowait;
        cached.irq = stats[i].irq;
        cached.soft_irq = stats[i].softirq;
        cached.steal = stats[i].steal;
        cached.guest = stats[i].guest;
        cached.guest_nice = stats[i].guest_nice;
    }
//先通过monitor_info指针创建并获取一个新的cpu_stat子消息句柄cpu_stat_msg，
//再分别计算当前stats[i]和历史old对应的 CPU 总运行时间、忙碌时间，
//以及 CPU 整体使用率和用户态、系统态、空闲态等各类细分状态的使用率，
//最后将 CPU 名称和所有计算得到的使用率数据通过cpu_stat_msg的set_xxx()系列方法
//填充到这个新创建的cpu_stat子消息中，完成单条 CPU 状态消息的构造与数据赋值。
    munmap(addr, stat_size);
    close(fd);
    return;
    
}
}  // namespace monitor