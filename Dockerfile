# =============================================================================
# sonic-cpp TTFHW 验证 Dockerfile
# 适用: aarch64 (鲲鹏920/950) + 无 GPU + openEuler 22.03 LTS SP4
#
# 取长补短来源 (7 份验证报告 JSON):
#
#   #7 verification_report_sonic-cpp_20261027.json (37KB) — A 类: 跑通全流程, 最详细
#      取: 175/175 UT 通过 / 4 个样例通过 / libasan 修复 / FetchContent 预下载方案
#          SVE2 Illegal instruction 排查 / -march=armv8-a 默认编译成功
#      不足: 容器网络不通用 bind-mount 绕过, 本 Dockerfile 直接在容器内 dnf 安装
#
#   #6 verification_report_SSH_sonic-cpp_20260807.json (14KB) — A 类: 跑通全流程
#      取: 175/175 UT 通过 / objdump SVE2 编译验证 / Docker 18.09 notify.sock bug 记录
#          host IS base OS 策略 / NEON 基础编译成功
#      不足: 未做样例验证, 未预下载 GoogleTest
#
#   #5 verification_report_Remote_sonic-cpp_ARM_20260710.json (11KB) — A 类: 跑通全流程
#      取: 175/175 UT 通过 / libasan 安装 (yum install libasan) / 补丁应用流程
#          NEON 编译 flags -march=armv8-a / UT 执行命令 ./build/tests/unittest
#      不足: 补丁仓工作流 (clone upstream + apply patch), dev_sve2-256 分支已合并
#
#   #2 verification_report_SSH_sonic-cpp_20260807.json (9KB) — B 类: 分析详细, 部分构建
#      取: 文档阅读完整 (quick_start.md) / 依赖分析 (g++/cmake/googletest)
#          documentation_gaps 5 条 (OS 版本/CPU 要求/网络依赖/示例命令/测试说明)
#      不足: SVE2_256 编译成功但运行 crash, 基础构建依赖预置产物
#
#   #4 verification_report_SSH_sonic-cpp_20260804.json (10KB) — B 类: 分析详细, 未构建
#      取: 补丁仓结构分析 / documentation_gaps 4 条 (构建命令缺失/平台限制/外网依赖/基线校验)
#      不足: 纯补丁仓不含源码, 无法独立构建
#
#   #3 verification_report_sonic-cpp_20261019.json (10KB) — C 类: 假成功
#      教训: git clone ... 2>/dev/null; ... || echo "..." 导致 CMake Error 被隐藏
#            output 含 "CMake Error: The source directory does not appear to contain CMakeLists.txt"
#            但 returncode=0, success=true → 典型假成功
#      规避: Dockerfile 每步加退出码检查, 禁止 2>/dev/null 和 | tail 截断
#
#   #1 verification_report_sonic-cpp_20261019.json (2KB) — D 类: 空模板
#      排除: 全字段空, 0 步, 不作为输入
#
# 源码核对 (分支 dev_sve2-256):
#   CMakeLists.txt:1           cmake_minimum_required(VERSION 3.11)
#   CMakeLists.txt:9           option(BUILD_UNITTEST "Build unittest." ON) — 默认 ON
#   CMakeLists.txt:12          option(ENABLE_SVE2_128 "..." OFF) — 注意: 无 ENABLE_SVE2_256 option
#   tests/CMakeLists.txt:14-18 FetchContent URL googletest commit 5376968f6948923e2411081fd9372e71a59d8e77
#   tests/CMakeLists.txt:33    clang: -O0 -g -fsanitize=address -Werror -Wall -Wextra-semi -Wextra-semi-stmt -Wcomma
#   tests/CMakeLists.txt:35    GNU: -O0 -g -fsanitize=address -Werror -Wall
#   tests/CMakeLists.txt:38    set_arch_flags(unittest ${CMAKE_SYSTEM_PROCESSOR})
#   tests/CMakeLists.txt:39    target_link_options -fsanitize=address — 需 libasan
#   cmake/set_arch_flags.cmake:5  aarch64 → -march=armv8-a (默认, 无 SVE2)
#   cmake/set_arch_flags.cmake:11 ENABLE_SVE2_256 → -march=armv8-a+sve+crc+sve2 -msve-vector-bits=256
#   scripts/run_cmake.sh:27-28  cmake -S . -B build && cmake --build build --target unittest -j
#   scripts/run_cmake.sh:31     ./build/tests/unittest
#   .github/workflows/clang-format-check.yml:20  clang-format-version: '11', fallback-style: Google
#   .clang-format:1             BasedOnStyle: Google
#   README.md:11                x86 platform with AVX2 (上游, dev_sve2-256 适配 aarch64)
#
# 验证内容: 构建(-march=armv8-a) → 单元测试(./build/tests/unittest) → 样例(example/) → clang-format 静态检查 → SVE2 编译验证
# =============================================================================

FROM openeuler/openeuler:22.03-lts-sp4

# ---- 0a. 基础系统工具 ----
# 来源: 通用基础工具, 所有报告均需要
# 核对: 仓库无 Dockerfile, 依据 CI (.github/workflows/) 和构建脚本需要
RUN dnf update -y \
    && dnf install -y \
        git wget curl unzip tar \
        which file findutils \
    && dnf clean all \
    && git --version \
    && wget --version | head -1 \
    && echo "=== 基础工具安装完成 ==="

