# 管理者服务器 API 文档

## 概述

管理者服务器（manager）提供两类 gRPC 服务：
1. **GrpcManager** - 接收工作者推送的监控数据
2. **QueryService** - 提供性能数据查询接口

服务地址：`0.0.0.0:50051`（默认）

---

## 架构说明

```
┌─────────────────┐     Push (10s)      ┌─────────────────────────────────────┐
│  Worker Server  │ ──────────────────► │         Manager Server              │
│    (worker)     │   SetMonitorInfo    │          (manager)                  │
└─────────────────┘                     │                                     │
                                        │  ┌─────────────┐  ┌──────────────┐  │
┌─────────────────┐     Push (10s)      │  │ GrpcManager │  │ QueryService │  │
│  Worker Server  │ ──────────────────► │  │  (接收数据)  │  │  (查询接口)   │  │
│    (worker)     │   SetMonitorInfo    │  └──────┬──────┘  └──────┬───────┘  │
└─────────────────┘                     │         │                │          │
                                        │         ▼                ▼          │
        ...                             │     ┌───────────────────────┐       │
                                        │     │      MySQL 数据库      │       │
┌─────────────────┐                     │     │   (5张性能数据表)       │       │
│  Query Client   │ ◄─────────────────► │     └───────────────────────┘       │
│  (grpcurl/app)  │   QueryService      └─────────────────────────────────────┘
└─────────────────┘
```

---

## 一、数据接收服务 (GrpcManager)

### 1.1 SetMonitorInfo - 接收监控数据

工作者服务器每10秒调用此接口推送监控数据。

**服务定义**
```protobuf
service GrpcManager {
  rpc SetMonitorInfo(MonitorInfo) returns (google.protobuf.Empty);
}
```

**请求消息**
```protobuf
message MonitorInfo {
    string name = 1;              // 主机名（兼容字段）
    HostInfo host_info = 2;       // 主机标识信息
    repeated SoftIrq soft_irq = 4;
    CpuLoad cpu_load = 5;
    repeated CpuStat cpu_stat = 6;
    MemInfo mem_info = 7;
    repeated NetInfo net_info = 8;
    repeated DiskInfo disk_info = 9;
}

message HostInfo {
    string hostname = 1;          // 主机名
    string ip_address = 2;        // IP 地址（主网卡 IP）
}
```

**子消息定义**

| 消息类型 | 说明 | 主要字段 |
|---------|------|---------|
| SoftIrq | 软中断统计 | cpu, hi, timer, net_tx, net_rx, block, sched |
| CpuLoad | CPU负载 | load_avg_1, load_avg_3, load_avg_15 |
| CpuStat | CPU使用率 | cpu_name, cpu_percent, usr_percent, system_percent, io_wait_percent |
| MemInfo | 内存信息 | total, free, avail, buffers, cached, used_percent |
| NetInfo | 网络信息 | name, send_rate, rcv_rate, err_in, err_out, drop_in, drop_out |
| DiskInfo | 磁盘信息 | name, read_bytes_per_sec, write_bytes_per_sec, util_percent |

**响应**: `google.protobuf.Empty`

**错误码**
| 状态码 | 说明 |
|-------|------|
| OK | 成功接收 |
| INVALID_ARGUMENT | 请求为空或缺少主机名 |

---

### 1.2 GetMonitorInfo - 获取监控数据（保留接口）

获取第一个主机的最新监控数据（主要用于测试）。

**请求**: `google.protobuf.Empty`

**响应**: `MonitorInfo`

---

## 二、查询服务 (QueryService)

QueryService 提供9个查询接口，支持时间范围查询、分页、聚合等功能。

### 公共类型定义

```protobuf
// 时间范围
message TimeRange {
    google.protobuf.Timestamp start_time = 1;
    google.protobuf.Timestamp end_time = 2;
}

// 分页参数
message Pagination {
    int32 page = 1;      // 页码，从1开始
    int32 page_size = 2; // 每页大小，默认100
}

// 排序方式
enum SortOrder {
    DESC = 0;  // 降序（默认）
    ASC = 1;   // 升序
}

// 服务器状态
enum ServerStatus {
    ONLINE = 0;   // 60秒内有数据上报
    OFFLINE = 1;  // 超过60秒无数据
}
```

