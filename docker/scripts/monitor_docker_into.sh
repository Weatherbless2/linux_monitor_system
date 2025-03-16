#!/usr/bin/env bash
# 进入已经运行的容器进行交互操作

# X11 本身是属于主机的，通常情况下，X11 的访问权限是受到严格控制的。
# 防止未经授权的用户或进程访问和操作图形界面。

# 允许本地root 用户访问 X11 显示服务器
xhost +local:root 1>/dev/null 2>&1

# 进入已经运行的名为 linux_monitor 的容器：
#- u root：以 root 用户身份执行命令。
# -it：以交互模式运行，并分配一个伪终端。
# linux_monitor：指定要进入的容器名称。
# /bin/bash：在容器中执行 bash，启动一个 Bash shell。
docker exec \
    -u root \
    -it linux_monitor \
    /bin/bash

# 撤销本地 root 用户对 X11 显示服务器的访问权限
xhost -local:root 1>/dev/null 2>&1
