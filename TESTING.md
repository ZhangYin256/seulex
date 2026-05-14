# SeuLex 测试流程指南

本文档说明如何运行项目的单元测试、端到端回归测试，以及如何生成覆盖率统计与按模块报告。

## 1. 测试体系概览

项目使用 CMake + CTest + GoogleTest，测试分为两层：

- `unit`：纯 C++ 单元测试（解析器、配置、自动机、代码生成器）
- `e2e`：端到端回归测试（生成 scanner、与 bison 语法分析器联编并执行）

## 2. 构建并运行全部测试

```bash
cd /home/zhangyin/seulex
cmake -S . -B build -DSEULEX_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## 3. 按标签运行测试

仅运行单元测试：

```bash
ctest --test-dir build -L unit --output-on-failure
```

仅运行端到端测试：

```bash
ctest --test-dir build -L e2e --output-on-failure
```

## 4. 端到端测试内容

- `e2e_generate_simple_scanner`：使用 SeuLex 生成 `simple1.l` 的 scanner，校验输出存在并包含 `yylex`。
- `e2e_bison_parser_integration`：
  1. 对 `test/c99.y` 运行 `bison -d`
  2. 用 SeuLex 处理 `test/c99.l`
  3. 用 `cc` 将 `c99.tab.c` 与 `c99.yy.c` 联编
  4. 以 `int x;` 为输入运行 parser，校验流程可通过

## 5. 生成覆盖率报告

覆盖率依赖 `gcovr`。建议使用项目内虚拟环境安装，避免污染系统 Python：

```bash
cd /home/zhangyin/seulex
python3 -m venv .venv-tools
. .venv-tools/bin/activate
pip install gcovr
```

如果系统不支持 `venv`（例如缺少 `python3-venv`），可使用用户级安装：

```bash
python3 -m pip install --user --break-system-packages gcovr
export PATH="$HOME/.local/bin:$PATH"
```

然后在启用覆盖率编译选项的构建目录执行：

```bash
PATH="/home/zhangyin/seulex/.venv-tools/bin:$PATH" \
cmake -S . -B build-coverage -DSEULEX_BUILD_TESTS=ON -DSEULEX_ENABLE_COVERAGE=ON
cmake --build build-coverage -j
cmake --build build-coverage --target coverage
```

## 6. 覆盖率产物位置

覆盖率目标会在 `build-coverage/coverage/` 生成：

- `coverage-summary.txt`：总体覆盖率摘要
- `coverage.html`：按文件可视化 HTML 报告
- `coverage.xml`：CI 可消费的 XML 报告
- `coverage.json`：原始 JSON 数据
- `module-coverage.md`：按 `src/<module>/` 聚合后的模块覆盖率表

## 7. 常见问题

- `gcovr not found`：将虚拟环境 `bin` 放入 `PATH` 后重新执行 `cmake -S . -B ...`。
- `venv` 无法创建：使用用户级安装方式（`pip install --user --break-system-packages gcovr`）。
- `bison/cc not found`：端到端联编测试会被跳过，请先安装依赖。
