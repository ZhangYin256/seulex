# AST 到 NFA 详细设计

## 设计目标

`seulex` 的这一阶段负责把正则表达式 AST 转换为 NFA，供后续的 DFA 构造、最小化和代码生成使用。设计目标如下：

- 正确实现基本正则语义：字面量、字符类、连接、选择、闭包、至少一次、可选。
- 保持实现简单、可验证，采用经典 Thompson 构造法。
- 支持多规则合并：每条规则形成一个独立 NFA 片段，再通过全局起始状态统一连接。
- 为后续扩展预留字段：规则编号、动作编号、尾部上下文等。

## 输入与输出

### 输入

输入是正则解析器输出的 AST，节点类型定义在 `src/types.h`：

- `Char`
- `CharClass`
- `Empty`
- `Concat`
- `Alter`
- `Star`
- `Plus`
- `Option`

### 输出

输出是 `std::vector<NfaState>`，其中：

- 每个 `NfaState` 代表一个状态。
- 每个状态包含若干条转移边：
  - `epsilon = true` 表示空转移。
  - `epsilon = false` 表示单字节字符转移。
- 接受态会记录：
  - `is_accept`
  - `rule_index`
  - `action_index`

## 递归下降设计思路

这一阶段的正则解析采用递归下降。它的核心思想是把正则表达式按优先级拆成多个子问题，每一层负责一种运算：

- `parse_alternation()` 负责 `|`。
- `parse_concatenation()` 负责隐式连接。
- `parse_quantifier()` 负责 `*`、`+`、`?`。
- `parse_atom()` 负责最小原子：分组、字符类、转义字符、普通字符和 `.`。

这种拆分方式有两个优点：

- 结构和语义一一对应，便于定位问题。
- 每一层只关心自己的运算符，不需要一次性处理所有语法细节。

### 递归下降的执行顺序

解析入口是 `RegexParser::parse()`。它会先保存输入模式到 `pattern_`，然后把当前位置 `pos_` 置零，接着从最高层的 `parse_alternation()` 开始。

递归下降的调用链可以理解为：

1. 从 `parse_alternation()` 进入。
2. 其内部先调用 `parse_concatenation()`。
3. `parse_concatenation()` 再调用 `parse_quantifier()`。
4. `parse_quantifier()` 继续调用 `parse_atom()`。
5. `parse_atom()` 识别具体的原子。
6. 当原子内部包含分组时，再回到 `parse_alternation()` 递归处理子表达式。

这个顺序自然形成了优先级关系：原子最高，量词次之，连接再次之，`|` 最低。

### 处理流程示例

以表达式 `(ab|c)*d?` 为例，解析过程如下：

1. `parse()` 进入后调用 `parse_alternation()`。
2. `parse_alternation()` 先调用 `parse_concatenation()`。
3. `parse_concatenation()` 先调用 `parse_quantifier()`。
4. `parse_quantifier()` 调用 `parse_atom()`，遇到 `(`，于是进入 `parse_group()`。
5. `parse_group()` 内部再次调用 `parse_alternation()`，开始解析括号内的 `ab|c`。
6. 括号内先解析 `ab`：
   - `parse_concatenation()` 先拿到 `a`。
   - 接着继续解析 `b`，用 `Concat` 把 `a` 和 `b` 串起来。
7. 看到 `|` 后，`parse_alternation()` 再解析右侧的 `c`。
8. 最终括号内构成 `Alter(Concat(a, b), c)`。
9. 回到外层，看到 `*`，`parse_quantifier()` 把整个分组包装成 `Star(...)`。
10. 继续向后解析 `d?`：
    - `d` 被解析成 `Char('d')`。
    - `?` 让它变成 `Option(Char('d'))`。
11. 外层 `parse_concatenation()` 把 `Star(...)` 和 `Option(Char('d'))` 用 `Concat` 连接起来。
12. `parse()` 检查输入已完全消费，返回最终 AST。

最终结构可以写成：

```text
Concat(
  Star(
    Alter(
      Concat(Char('a'), Char('b')),
      Char('c')
    )
  ),
  Option(Char('d'))
)
```

## 核心数据结构

### `NfaTransition`

定义于 `src/types.h`：

- `epsilon`：是否为空转移。
- `input_char`：当 `epsilon == false` 时表示输入字符。
- `target_id`：目标状态编号。

