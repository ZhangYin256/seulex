# SeuLex开发和使用文档

本文档说明如何在仓库根目录构建 SeuLex 的生成器、使用生成器生成词法分析器代码（C），以及如何与 Bison 生成的语法分析器联合编译与运行。包含常见选项说明、调试与回归测试建议。

## 前提

- Linux 开发环境
- 工具：`cmake`, `make`/`ninja`（或支持的构建工具），`gcc`/`clang`（或 `cc`），`bison`，`flex`（非必需但可用于对比），`git`（可选）

测试与覆盖率的完整流程见 [TESTING.md](TESTING.md)。

## 目录

- 根目录: [seulex](seulex)
- 源码: [seulex/src](seulex/src)
- 生成输出目录（构建）: [seulex/build](seulex/build)
- 测试用源文件: [seulex/test](seulex/test)
- 测试输入: [seulex/test_inputs](seulex/test_inputs)
- 测试输出: [seulex/test_outputs](seulex/test_outputs)

## 构建SeuLex

### 使用CMake

推荐使用 CMake 构建（仓库已包含 CMakeLists）。在仓库根目录运行：

```bash
cd /home/zhangyin/seulex
cmake -S . -B build
cmake --build build --target SeuLex -j
```

CMake会自动寻找构建工具，如果不存在则会报错。构建成功后，生成器可执行文件通常位于 `build/SeuLex`。可以使用绝对或相对路径运行它。

## 运行自动化测试

项目已接入 GoogleTest，并通过 CTest 自动发现测试用例。

```bash
cd /home/zhangyin/seulex
cmake -S . -B build -DSEULEX_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

如果只运行某一组测试，可直接执行测试程序：

```bash
./build/seulex_tests --gtest_filter=RegexParserTest.*
```

### 分步使用CMake和构建工具

或者分步生成不同构建类型，此处使用CMake+Ninja：

情况一：还没有生成构建命令，但需要 Release 版本。

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

说明：先生成 Release 构建文件，再执行完整构建，适合正式发布或性能测试。

情况二：还没有生成构建命令，需要 Debug 版本。

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

说明：一次性完成 Debug 配置和构建，适合调试和排查问题。

情况三：仅修改了部分代码，没有改动 `CMakeLists.txt`、编译选项或文件结构。

```sh
cd build
ninja
```

说明：这是增量构建，Ninja 只会重新编译受影响的文件，速度更快。

## IDE的语法检查器

需要正确配置语法检查器以正确识别include路径与现代C++语法。

可以在构建之后将build目录下的`compile_commands.json`复制到主文件夹（seulex）中供编辑器/语法检查器使用。

## 生成词法分析器

SeuLex 的用法:

```sh
SeuLex [OPTIONS] input.l
```

常用选项：
  -o FILE       将生成文件写入 FILE（默认按输入文件名生成 `<stem>.yy.c`）
  -t            将生成结果输出到 stdout（适合管道）
  -d            生成调试输出
  -h, --help    显示帮助

示例：

```sh
  ./build/SeuLex -o lex.yy.c test_inputs/c99.l
  # 或者输出到 stdout：
  ./build/SeuLex -t test_inputs/c99.l > lex.yy.c
```

## 与 Bison 联合使用

步骤顺序建议如下：

- 先使用 `bison -d` 生成语法分析器（`.tab.c` 和 `.tab.h`），以便 scanner 可以包含 parser 生成的头文件中定义的 token 常量。
- 再运行 SeuLex 生成扫描器源文件（`.c`）。
- 编译并链接 parser 与 scanner。示例：

```sh
# 在 test_outputs 或工程目标目录：
cd /home/zhangyin/seulex/test_outputs
# 1) 运行 bison 生成 parser 源
bison -d ../test/c99.y -o y.tab.c
# 2) 运行 SeuLex 生成 scanner 源（如果尚未在上一步输出目录生成）
../build/SeuLex -o c99.yy.c ../test/c99.l
# 3) 编译并链接（使用系统 C 编译器，并链接 libfl 或自行实现的支持）
cc c99.tab.c c99.yy.c -lfl -o c99parser
# libfl提供了程序的主函数

# 运行 parser（示例）：
printf 'int x;\n' | ./c99parser
```

说明：

- `bison -d` 生成 `c99.tab.h`（token 定义）和 `c99.tab.c`（parse 逻辑）。
- 生成的 scanner 源文件会 `#include "c99.tab.h"`，因此在编译 scanner 时要保证头文件在包含路径下。

## 使用 CSmith 进行随机测试

[CSmith](https://github.com/csmith-project/csmith) 是一个随机 C 程序生成器，可用于对生成的语法分析器进行大规模随机测试。

### 安装 CSmith

```bash
# 下载源码
curl -sL https://github.com/csmith-project/csmith/archive/refs/tags/csmith-2.3.0.tar.gz | tar xz
cd csmith-2.3.0
# 配置、编译、安装
cmake -B build -DCMAKE_INSTALL_PREFIX=~/.local
cmake --build build -j$(nproc)
cmake --install build
```

### 生成测试并运行

```bash
# 生成 10 个随机 C 测试程序
mkdir -p /tmp/csmith_tests
for i in $(seq 1 10); do
  csmith --no-argc --no-longlong -o /tmp/csmith_tests/test${i}.c
done

# 用生成的语法分析器解析每个文件
cd /path/to/test_outputs
for i in $(seq 1 10); do
  ./c99parser < /tmp/csmith_tests/test${i}.c && echo "test${i}: OK" || echo "test${i}: FAIL"
done
```

CSmith 生成的 C 代码遵循 C99 标准，覆盖了丰富的语法结构（指针、数组、结构体、控制流等），可有效检验语法分析器的完备性。

