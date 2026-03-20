# 1 项目介绍

sonic-cpp是由字节跳动研发的一款面向C++语言的高效JSON库，极致地利用当前CPU硬件特性与向量化编程，大幅提高了JSON序列化反序列化性能。

ksonic-cpp(Kunpeng Sonic Cpp)是鲲鹏参与sonic-cpp开源社区的仓库，基于华为鲲鹏处理器利用向量化指令集对sonic-cpp中的JSON序列化、反序列化函数进行性能优化，适用于鲲鹏950处理器，函数接口与开源sonic-cpp保持一致。

# 2 目录结构

```text
ksonic-cpp/
├── bazel/                    # Bazel构建配置
├── benchmark/                # 性能测试代码
├── cmake/                    # CMake配置文件
├── docs/                     # 项目文档
│   ├── images/               # 文档图片资源
│   └── usage.md
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

# 3 版本说明

| ksonic-cpp | 开源sonic-cpp |  特性 |
| ------------ | ------------ | ------------ |
|  v1.0.0 |  对标sonic-cpp master分支69deb02 commit点 |针对鲲鹏处理器增加JSON序列化、反序列化函数性能优化，支持NEON/SVE指令集|

# 4 环境部署

## 环境要求

- 鲲鹏950平台，支持NEON/SVE指令集

已验证的硬件配置：鲲鹏架构下的Debian 12操作系统。

## 编译测试说明

以clang编译器为例：

```bash
export CC=clang-16
export CXX=clang++-16
mkdir build-llvm-sve2
cmake -S . -B build-llvm-sve2 -DBUILD_BENCH=ON -DENABLE_SVE2_256=ON
cmake --build build-llvm-sve2 --target bench -j
```

运行benchmark/bench：

```bash
numactl -C 310 ./build-llvm-sve2/benchmark/bench --benchmark_filter=Sonic --benchmark_min_time=3s
```

ksonic-cpp是头文件库，本身无编译产物，测试编译产物在build-llvm-sve2文件夹下。

# 5 快速上手

## 鲲鹏950优化补丁

针对鲲鹏950处理器，本仓库提供了SVE2-256优化补丁 `950-sve2-256.patch`，可进一步提升JSON序列化、反序列化性能。

应用补丁：

```bash
git apply 950-sve2-256.patch
```

> **说明：** 该补丁主要针对鲲鹏950处理器进行了以下优化：
>
> - 添加SVE2-256编译选项支持，使能SVE2-256指令集
> - 优化关键函数的inline策略
> - 改进解析逻辑以更好地利用向量化指令

## 包含头文件

Sonic-Cpp是一个header-only库，只需要包含头文件目录即可：

```bash
-I/path/to/sonic/include/
```

## 序列化与反序列化

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

编译命令

```bash
g++ -I./include/ -march=armv8-a+sve2 -msve-vector-bits=256 --std=c++11 -O3 example.cpp -o example
```

## 使能验证

执行如下命令查看

```bash
objdump -d ./example | grep z0 | grep match
```

回显如下结果说明使能成功

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

对外接口具体声明可参考[API参考文档](docs/zh/api.md)。

# 6 贡献指南

如果使用过程中有任何问题，或者需要反馈特性需求和bug报告，可以提交isssues联系我们，具体贡献方法可参考[这里](https://gitcode.com/boostkit/community/blob/master/docs/contributor/contributing.md)。

# 7 免责声明

此代码仓计划参与sonic-cpp软件开源，仅对sonic-cpp部分函数在鲲鹏处理器上进行性能优化，编码风格遵照原生开源软件，继承原生开源软件安全设计，不破坏原生开源软件设计及编码风格和方式，软件的任何漏洞与安全问题，均由相应的上游社区根据其漏洞和安全响应机制解决。请密切关注上游社区发布的通知和版本更新。鲲鹏计算社区对软件的漏洞及安全问题不承担任何责任。

# 8 LICENSE

ksonic-cpp遵循 Apache-2.0许可证，具体请参见[LICENSE文件](LICENSE)。

本项目的文档适用CC-BY 4.0许可证，具体请参见[LICENSE文件](docs/LICENSE)。