### `NfaState`

定义于 `src/types.h`：

- `id`：状态编号。
- `transitions`：出边列表。
- `is_accept`：是否为接受态。
- `rule_index`：接受态对应的规则序号。
- `action_index`：接受态对应的动作序号。
- `is_trailing_accept`：预留字段，用于尾部上下文。
- `original_rule_index`：预留字段，用于尾部上下文。

### `NfaFragment`

定义于 `src/automata/nfa_builder.h`：

- `start_id`：片段起始状态。
- `accept_id`：片段接受状态。

该结构用于临时表示“一个 AST 子树转换后的 NFA 片段”。

## 构造流程

实现入口为 `NfaBuilder::build_from_ast(const RegexNode *root)`，内部调用 `build_node(root)` 递归完成构造。

### 总体流程

1. 遇到 AST 根节点后，递归构造其对应的 NFA 片段。
2. 对于每个叶子节点或操作节点，创建必要的新状态。
3. 用 epsilon 边把子片段拼接为父片段。
4. 返回父片段的起始状态与接受状态。
5. 在规则级别，由上层代码把接受态标记为 `is_accept`，并写入 `rule_index`、`action_index`。

### 状态分配

`NfaBuilder::create_state()` 负责创建状态：

- 新状态编号等于当前 `states_` 大小。
- 创建后立即写入 `state.id`。
- 状态按顺序追加到 `states_` 中。

这种做法有两个好处：

- 编号稳定且连续。
- 便于可视化与调试。

## AST 节点到 NFA 的映射

### `Char`

语义：匹配一个字面字符。

构造：

- 创建两个状态 `s` 和 `a`。
- 添加一条字符边：`s --ch--> a`。
- 返回 `{s, a}`。

### `CharClass`

语义：匹配字符类中允许的字符。

构造：

- 创建两个状态 `s` 和 `a`。
- 建立一个 256 位的允许表 `allow`。
- 若 `negated == false`：
  - 先全部置 `false`。
  - 再把 ranges 中的字符区间置 `true`。
- 若 `negated == true`：
  - 先全部置 `true`。
  - 再把 ranges 中的字符区间置 `false`。
- 对每个允许的字节添加一条 `s --byte--> a`。

设计说明：

- 当前实现是字节级 NFA。
- 这样简单、可验证，但字符类较大时会产生较多边。

### `Empty`

语义：空串 epsilon。

构造：

- 创建两个状态 `s` 和 `a`。
- 添加一条 epsilon 边：`s --eps--> a`。
- 返回 `{s, a}`。

### `Concat`

语义：连接表达式 `A B`。

构造：

- 先递归构造左子树 `left`。
- 再递归构造右子树 `right`。
- 添加 epsilon 边：`left.accept --eps--> right.start`。
- 返回 `{left.start, right.accept}`。

语义解释：

- 输入必须先匹配左部分，再匹配右部分。

### `Alter`

语义：选择表达式 `A | B`。

构造：

- 分别构造左、右子片段。
- 新建统一起点 `s` 和统一接受点 `a`。
- 添加 epsilon 边：
  - `s --eps--> left.start`
  - `s --eps--> right.start`
  - `left.accept --eps--> a`
  - `right.accept --eps--> a`
- 返回 `{s, a}`。

语义解释：

- 输入可以走任一分支。
- NFA 的非确定性天然适合实现 `|`。

### `Star`

语义：零次或多次重复 `A*`。

构造：

- 构造子片段 `child`。
- 新建统一起点 `s` 和统一接受点 `a`。
- 添加 epsilon 边：
  - `s --eps--> child.start`
  - `s --eps--> a`
  - `child.accept --eps--> child.start`
  - `child.accept --eps--> a`
- 返回 `{s, a}`。

语义解释：

- `s --eps--> a` 保证“零次重复”合法。
- `child.accept --eps--> child.start` 保证重复循环。

### `Plus`

语义：至少一次重复 `A+`。

构造：

- 构造子片段 `child`。
- 新建统一起点 `s` 和统一接受点 `a`。
- 添加 epsilon 边：
  - `s --eps--> child.start`
  - `child.accept --eps--> child.start`
  - `child.accept --eps--> a`
- 返回 `{s, a}`。

语义解释：

