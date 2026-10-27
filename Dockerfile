FROM swr.cn-north-4.myhuaweicloud.com/cloud_boostkit/openeuler-22.03-lts-sp4:latest

# ---- 镜像获取方式 (二选一) ----
# 方式一 自行构建: 在本文件所在目录执行
#   docker build -t <镜像名>:<标签> .
# 方式二 直接拉取: 使用已按本 Dockerfile 构建并推送好的镜像, 无需再构建
#   docker pull swr.cn-north-4.myhuaweicloud.com/cloud_boostkit/openeuler22.03_lts_sp4:sonic-cpp_26.3.0.01

# ---- 0a. 基础系统工具 ----
RUN dnf update -y \
    && dnf install -y \
        git wget curl unzip tar \
        which file findutils \
    && dnf clean all \
    && git --version \
    && wget --version | head -1 \
    && echo "=== 基础工具安装完成 ==="

# ---- 0b. 编译器工具链 ----
RUN dnf install -y \
        gcc gcc-c++ \
    && dnf clean all \
    && gcc --version | head -1 \
    && g++ --version | head -1 \
    && echo "int main(){return 0;}" > /tmp/t.c && gcc /tmp/t.c -o /tmp/t && /tmp/t && echo "=== gcc 编译验证通过 ===" \
    && echo "int main(){return 0;}" > /tmp/t.cpp && g++ /tmp/t.cpp -o /tmp/t2 && /tmp/t2 && echo "=== g++ 编译验证通过 ===" \
    && rm -f /tmp/t /tmp/t.c /tmp/t2 /tmp/t.cpp

# ---- 0c. 构建工具 ----
RUN dnf install -y \
        cmake make \
    && dnf clean all \
    && cmake --version | head -1 \
    && make --version | head -1 \
    && echo "=== 构建工具安装完成 ==="

# ---- 0d. AddressSanitizer 库 ----
RUN dnf install -y \
        libasan \
    && dnf clean all \
    && rpm -q libasan \
    && echo "=== libasan 安装完成 ==="

# ---- 0e. 静态检查工具 (clang-format) ----
RUN dnf install -y \
        clang-tools-extra \
    && dnf clean all \
    && (clang-format --version 2>/dev/null || echo "clang-format 不可用, 静态检查步骤将跳过") \
    && echo "=== 静态检查工具安装完成 ==="

# ---- 0f. 二进制分析工具 (objdump) ----
RUN dnf install -y \
        binutils \
    && dnf clean all \
    && objdump --version | head -1 \
    && echo "=== binutils 安装完成 ==="

# ---- 1. 预下载 GoogleTest 源码 ----
RUN mkdir -p /opt/googletest-src /tmp/googletest-x \
    && wget -q -O /tmp/googletest.zip \
       https://gitee.com/mirrors/googletest/repository/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip \
    && unzip -q /tmp/googletest.zip -d /tmp/googletest-x \
    && cp -r /tmp/googletest-x/*/. /opt/googletest-src/ \
    && rm -rf /tmp/googletest.zip /tmp/googletest-x \
    && ls /opt/googletest-src/CMakeLists.txt \
    && echo "=== GoogleTest 源码预下载完成 ==="

# ---- 2. 预编译 GoogleTest 为静态库 ----
RUN mkdir -p /opt/googletest-build \
    && cmake -S /opt/googletest-src -B /opt/googletest-build \
       -DCMAKE_CXX_COMPILER=g++ \
       -DCMAKE_C_COMPILER=gcc \
    && cmake --build /opt/googletest-build --target gtest gtest_main -j$(nproc) \
    && ls -la /opt/googletest-build/lib/libgtest*.a \
    && echo "=== GoogleTest 预编译完成 ==="

# ---- 3. 设置环境变量 ----
ENV FETCHCONTENT_SOURCE_DIR_GOOGLETEST=/opt/googletest-src
ENV GTEST_LIBRARY_DIR=/opt/googletest-build/lib

# ---- 4. 工作目录 ----
WORKDIR /workspace

# ---- 默认 CMD: 交互式 shell ----
# 用法:
#   1. 交互式: docker run -it --rm -v <sonic-cpp源码>:/workspace/sonic-cpp <镜像>
#      容器内执行: /opt/verify.sh
#   2. 一键验证: docker run --rm -v <sonic-cpp源码>:/workspace/sonic-cpp <镜像> /opt/verify.sh
CMD ["/bin/bash"]
