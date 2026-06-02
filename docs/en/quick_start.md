# Quick Start

## Environment Requirements

- Verified hardware configuration: Kunpeng 950 processor, supporting the NEON, SVE, and SVE2 instruction sets

- Verified OS: Debian 12

- Verified compiler: clang-16/g++-12

## Enabling the Kunpeng 950 Optimization Patch

For the Kunpeng 950 processor, this repository provides an SVE2-256 optimization patch (`950-sve2-256.patch`) that can further improve the performance of JSON serialization and deserialization.

Obtain the sonic-cpp source code:

```bash
git clone https://github.com/bytedance/sonic-cpp.git
cd sonic-cpp
git checkout 69deb02
```

Obtain and apply the patch:

```bash
wget https://raw.gitcode.com/boostkit/sonic-cpp/raw/master/950-sve2-256.patch
git apply 950-sve2-256.patch
```

> **Note**: The patch provides the following optimizations for the Kunpeng 950 processor:
>
> - Added the SVE2-256 compilation option to enable the SVE2-256 instruction set.
> - Optimized the inline policy for key functions.
> - Improved the parsing logic to better utilize vectorized instructions.

### Directory Structure After the Patch Is Enabled

```text
sonic-cpp/
├── bazel/                    # Bazel build configuration
├── benchmark/                # Performance test code
├── cmake/                    # CMake configuration file
├── example/                  # Sample code
├── fuzz/                     # Fuzz test
├── include/                  # Header file
│   ├── sonic/                # sonic-cpp core header file
│   │   ├── dom/              # DOM-related
│   │   ├── experiment/
│   │   └── internal/         # Internal implementation
│   │       └── arch/         # Architecture-related implementation
│   │           ├── common/   # Common implementation
│   │           ├── avx2/     # AVX2 optimization
│   │           ├── sse/      # SSE optimization
│   │           ├── neon/     # NEON optimization
│   │           ├── sve2-128/ # SVE2-128 optimization
│   │           └── sve2-256/ # SVE2-256 optimization
│   └── thirdparty/
├── licenses/                 # License files
├── scripts/                  # Scripts
├── testdata/                 # Test data
├── tests/                    # Unit test
├── BUILD.bazel
├── CMakeLists.txt
├── CONTRIBUTING.md
├── Doxyfile
├── LICENSE
├── MODULE.bazel
├── README.md
├── WORKSPACE.bzlmod
```

## Benchmark Test Example of sonic-cpp

```bash
mkdir build-llvm-sve2
cmake -S . -B build-llvm-sve2 -DBUILD_BENCH=ON -DENABLE_SVE2_256=ON
cmake --build build-llvm-sve2 --target bench -j
```

Run benchmark/bench:

```bash
numactl -C 310 ./build-llvm-sve2/benchmark/bench --benchmark_filter=Sonic --benchmark_min_time=3s
```

sonic-cpp is a header-only library, which itself does not produce any compiled files. The compiled files are stored in the **build-llvm-sve2** folder.

## Example of Enabling Method of Including Header Files

sonic-cpp is a header-only library. It only requires including the directory containing the header files.

### Serialization and Deserialization Examples

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

Compilation command

```bash
g++ -I./include/ -march=armv8-a+sve2 -msve-vector-bits=256 --std=c++11 -O3 example-test.cpp -o example-test
```

### Enabling Verification

Run the following command:

```bash
objdump -d ./example | grep z0 | grep match
```

If the following information is displayed, the enabling is successful.

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
