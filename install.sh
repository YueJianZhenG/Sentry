#!/bin/bash

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # 无颜色

# 检查是否有root权限
if [ "$(id -u)" -ne 0 ]; then
    echo -e "${RED}错误: 此脚本需要root权限运行。请使用sudo执行。${NC}"
    exit 1
fi

# 检测操作系统（改进识别逻辑，支持完整名称）
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS_FULL="$NAME"  # 完整名称，如 "Debian GNU/Linux"
else
    OS_FULL=$(uname -s)
fi

# 识别发行版系列（支持完整名称匹配）
if [[ "$OS_FULL" == *"Ubuntu"* || "$OS_FULL" == *"Debian"* || "$OS_FULL" == *"Linux Mint"* ]]; then
    OS_FAMILY="Debian"
    PACKAGE_MANAGER="apt-get"
    UPDATE_COMMAND="$PACKAGE_MANAGER update -qq"
    INSTALL_COMMAND="$PACKAGE_MANAGER install -y -qq"
elif [[ "$OS_FULL" == *"CentOS"* || "$OS_FULL" == *"Red Hat"* || "$OS_FULL" == *"Fedora"* ]]; then
    OS_FAMILY="RedHat"
    PACKAGE_MANAGER="yum"
    UPDATE_COMMAND="$PACKAGE_MANAGER update -y -q"
    INSTALL_COMMAND="$PACKAGE_MANAGER install -y -q"
elif [[ "$OS_FULL" == *"Arch"* || "$OS_FULL" == *"Manjaro"* ]]; then
    OS_FAMILY="Arch"
    PACKAGE_MANAGER="pacman"
    UPDATE_COMMAND="$PACKAGE_MANAGER -Syy --noconfirm"
    INSTALL_COMMAND="$PACKAGE_MANAGER -S --noconfirm"
else
    echo -e "${RED}不支持的操作系统: $OS_FULL${NC}"
    exit 1
fi

echo -e "${YELLOW}检测到系统: $OS_FULL (${OS_FAMILY}系列)${NC}"

# 工具列表
TOOLS=("gcc" "g++" "cmake" "ccache")

# 检查并安装工具
for tool in "${TOOLS[@]}"
do
    # 检查是否安装
    if command -v $tool &> /dev/null
    then

        # 打印版本
        echo -n "$tool 版本: "
        if [ "$tool" == "cmake" ]; then
            $tool --version | head -n 1
        else
            $tool --version | head -n 1
        fi
    else
        echo -e "${RED}$tool 未安装，正在安装...${NC}"

        # 安装工具
        $INSTALL_COMMAND $tool || { echo -e "${RED}安装 $tool 失败${NC}"; exit 1; }

        # 验证安装
        if command -v $tool &> /dev/null
        then
            echo -e "${GREEN}$tool 安装成功${NC}"
            echo -n "$tool 版本: "
            if [ "$tool" == "cmake" ]; then
                $tool --version | head -n 1
            else
                $tool --version | head -n 1
            fi
        else
            echo -e "${RED}$tool 安装失败${NC}"
            exit 1
        fi
    fi
done

echo -e "\n${GREEN}所有工具检查和安装完成!${NC}"