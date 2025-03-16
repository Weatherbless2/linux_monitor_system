# 配置 Protobuf 和 gRPC 库，定义需要编译的 .proto 文件，并生成相应的 C++ 代码
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(c-ares CONFIG)
# find_package(Threads)

#
# Protobuf/Grpc source files
#
# 所有需要编译的 .proto 文件
set(PROTO_FILES
    monitor_info.proto
    cpu_load.proto
    cpu_softirq.proto
    cpu_stat.proto
    mem_info.proto
    net_info.proto
)

#
# Add Library target with protobuf sources
#
# 创建了一个名为 monitor_proto 的库目标，并将之前定义的 .proto 文件添加到这个目标中
# 然后，将 protobuf 和 gRPC 库链接到这个目标，并设置包含目录
add_library(monitor_proto ${PROTO_FILES})
target_link_libraries(monitor_proto
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc
        gRPC::grpc++
)
target_include_directories(monitor_proto PUBLIC
${PROTOBUF_INCLUDE_DIRS} 
${CMAKE_CURRENT_BINARY_DIR})

#
# Compile protobuf and grpc files in mointor proto target to cpp
#
# 获取 gRPC 插件的位置
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
# 生成 Protobuf 和 gRPC 的 C++ 代码
protobuf_generate(TARGET monitor_proto LANGUAGE cpp)
protobuf_generate(TARGET monitor_proto LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")
