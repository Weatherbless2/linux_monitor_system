#!/usr/bin/env bash

# 确保脚本在遇到任何错误时立即退出
set -e

# 将当前工作目录切换到脚本文件所在的目录
cd "$(dirname "${BASH_SOURCE[0]}")"

# https://github.com/abseil/abseil-cpp/archive/refs/tags/20200225.2.tar.gz
# Install abseil.

# 定义变量
# 系统 CPU 的核心数量，用于并行编译
THREAD_NUM=$(nproc)
VERSION="20200225.2"
PKG_NAME="abseil-cpp-${VERSION}.tar.gz"

# 解压，然后进入解压后的目录，使用 cmake配置编译项，然后使用 make 编译并安装库
tar xzf "${PKG_NAME}"
pushd "abseil-cpp-${VERSION}"
    mkdir build && cd build
    cmake .. \
        -DBUILD_SHARED_LIBS=ON \
        -DCMAKE_CXX_STANDARD=14 \
        -DCMAKE_INSTALL_PREFIX=/usr/local
    make -j${THREAD_NUM}
    make install
popd

# 更新动态链接库缓存，以便新安装的库可以被系统识别
ldconfig

# Clean up
rm -rf "abseil-cpp-${VERSION}" "${PKG_NAME}"