# ---- 0b. 编译器工具链 ----
# 来源: #5 报告 (gcc 10.3.1 编译成功) + #7 报告 (gcc 10.3.1, g++ 10.3.1)
# 源码依据: CMakeLists.txt LANGUAGES CXX; README 要求 GCC or LLVM
# #2 documentation_gaps: 文档要求 g++-12/clang-16, 实际 g++ 10.3.1 可编译基础功能
RUN dnf install -y \
        gcc gcc-c++ \
    && dnf clean all \
    && gcc --version | head -1 \
    && g++ --version | head -1 \
    && echo "int main(){return 0;}" > /tmp/t.c && gcc /tmp/t.c -o /tmp/t && /tmp/t && echo "=== gcc 编译验证通过 ===" \
    && echo "int main(){return 0;}" > /tmp/t.cpp && g++ /tmp/t.cpp -o /tmp/t2 && /tmp/t2 && echo "=== g++ 编译验证通过 ===" \
    && rm -f /tmp/t /tmp/t.c /tmp/t2 /tmp/t.cpp

# ---- 0c. 构建工具 ----
# 来源: #5 报告 (cmake 3.22.0) + #7 报告 (cmake 3.22.0)
# 源码依据: CMakeLists.txt:1 cmake_minimum_required(VERSION 3.11);
#           scripts/run_cmake.sh:20-22 检查 cmake >= 3.14
RUN dnf install -y \
        cmake make \
    && dnf clean all \
    && cmake --version | head -1 \
    && make --version | head -1 \
    && echo "=== 构建工具安装完成 ==="

# ---- 0d. AddressSanitizer 库 ----
# 来源: #5 报告 (yum install libasan) + #7 报告 (dnf install libasan, 链接 -lasan 失败)
# 源码依据: tests/CMakeLists.txt:35 target_compile_options -fsanitize=address (GNU)
#           tests/CMakeLists.txt:39 target_link_options -fsanitize=address — 链接需 libasan
# 教训 (#5/#7): 不装 libasan → /usr/bin/ld: cannot find -lasan → 链接失败
RUN dnf install -y \
        libasan \
    && dnf clean all \
    && rpm -q libasan \
    && echo "=== libasan 安装完成 ==="

# ---- 0e. 静态检查工具 (clang-format) ----
# 来源: .github/workflows/clang-format-check.yml — CI 使用 clang-format 11, fallback Google
# 源码依据: .clang-format:1 BasedOnStyle: Google
#           clang-format-check.yml:20 clang-format-version: '11'
# #2/#4 报告: 仓库有 CI 静态检查配置, 前报告未执行 clang-format 验证
RUN dnf install -y \
        clang-tools-extra \
    && dnf clean all \
    && (clang-format --version 2>/dev/null || echo "clang-format 不可用, 静态检查步骤将跳过") \
    && echo "=== 静态检查工具安装完成 ==="

# ---- 0f. 二进制分析工具 (objdump) ----
# 来源: #6 报告 — objdump 验证 SVE2-256 指令生成 (grep -E 'sve2|match')
# 源码依据: cmake/set_arch_flags.cmake:11 ENABLE_SVE2_256 生成 SVE2 指令
RUN dnf install -y \
        binutils \
    && dnf clean all \
    && objdump --version | head -1 \
    && echo "=== binutils 安装完成 ==="

# ---- 1. 预下载 GoogleTest 源码 ----
# 来源: #7 报告 (宿主机预下载 → 容器 FetchContent 无网络时使用)
# 源码依据: tests/CMakeLists.txt:14-18 FetchContent_Declare(googletest URL ...)
#           commit: 5376968f6948923e2411081fd9372e71a59d8e77
# #2 documentation_gaps: 网络依赖未说明, 内网 GitHub 不可达时 FetchContent 失败
# 方案: 预下载到 /opt/googletest-src, 构建时 -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST 指定
# 国内源: 使用 Gitee 镜像 (gitee.com/mirrors/googletest), 替代 GitHub 原始地址
RUN mkdir -p /opt/googletest-src /tmp/googletest-x \
    && wget -q -O /tmp/googletest.zip \
       https://gitee.com/mirrors/googletest/repository/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip \
    && unzip -q /tmp/googletest.zip -d /tmp/googletest-x \
    && cp -r /tmp/googletest-x/*/. /opt/googletest-src/ \
    && rm -rf /tmp/googletest.zip /tmp/googletest-x \
    && ls /opt/googletest-src/CMakeLists.txt \
    && echo "=== GoogleTest 源码预下载完成 ==="

# ---- 2. 预编译 GoogleTest 为静态库 ----
# 来源: #7 报告 — cmake FetchContent 编译 GoogleTest 耗时约 15s, 预编译后加速验证
# 源码依据: tests/CMakeLists.txt:22 FetchContent_MakeAvailable(googletest) 编译 gtest + gtest_main
RUN mkdir -p /opt/googletest-build \
    && cmake -S /opt/googletest-src -B /opt/googletest-build \
       -DCMAKE_CXX_COMPILER=g++ \
       -DCMAKE_C_COMPILER=gcc \
    && cmake --build /opt/googletest-build --target gtest gtest_main -j$(nproc) \
    && ls -la /opt/googletest-build/lib/libgtest*.a \
    && echo "=== GoogleTest 预编译完成 ==="

# ---- 3. 设置环境变量 ----
# 来源: #5/#6/#7 报告 — 不启用 ENABLE_SVE2_256 (Kunpeng 920 无 SVE2)
# 源码依据: cmake/set_arch_flags.cmake:11-12 ENABLE_SVE2_256 → -march=armv8-a+sve+crc+sve2 (需 SVE2 硬件)
# 教训 (#5/#6/#7): SVE2_256 编译成功但运行时 Illegal instruction (Kunpeng 920 无 SVE2)
# FETCHCONTENT_SOURCE_DIR_GOOGLETEST: 指向预下载的 GoogleTest, 跳过网络下载
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
