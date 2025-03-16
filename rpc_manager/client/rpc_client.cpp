#include "rpc_client.h"

namespace monitor {
RpcClient::RpcClient(const std::string &server_address) {
    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    stub_ptr_ = monitor::proto::GrpcManager::NewStub(channel);
}
RpcClient::~RpcClient() {}

// 向 grpc 发送信息。被监控的机器使用
void RpcClient::SetMonitorInfo(const monitor::proto::MonitorInfo &monito_info) {
    // 这里的 context 用于在客户端和服务端之间传递元数据、控制 RPC 的行为以及跟踪 RPC 的状态
    // 例如设置连接的超时时间等
    // 但是这里什么都没写，因为我们不需要这些设置
    ::grpc::ClientContext context;

    // 设置超时时间示例：
    // grpc::ClientContext context;
    // gpr_timespec ts;
    // ts.tv_sec = 10;
    // ts.tv_nsec = 0;
    // ts.clock_type = GPR_TIMESPAN;
    // context.set_deadline(ts);

    // 类型为 Empty，表示不需要响应数据
    ::google::protobuf::Empty response;

    ::grpc::Status status = stub_ptr_->SetMonitorInfo(&context, monito_info, &response);
    if (status.ok()) {
    } else {
        std::cout << status.error_details() << std::endl;
        std::cout << "status.error_message: " << status.error_message() << std::endl;
        std::cout << "falied to connect !!!" << std::endl;
    }
}

// 从 grpc 获取监控信息。主动监控的机器使用
void RpcClient::GetMonitorInfo(monitor::proto::MonitorInfo *monito_info) {
    ::grpc::ClientContext context;
    ::google::protobuf::Empty request;
    ::grpc::Status status = stub_ptr_->GetMonitorInfo(&context, request, monito_info);
    if (status.ok()) {
    } else {
        std::cout << status.error_details() << std::endl;
        std::cout << "status.error_message: " << status.error_message() << std::endl;
        std::cout << "falied to connect !!!" << std::endl;
    }
}
} // namespace monitor
