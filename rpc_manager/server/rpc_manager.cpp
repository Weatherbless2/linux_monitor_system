#include "rpc_manager.h"
#include <iostream>

// 这些代码是指 grpc 服务器（中转站）的代码

namespace monitor {
GrpcManagerImpl::GrpcManagerImpl() {}

// grpc 接收并存储监控信息
GrpcManagerImpl::~GrpcManagerImpl() {}

::grpc::Status GrpcManagerImpl::SetMonitorInfo(::grpc::ServerContext *context,
                                               const ::monitor::proto::MonitorInfo *request,
                                               ::google::protobuf::Empty *response) {
    monitor_infos_.Clear();
    monitor_infos_ = *request;
    return grpc::Status::OK;
}

// grpc 返回存储的监控信息
::grpc::Status GrpcManagerImpl::GetMonitorInfo(::grpc::ServerContext *context,
                                               const ::google::protobuf::Empty *request,
                                               ::monitor::proto::MonitorInfo *response) {
    *response = monitor_infos_;
    return grpc::Status::OK;
}

} // namespace monitor
