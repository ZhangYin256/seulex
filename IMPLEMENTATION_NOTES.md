# SeuLex 实现记录

## 项目目标
本文记录 `seulex` 当前实现细节，包括此前已完成内容与新增功能。

当前主流程：
- 读取 `.l` 文件并切分定义区、规则区、用户代码区。
- 解析规则与动作代码。
- 将 Flex 扩展正则转换为普通正则。
- 将普通正则解析为 AST。
- 将 AST 构造成 NFA。

## 已实现能力

### 词法输入扫描
文件：`src/parser/lex_input_scanner.cpp`

已实现：
- 分段读取定义区、规则区、用户代码区。
- 处理 CRLF 行尾，去除 `\r`。
- 跳过定义区 `/* ... */` 注释。
- 规则行分割时，避免把字符类中的空白误识别为 regex/action 分隔。
- 动作代码支持跨行块，并统计花括号匹配。
- 花括号统计时会忽略字符串、字符常量、块注释、行注释内的 `{}`。

### Flex 扩展正则转换
文件：`src/parser/flex_regex_converter.cpp`

已实现：
- 宏展开，含循环依赖检测。
- 字符类原样保留。
- 双引号字面量转义为普通正则字面量。
- 常规反斜杠转义保留。

可处理示例：
- `L?\"(\\.|[^\\\"\n])*\"`
- `[ \t\v\n\f]`
- `("{"|"<%")`

### 正则解析到 AST
文件：`src/parser/regex_parser.cpp`

已实现：
- 递归下降解析：`|`、连接、`*`、`+`、`?`、分组、字符类。
- 支持转义字符、八进制转义、十六进制转义。
- `RegexNode` 析构释放子树与字符类范围，保证 AST 所有权正确。

### AST 到 NFA
文件：`src/automata/nfa_builder.h`、`src/automata/nfa_builder.cpp`

已实现：
- Thompson 构造法。
- 支持节点：`Char`、`CharClass`、`Empty`、`Concat`、`Alter`、`Star`、`Plus`、`Option`。
- 支持全局起始状态，将所有规则起点以 epsilon 边连接。
- 接受态写入 `rule_index` 与 `action_index`。


## 已完成验证

### 构建验证
- 命令：`cmake --build /home/zhangyin/seulex/build -j2`
- 结果：构建通过。

### 功能验证
- 在 `c99.l` 上运行，完成规则解析、正则转换、AST 构造、NFA 构造。
- 在小样例上验证 Mermaid 输出可渲染。

## 当前限制
- NFA 转移目前为字节级转移，字符类会展开为多条字符边。
- `%s/%x` 起始条件尚未并入 NFA 分区起始状态。
- 行首锚定、尾随上下文等高级 flex 语义尚未贯通到 automata 阶段。
- DFA 子集构造、最小化与代码生成尚未在 `seulex` 主线接通。

## 后续建议
- 实现 epsilon-closure + move 的 NFA->DFA 子集构造。
- 实现 DFA 最小化与转移表压缩。
- 将起始条件与规则优先级完整并入 automata 构造。
- 增加阶段性回归测试（converter/parser/nfa/visualizer）。