---

### 2.1 QueryPerformance - 时间段性能数据查询

查询指定服务器在指定时间段内的所有性能数据。

**请求**
```protobuf
message QueryPerformanceRequest {
    string server_name = 1;       // 服务器名称（格式：hostname_ip）
    TimeRange time_range = 2;     // 时间范围
    Pagination pagination = 3;    // 分页参数
}
```

**响应**
```protobuf
message QueryPerformanceResponse {
    repeated PerformanceRecord records = 1;
    int32 total_count = 2;        // 总记录数
    int32 page = 3;               // 当前页码
    int32 page_size = 4;          // 每页大小
}
```

**PerformanceRecord 字段说明**

| 字段 | 类型 | 说明 |
|-----|------|------|
| server_name | string | 服务器标识 |
| timestamp | Timestamp | 采集时间 |
| cpu_percent | float | CPU总使用率 (%) |
| usr_percent | float | 用户态CPU (%) |
| system_percent | float | 内核态CPU (%) |
| io_wait_percent | float | IO等待 (%) |
| load_avg_1/3/15 | float | 1/3/15分钟负载 |
| mem_used_percent | float | 内存使用率 (%) |
| mem_total/free/avail | float | 总/空闲/可用内存 (GB) |
| disk_util_percent | float | 磁盘利用率 (%) |
| send_rate/rcv_rate | float | 网络发送/接收速率 (kB/s) |
| score | float | 综合评分 (0-100) |
| *_rate | float | 各指标变化率 |

**示例**
```bash
grpcurl -plaintext -d '{
  "server_name": "server1_192.168.1.100",
  "time_range": {
    "start_time": "2024-01-01T00:00:00Z",
    "end_time": "2024-01-01T01:00:00Z"
  },
  "pagination": {"page": 1, "page_size": 50}
}' localhost:50051 monitor.proto.QueryService/QueryPerformance
```

---

### 2.2 QueryTrend - 变化率趋势查询

查询性能指标的变化率趋势，支持按时间间隔聚合。

**请求**
```protobuf
message QueryTrendRequest {
    string server_name = 1;
    TimeRange time_range = 2;
    int32 interval_seconds = 3;   // 聚合间隔（秒），0表示不聚合
}
```

**响应**
```protobuf
message QueryTrendResponse {
    repeated PerformanceRecord records = 1;
    int32 interval_seconds = 2;
}
```

**聚合说明**
- `interval_seconds = 0`: 返回原始数据点
- `interval_seconds = 60`: 按分钟聚合（AVG）
- `interval_seconds = 3600`: 按小时聚合（AVG）
- `interval_seconds = 86400`: 按天聚合（AVG）

**示例**
```bash
# 查询最近24小时，按1小时聚合
grpcurl -plaintext -d '{
  "server_name": "server1_192.168.1.100",
  "time_range": {
    "start_time": "2024-01-01T00:00:00Z",
    "end_time": "2024-01-02T00:00:00Z"
  },
  "interval_seconds": 3600
}' localhost:50051 monitor.proto.QueryService/QueryTrend
```

---

### 2.3 QueryAnomaly - 异常数据查询

查询超过阈值的异常性能数据，用于预警。

**请求**
```protobuf
message QueryAnomalyRequest {
    string server_name = 1;       // 可选，空表示查询所有服务器
    TimeRange time_range = 2;
    float cpu_threshold = 3;      // CPU阈值，默认80%
    float mem_threshold = 4;      // 内存阈值，默认90%
    float disk_threshold = 5;     // 磁盘阈值，默认85%
    float change_rate_threshold = 6;  // 变化率阈值，默认0.5（50%）
    Pagination pagination = 7;
}
```

**响应**
```protobuf
message QueryAnomalyResponse {
    repeated AnomalyRecord anomalies = 1;
    int32 total_count = 2;
    int32 page = 3;
    int32 page_size = 4;
}

message AnomalyRecord {
    string server_name = 1;
    google.protobuf.Timestamp timestamp = 2;
    string anomaly_type = 3;      // CPU_HIGH, MEM_HIGH, DISK_HIGH, RATE_SPIKE
    string severity = 4;          // WARNING, CRITICAL
    float value = 5;              // 异常值
    float threshold = 6;          // 阈值
    string metric_name = 7;       // 指标名称
}
```

