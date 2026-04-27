# 快速入门

## 环境要求

- 已验证的硬件配置：鲲鹏950处理器，支持NEON/SVE/SVE2指令集

- 已验证的OS：Debian 12

- 已验证的编译器：clang-16/g++-12

## 使能鲲鹏950优化补丁

针对鲲鹏950处理器，本仓库提供了SVE2-256优化补丁 `950-sve2-256.patch`，可进一步提升Json序列化、反序列化性能。该补丁主要针对鲲鹏950处理器进行了以下优化：

- 添加SVE2-256编译选项支持，使能SVE2-256指令集。
- 优化关键函数的inline策略。
- 改进解析逻辑以更好地利用向量化指令。

1. 获取sonic-cpp源码。

   ```bash
   git clone https://github.com/bytedance/sonic-cpp.git
   cd sonic-cpp
   git checkout 69deb02
   ```

2. 获取并合入补丁。

   ```bash
   wget https://raw.gitcode.com/boostkit/sonic-cpp/raw/master/950-sve2-256.patch
   git apply 950-sve2-256.patch
   ```

### 使能后目录结构

```text
sonic-cpp/
├── bazel/                    # Bazel构建配置
├── benchmark/                # 性能测试代码
├── cmake/                    # CMake配置文件
├── example/                  # 示例代码
├── fuzz/                     # Fuzz测试
├── include/                  # 头文件
│   ├── sonic/                # sonic-cpp核心头文件
│   │   ├── dom/              # DOM相关
│   │   ├── experiment/
│   │   └── internal/         # 内部实现
│   │       └── arch/         # 架构相关实现
│   │           ├── common/   # 通用实现
│   │           ├── avx2/     # AVX2优化
│   │           ├── sse/      # SSE优化
│   │           ├── neon/     # NEON优化
│   │           ├── sve2-128/ # SVE2-128优化
│   │           └── sve2-256/ # SVE2-256优化
│   └── thirdparty/
├── licenses/                 # 许可证文件
├── scripts/                  # 脚本工具
├── testdata/                 # 测试数据
├── tests/                    # 单元测试
├── BUILD.bazel
├── CMakeLists.txt
├── CONTRIBUTING.md
├── Doxyfile
├── LICENSE
├── MODULE.bazel
├── README.md
├── WORKSPACE.bzlmod
```

## sonic-cpp自带Benchmark测试示例

```bash
mkdir build-llvm-sve2
cmake -S . -B build-llvm-sve2 -DBUILD_BENCH=ON -DENABLE_SVE2_256=ON
cmake --build build-llvm-sve2 --target bench -j
```

运行benchmark/bench。

```bash
numactl -C 310 ./build-llvm-sve2/benchmark/bench --benchmark_filter=Sonic --benchmark_min_time=3s
```

sonic-cpp是头文件库，本身无编译产物，测试编译产物在build-llvm-sve2文件夹下。

## 包含头文件使能方法示例

sonic-cpp是一个header-only库，只需要包含头文件目录即可。

### 序列化与反序列化样例

```cpp
#include "sonic/sonic.h"

#include <string>
#include <iostream>

int main()
{
  std::string json = R"(
    {
      "a": 1,
      "b": 2
    }
  )";

  sonic_json::Document doc;
  doc.Parse(json);

  sonic_json::WriteBuffer wb;
  doc.Serialize(wb);
  std::cout << wb.ToString() << std::endl;
}
```

编译命令。

```bash
g++ -I./include/ -march=armv8-a+sve2 -msve-vector-bits=256 --std=c++11 -O3 example-test.cpp -o example-test
```

### 使能验证

执行如下命令查看。

```bash
objdump -d ./example | grep z0 | grep match
```

回显如下结果说明使能成功。

```text
    41e4:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    41f8:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    42e0:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    4434:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    4448:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    44bc:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    44d0:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    4570:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    4580:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    4620:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    46b8:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    478c:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    484c:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    4860:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    49ec:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    4db0:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    5010:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    559c:       45218011        nmatch  p1.b, p0/z, z0.b, z1.b
    7454:       45208031        nmatch  p1.b, p0/z, z1.b, z0.b
```
