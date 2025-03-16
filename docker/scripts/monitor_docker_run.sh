#!/usr/bin/env bash
# 启动和配置一个新的 Docker 容器

# 获取脚本所在目录的上两级目录的绝对路径
MONITOR_HOME_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/../.." && pwd )"

# 获取 DISPLAY 变量，确保容器能够正确使用 X11 显示服务器进行图形界面显示
display=""
if [ -z ${DISPLAY} ];then
    display=":1"
else
    display="${DISPLAY}"
fi

# 获取本地主机名、当前用户、用户 ID、组名和组 ID
local_host="$(hostname)"
user="${USER}"
uid="$(id -u)"
group="$(id -g -n)"
gid="$(id -g)"

# 停止名为 linux_monitor 的容器，然后删除它
# 避免启动新容器时出现名称冲突或资源占用的问题
echo "stop and rm docker" 
docker stop linux_monitor > /dev/null
docker rm -v -f linux_monitor > /dev/null

# 启动新的 Docker 容器
# 容器名称为 linux_monitor
# 设置环境变量 DISPLAY 为之前定义的 display
# 以特权模式运行容器
# 设置多个环境变量（如 DOCKER_USER、USER、DOCKER_USER_ID、DOCKER_GRP、DOCKER_GRP_ID、XDG_RUNTIME_DIR）
# 挂载主机的 MONITOR_HOME_DIR 到容器的 /work 目录
# 挂载主机的 XDG_RUNTIME_DIR 到容器的 XDG_RUNTIME_DIR
# 使用主机网络模式
# 使用 linux:monitor 镜像启动容器
echo "start docker"
docker run -it -d \
--name linux_monitor \
-e DISPLAY=$display \
--privileged=true \
-e DOCKER_USER="${user}" \
-e USER="${user}" \
-e DOCKER_USER_ID="${uid}" \
-e DOCKER_GRP="${group}" \
-e DOCKER_GRP_ID="${gid}" \
-e XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR \
-v ${MONITOR_HOME_DIR}:/work \
-v ${XDG_RUNTIME_DIR}:${XDG_RUNTIME_DIR} \
--net host \
linux:monitor