**异常类型说明**

| anomaly_type | 说明 | WARNING 阈值 | CRITICAL 阈值 |
|-------------|------|-------------|--------------|
| CPU_HIGH | CPU使用率过高 | > cpu_threshold | > 95% |
| MEM_HIGH | 内存使用率过高 | > mem_threshold | > 95% |
| DISK_HIGH | 磁盘利用率过高 | > disk_threshold | > 95% |
| RATE_SPIKE | 变化率异常 | > change_rate_threshold | > 100% |

**示例**
```bash
# 查询所有服务器最近1小时的异常
grpcurl -plaintext -d '{
  "time_range": {
    "start_time": "2024-01-01T00:00:00Z",
    "end_time": "2024-01-01T01:00:00Z"
  },
  "cpu_threshold": 80,
  "mem_threshold": 90,
  "pagination": {"page": 1, "page_size": 100}
}' localhost:50051 monitor.proto.QueryService/QueryAnomaly
```

---

### 2.4 QueryScoreRank - 评分排序查询

按评分排序查询所有服务器，用于负载均衡调度。

**请求**
```protobuf
message QueryScoreRankRequest {
    SortOrder order = 1;          // DESC(默认) 或 ASC
    Pagination pagination = 2;
}
```

**响应**
```protobuf
message QueryScoreRankResponse {
    repeated ServerScoreSummary servers = 1;
    int32 total_count = 2;
    int32 page = 3;
    int32 page_size = 4;
}

message ServerScoreSummary {
    string server_name = 1;
    float score = 2;              // 综合评分 (0-100)
    google.protobuf.Timestamp last_update = 3;
    ServerStatus status = 4;      // ONLINE 或 OFFLINE
    float cpu_percent = 10;
    float mem_used_percent = 11;
    float disk_util_percent = 12;
    float load_avg_1 = 13;
}
```

**示例**
```bash
# 获取评分最高的前10台服务器（用于负载均衡）
grpcurl -plaintext -d '{
  "order": "DESC",
  "pagination": {"page": 1, "page_size": 10}
}' localhost:50051 monitor.proto.QueryService/QueryScoreRank

# 获取评分最低的服务器（用于告警）
grpcurl -plaintext -d '{
  "order": "ASC",
  "pagination": {"page": 1, "page_size": 5}
}' localhost:50051 monitor.proto.QueryService/QueryScoreRank
```

---

### 2.5 QueryLatestScore - 最新评分查询

查询所有服务器的最新评分和集群统计信息。

**请求**
```protobuf
message QueryLatestScoreRequest {
    // 无参数
}
```

**响应**
```protobuf
message QueryLatestScoreResponse {
    repeated ServerScoreSummary servers = 1;
    ClusterStats cluster_stats = 2;
}

message ClusterStats {
    int32 total_servers = 1;      // 服务器总数
    int32 online_servers = 2;     // 在线服务器数
    int32 offline_servers = 3;    // 离线服务器数
    float avg_score = 4;          // 平均评分
    float max_score = 5;          // 最高评分
    float min_score = 6;          // 最低评分
    string best_server = 7;       // 最佳服务器
    string worst_server = 8;      // 最差服务器
}
```

**示例**
```bash
grpcurl -plaintext localhost:50051 monitor.proto.QueryService/QueryLatestScore
```

**响应示例**
```json
{
  "servers": [
    {"server_name": "web1_192.168.1.10", "score": 85.5, "status": "ONLINE", ...},
    {"server_name": "web2_192.168.1.11", "score": 72.3, "status": "ONLINE", ...}
  ],
  "cluster_stats": {
    "total_servers": 10,
    "online_servers": 8,
    "offline_servers": 2,
    "avg_score": 75.2,
    "max_score": 92.1,
    "min_score": 45.8,
    "best_server": "web3_192.168.1.12",
    "worst_server": "db1_192.168.1.20"
  }
}
```

