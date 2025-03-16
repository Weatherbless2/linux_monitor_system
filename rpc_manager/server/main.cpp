#include "rpc_manager.h"
#include <grpc/grpc.h>
#include <grpcpp/server_builder.h>
#include <iostream>

// 初始化并启动 gRPC 服务器，监听端口 50051，并注册

constexpr char kServerPortInfo[] = "0.0.0.0:50051";
void InitServer() {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(kServerPortInfo, grpc::InsecureServerCredentials());

    monitor::GrpcManagerImpl grpc_server;
    builder.RegisterService(&grpc_server);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();

    return;
}

int main() {
    InitServer();
    return 0;
}