- 与 `Star` 的区别是：没有 `s --eps--> a`。
- 因此必须至少匹配一次 `child`。

### `Option`

语义：可选 `A?`。

构造：

- 构造子片段 `child`。
- 新建统一起点 `s` 和统一接受点 `a`。
- 添加 epsilon 边：
  - `s --eps--> child.start`
  - `s --eps--> a`
  - `child.accept --eps--> a`
- 返回 `{s, a}`。

语义解释：

- 允许“直接跳过”子表达式。
- 与 `Star` 相比，只有 0 次或 1 次，没有重复回环。

## 规则级合并

AST 转 NFA 之后，`main.cpp` 会把所有规则连接到一个全局起始状态：

1. 每条规则先构造自己的 NFA 片段。
2. 记录该片段的起始状态 `rule_start`。
3. 构造完成后，调用 `append_unified_entry_state(rule_starts)`：
  - 新建统一入口状态 `unified_entry_state`。
  - 对每个 `rule_start` 添加 epsilon 边：`unified_entry_state --eps--> rule_start`。
4. 把每条规则的接受态标记为接受态，并写入规则编号与动作编号。

这样做的目的：

- 把多条词法规则统一成一个入口。
- 方便后续 DFA 子集构造时从单一起点出发。

## 结合性与优先级

NFA 构造本身不重新决定结合性，结合性由 AST 形状决定：

- `parse_alternation()` 左结合处理 `|`。
- `parse_concatenation()` 左结合处理连接。
- `parse_quantifier()` 只绑定紧邻的单个原子或分组。

因此：

- `ab*` 会被解析为 `Concat(Char('a'), Star(Char('b')))`. 
- `(ab)*` 会被解析为 `Star(Concat(Char('a'), Char('b')))`. 
- `a|b|c` 会被解析为左结合的 `Alter(Alter(a,b),c)`.

NFA 构造阶段只负责忠实地把 AST 语义展开，不再额外调整优先级。

## 可视化与验证

`NfaVisualizer` 会把 NFA 输出为 Mermaid 图：

- epsilon 边标记为 `eps`。
- 多个字符到同一目标的边会被合并成紧凑区间展示。
- 起始态与接受态使用不同样式。

这使得 NFA 结构可以快速肉眼验证。

## 已知限制

### 字节级展开

字符类会被展开为 256 字节范围上的若干边：

- 优点：实现直接，语义清晰。
- 缺点：状态/边数偏多，不适合大字符类或 Unicode 语义。

### 高级 Flex 语义未完全进入 NFA

当前实现尚未完整支持：

- `%s/%x` 起始条件分区。
- 行首锚定与尾部上下文的完整语义。
- 需要回退的 trailing context。

这些字段在 `types.h` 中已有预留，但构造流程尚未完整使用。

### 规则优先级与最长匹配

当前文档描述的是 NFA 构造阶段。真正的词法优先级、最长匹配和动作选择，需要 DFA 或扫描执行阶段继续落实。

## 扩展建议

如果后续要继续完善这一阶段，建议按以下顺序：

1. 增加 AST/NFA 单元测试，覆盖 `Char`、`CharClass`、`Concat`、`Alter`、`Star`、`Plus`、`Option`。
2. 实现 NFA 到 DFA 的子集构造。
3. 在 DFA 构造时处理接受优先级、最长匹配与 trailing context。
4. 将字符类从逐字节边改为区间或位图表示，以优化边数。
5. 增加更多 Mermaid 样例，作为可视化回归测试。

## 参考文件

- [src/automata/nfa_builder.h](../src/automata/nfa_builder.h)
- [src/automata/nfa_builder.cpp](../src/automata/nfa_builder.cpp)
- [src/parser/regex_parser.cpp](../src/parser/regex_parser.cpp)
- [src/types.h](../src/types.h)
- [src/main.cpp](../src/main.cpp)

## 小结

这部分代码的设计重点是“用 Thompson 构造法把 AST 直接翻译成 NFA”。

其优势是：

- 结构清晰。
- 语义直观。
- 容易调试和验证。
- 便于后续扩展到 DFA 与代码生成。

如果要继续扩展完整的 flex 语义，建议在保持这个基础构造不变的前提下，在更高一层加入起始条件、优先级和 trailing context 的处理。