---

### 2.6 QueryNetDetail - 网络详细数据查询

查询指定服务器各网卡的详细网络数据。

**请求**
```protobuf
message QueryDetailRequest {
    string server_name = 1;
    TimeRange time_range = 2;
    Pagination pagination = 3;
}
```

**响应**
```protobuf
message QueryNetDetailResponse {
    repeated NetDetailRecord records = 1;
    int32 total_count = 2;
    int32 page = 3;
    int32 page_size = 4;
}

message NetDetailRecord {
    string server_name = 1;
    string net_name = 2;          // 网卡名称（如 eth0, ens33）
    google.protobuf.Timestamp timestamp = 3;
    uint64 err_in = 10;           // 接收错误数
    uint64 err_out = 11;          // 发送错误数
    uint64 drop_in = 12;          // 接收丢弃数
    uint64 drop_out = 13;         // 发送丢弃数
    float rcv_bytes_rate = 20;    // 接收速率 (kB/s)
    float snd_bytes_rate = 21;    // 发送速率 (kB/s)
    float rcv_packets_rate = 22;  // 接收包数速率 (个/s)
    float snd_packets_rate = 23;  // 发送包数速率 (个/s)
}
```

**示例**
```bash
grpcurl -plaintext -d '{
  "server_name": "server1_192.168.1.100",
  "time_range": {
    "start_time": "2024-01-01T00:00:00Z",
    "end_time": "2024-01-01T01:00:00Z"
  },
  "pagination": {"page": 1, "page_size": 100}
}' localhost:50051 monitor.proto.QueryService/QueryNetDetail
```

---

### 2.7 QueryDiskDetail - 磁盘详细数据查询

查询指定服务器各磁盘的详细IO数据。

**请求**: 同 `QueryDetailRequest`

**响应**
```protobuf
message QueryDiskDetailResponse {
    repeated DiskDetailRecord records = 1;
    int32 total_count = 2;
    int32 page = 3;
    int32 page_size = 4;
}

message DiskDetailRecord {
    string server_name = 1;
    string disk_name = 2;         // 磁盘名称（如 sda, nvme0n1）
    google.protobuf.Timestamp timestamp = 3;
    float read_bytes_per_sec = 10;    // 读取速率 (B/s)
    float write_bytes_per_sec = 11;   // 写入速率 (B/s)
    float read_iops = 12;             // 读IOPS
    float write_iops = 13;            // 写IOPS
    float avg_read_latency_ms = 14;   // 平均读延迟 (ms)
    float avg_write_latency_ms = 15;  // 平均写延迟 (ms)
    float util_percent = 16;          // 利用率 (%)
}
```

**示例**
```bash
grpcurl -plaintext -d '{
  "server_name": "server1_192.168.1.100",
  "time_range": {...},
  "pagination": {"page": 1, "page_size": 100}
}' localhost:50051 monitor.proto.QueryService/QueryDiskDetail
```

---

### 2.8 QueryMemDetail - 内存详细数据查询

查询指定服务器的详细内存分布数据。

**请求**: 同 `QueryDetailRequest`

**响应**
```protobuf
message QueryMemDetailResponse {
    repeated MemDetailRecord records = 1;
    int32 total_count = 2;
    int32 page = 3;
    int32 page_size = 4;
}

message MemDetailRecord {
    string server_name = 1;
    google.protobuf.Timestamp timestamp = 2;
    float total = 10;             // 总内存 (GB)
    float free = 11;              // 空闲内存 (GB)
    float avail = 12;             // 可用内存 (GB)
    float buffers = 13;           // Buffers (GB)
    float cached = 14;            // Cached (GB)
    float active = 15;            // 活跃内存 (GB)
    float inactive = 16;          // 非活跃内存 (GB)
    float dirty = 17;             // 脏页 (GB)
}
```

**示例**
```bash
grpcurl -plaintext -d '{
  "server_name": "server1_192.168.1.100",
  "time_range": {...},
  "pagination": {"page": 1, "page_size": 100}
}' localhost:50051 monitor.proto.QueryService/QueryMemDetail
```

