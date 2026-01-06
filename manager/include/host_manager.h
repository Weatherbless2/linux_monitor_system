#pragma once

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <mutex>
#include "monitor_info.pb.h"
#include "utils/safe_queue.h" // [新增] 引入队列头文件

namespace monitor {

// 定义 HostScore 结构体 (保留原有逻辑)
struct HostScore {
  monitor::proto::MonitorInfo info;
  double score;
  std::chrono::system_clock::time_point timestamp;
};

class HostManager {
 public:
  // [修改] 构造函数增加 thread_count 参数，默认 4 个线程
  explicit HostManager(size_t thread_count = 4);
  ~HostManager();

  // 启动后台清理线程等
  void Start();
  // 停止所有线程
  void Stop();

  // [生产者接口] gRPC 线程调用此方法，内部实现 Hash 分发
  void OnDataReceived(const monitor::proto::MonitorInfo& host_info);

  // 获取所有主机评分 (供 QueryService 使用)
  std::unordered_map<std::string, HostScore> GetAllHostScores();
  
  // 获取最高分主机
  std::string GetBestHost();

 private:
  // [新增] 消费者线程的工作函数
  // queue_index: 标记当前线程负责处理第几个队列
  void ConsumeMessage(int queue_index);

  // [新增] 真正的业务处理逻辑 (计算评分、入库、告警)
  // 由后台线程调用，不阻塞 gRPC 线程
  void ProcessData(const monitor::proto::MonitorInfo& host_info);

  // 清理过期数据的线程函数
  void ProcessLoop();

  // 辅助函数：计算评分
  double CalcScore(const monitor::proto::MonitorInfo& info);
  
  // 辅助函数：写入 MySQL
  void WriteToMysql(
      const std::string& host_name, const HostScore& host_score,
      double net_in_rate, double net_out_rate, float cpu_percent_rate,
      float usr_percent_rate, float system_percent_rate, float nice_percent_rate,
      float idle_percent_rate, float io_wait_percent_rate, float irq_percent_rate,
      float soft_irq_percent_rate, float steal_percent_rate,
      float guest_percent_rate, float guest_nice_percent_rate,
      float load_avg_1_rate, float load_avg_3_rate, float load_avg_15_rate,
      float mem_used_percent_rate, float mem_total_rate, float mem_free_rate,
      float mem_avail_rate, float net_in_rate_rate, float net_out_rate_rate,
      float net_in_drop_rate_rate, float net_out_drop_rate_rate);

  // 线程数量
  size_t thread_count_;
  
  // 运行状态标志
  std::atomic<bool> running_;

  // [核心新增] N 个队列，N 个线程
  // 使用 shared_ptr 管理 SafeQueue，方便在 vector 中存储和传递
  std::vector<std::shared_ptr<SafeQueue<monitor::proto::MonitorInfo>>> queues_;
  std::vector<std::thread> worker_threads_;

  // 清理线程
  std::unique_ptr<std::thread> thread_;

  // 内存数据存储 (HostScores)
  std::mutex mtx_;
  std::unordered_map<std::string, HostScore> host_scores_;
};

}  // namespace monitor