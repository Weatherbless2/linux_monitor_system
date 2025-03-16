# 指定基础镜像为 Ubuntu 18.04
FROM  ubuntu:18.04

# 设置构建时的环境变量
# 避免在安装软件包时出现交互提示
ARG DEBIAN_FRONTEND=noninteractive
# 设置时区
ENV TZ=Asia/Shanghai

# 使用 Bash 作为默认的 shell
SHELL ["/bin/bash", "-c"]

# 清理 apt 缓存，然后将本地的 sources.list 文件复制到容器的 /etc/apt/ 目录下，以使用自定义的 apt 源
RUN apt-get clean && \
    apt-get autoclean
COPY apt/sources.list /etc/apt/

# 更新 apt 包列表并升级系统
# 然后安装一系列软件包
RUN apt-get update  && apt-get upgrade -y  && \
    apt-get install -y \
    htop \
    apt-utils \
    curl \
    git \
    openssh-server \
    build-essential \
    qtbase5-dev \
    qtchooser \
    qt5-qmake \
    qtbase5-dev-tools \
    libboost-all-dev \
    net-tools \
    vim \
    stress 

# 安装开发库和工具
RUN apt-get install -y libc-ares-dev  libssl-dev gcc g++ make 
RUN apt-get install -y  \
    libx11-xcb1 \
    libfreetype6 \
    libdbus-1-3 \
    libfontconfig1 \
    libxkbcommon0   \
    libxkbcommon-x11-0


# 以下是将本地的安装脚本复制到容器中并执行安装
# 注意 /tmp 是在镜像中的，不是在宿主机
 COPY install/cmake /tmp/install/cmake
 RUN /tmp/install/cmake/install_cmake.sh

COPY install/protobuf /tmp/install/protobuf
RUN /tmp/install/protobuf/install_protobuf.sh

COPY install/abseil /tmp/install/abseil
RUN /tmp/install/abseil/install_abseil.sh

COPY install/grpc /tmp/install/grpc
RUN /tmp/install/grpc/install_grpc.sh



#  RUN apt-get install -y python3-pip
#  RUN pip3 install cuteci -i https://mirrors.aliyun.com/pypi/simple

#  COPY install/qt /tmp/install/qt
#  RUN /tmp/install/qt/install_qt.sh