---

### 2.9 QuerySoftIrqDetail - 软中断详细数据查询

查询指定服务器各CPU核心的软中断统计。

**请求**: 同 `QueryDetailRequest`

**响应**
```protobuf
message QuerySoftIrqDetailResponse {
    repeated SoftIrqDetailRecord records = 1;
    int32 total_count = 2;
    int32 page = 3;
    int32 page_size = 4;
}

message SoftIrqDetailRecord {
    string server_name = 1;
    string cpu_name = 2;          // CPU名称（如 cpu0, cpu1）
    google.protobuf.Timestamp timestamp = 3;
    int64 hi = 10;                // 高优先级软中断
    int64 timer = 11;             // 定时器软中断
    int64 net_tx = 12;            // 网络发送软中断
    int64 net_rx = 13;            // 网络接收软中断
    int64 block = 14;             // 块设备软中断
    int64 sched = 15;             // 调度软中断
}
```

**示例**
```bash
grpcurl -plaintext -d '{
  "server_name": "server1_192.168.1.100",
  "time_range": {...},
  "pagination": {"page": 1, "page_size": 100}
}' localhost:50051 monitor.proto.QueryService/QuerySoftIrqDetail
```

---

## 三、接口汇总表

| 接口 | 方法 | 说明 | 用途 |
|-----|------|------|------|
| GrpcManager.SetMonitorInfo | POST | 接收监控数据 | 工作者推送数据 |
| GrpcManager.GetMonitorInfo | GET | 获取监控数据 | 测试/调试 |
| QueryService.QueryPerformance | POST | 时间段性能查询 | 历史数据分析 |
| QueryService.QueryTrend | POST | 变化率趋势查询 | 趋势分析/预测 |
| QueryService.QueryAnomaly | POST | 异常数据查询 | 告警/预警 |
| QueryService.QueryScoreRank | POST | 评分排序查询 | 负载均衡调度 |
| QueryService.QueryLatestScore | POST | 最新评分查询 | 集群概览/监控面板 |
| QueryService.QueryNetDetail | POST | 网络详细查询 | 网络问题排查 |
| QueryService.QueryDiskDetail | POST | 磁盘详细查询 | IO问题排查 |
| QueryService.QueryMemDetail | POST | 内存详细查询 | 内存问题排查 |
| QueryService.QuerySoftIrqDetail | POST | 软中断详细查询 | 中断问题排查 |

---

## 四、错误码说明

| gRPC Status Code | 说明 | 常见原因 |
|-----------------|------|---------|
| OK (0) | 成功 | - |
| INVALID_ARGUMENT (3) | 参数无效 | 时间范围错误、分页参数错误、缺少必填字段 |
| NOT_FOUND (5) | 服务器不存在 | server_name 不存在于数据库 |
| UNAVAILABLE (14) | 服务不可用 | MySQL 连接失败、QueryManager 未初始化 |
| DEADLINE_EXCEEDED (4) | 查询超时 | 查询数据量过大、数据库响应慢 |

---

## 五、评分算法

综合评分范围 0-100，分数越高表示服务器性能越好（负载越低）。

**权重配置**
| 指标 | 权重 | 说明 |
|-----|------|------|
| CPU 使用率 | 35% | 核心计算资源 |
| 内存使用率 | 30% | 内存资源 |
| CPU 负载 | 15% | 系统负载 |
| 磁盘 IO | 15% | 存储性能 |
| 网络带宽 | 5% | 网络资源 |

**计算公式**
```
score = (1 - cpu/100) * 0.35 
      + (1 - mem/100) * 0.30 
      + (1 - load/(cores*1.5)) * 0.15 
      + (1 - disk/100) * 0.15
      + (1 - net/max_bandwidth) * 0.05
```

**说明**
- `cores` 为 CPU 核心数
- `1.5` 为负载系数（适用于 I/O 密集型场景）
- 各项值超过 100% 时按 100% 计算

---

## 六、变化率计算

变化率用于分析性能指标的变化趋势，计算公式：

```
rate = (current - previous) / previous
```

**说明**
- 正值表示上升趋势
- 负值表示下降趋势
- 0 表示无变化
- 当 previous = 0 时，rate = 0

**变化率字段**
| 字段 | 说明 |
|-----|------|
| cpu_percent_rate | CPU使用率变化率 |
| mem_used_percent_rate | 内存使用率变化率 |
| disk_util_percent_rate | 磁盘利用率变化率 |
| load_avg_1_rate | 1分钟负载变化率 |
| send_rate_rate | 发送速率变化率 |
| rcv_rate_rate | 接收速率变化率 |

---

## 七、服务器标识

服务器使用 `hostname_ip` 格式作为唯一标识。

**格式**: `{hostname}_{ip_address}`

**示例**:
- `server1_192.168.1.100`
- `web-prod-01_10.0.0.50`
- `db-master_172.16.0.10`

**在线状态判断**
- ONLINE: 60秒内有数据上报
- OFFLINE: 超过60秒无数据上报

---

## 八、数据库表结构

系统使用 MySQL 存储性能数据，包含5张表：

| 表名 | 说明 | 主要索引 |
|-----|------|---------|
| server_performance | 性能汇总表（主表） | server_name + timestamp, score |
| server_net_detail | 网络详细数据 | server_name + net_name + timestamp |
| server_disk_detail | 磁盘详细数据 | server_name + disk_name + timestamp |
| server_mem_detail | 内存详细数据 | server_name + timestamp |
| server_softirq_detail | 软中断详细数据 | server_name + cpu_name + timestamp |

**初始化脚本**: `manager/sql/init_server_performance.sql`

---

## 九、编译与运行

### 编译

```bash
# 创建构建目录
mkdir build && cd build

# 配置
cmake ..

# 编译
make -j$(nproc)
```

### 运行

```bash
# 运行管理者服务器（默认监听 0.0.0.0:50051）
./build/manager/manager

# 运行工作者服务器（需要 sudo 权限以加载 eBPF）
sudo ./build/worker/worker <manager_ip>:50051
```

### 数据库配置

在运行前需要修改代码中的数据库连接信息：

**文件**: `manager/src/main.cpp` 和 `manager/src/host_manager.cpp`

```cpp
// 修改为你的 MySQL 配置
const char* host = "localhost";
const char* user = "monitor";        // 你的用户名
const char* password = "monitor123"; // 你的密码
const char* database = "monitor_db";
```

修改后重新编译：
```bash
make -C build -j$(nproc)
```

---

## 十、使用示例

### 10.1 负载均衡场景

获取评分最高的服务器用于任务调度：

```bash
# 获取最佳服务器
grpcurl -plaintext -d '{"order": "DESC", "pagination": {"page": 1, "page_size": 1}}' \
  localhost:50051 monitor.proto.QueryService/QueryScoreRank
```

### 10.2 告警监控场景

查询异常数据用于告警：

```bash
# 查询最近5分钟的异常
grpcurl -plaintext -d '{
  "time_range": {
    "start_time": "2024-01-01T00:00:00Z",
    "end_time": "2024-01-01T00:05:00Z"
  },
  "cpu_threshold": 80,
  "mem_threshold": 90,
  "disk_threshold": 85
}' localhost:50051 monitor.proto.QueryService/QueryAnomaly
```

### 10.3 性能分析场景

分析某服务器过去24小时的性能趋势：

```bash
# 按小时聚合查询
grpcurl -plaintext -d '{
  "server_name": "server1_192.168.1.100",
  "time_range": {
    "start_time": "2024-01-01T00:00:00Z",
    "end_time": "2024-01-02T00:00:00Z"
  },
  "interval_seconds": 3600
}' localhost:50051 monitor.proto.QueryService/QueryTrend
```

### 10.4 集群概览场景

获取集群整体状态：

```bash
grpcurl -plaintext localhost:50051 monitor.proto.QueryService/QueryLatestScore
```

---

## 十一、Proto 文件位置

- 数据接收服务: `proto/monitor_info.proto`
- 查询服务: `proto/query_api.proto`
- 依赖消息: `proto/cpu_*.proto`, `proto/mem_info.proto`, `proto/net_info.proto`, `proto/disk_info.proto`
