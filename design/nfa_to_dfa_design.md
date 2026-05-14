# NFA 到 DFA 详细设计

## 目标

本设计文档描述 `seulex` 中 NFA 到 DFA 的实现方案，目标包括：

- 用子集构造法将 NFA 转换为可执行的 DFA。
- 在 DFA 接受态上保留规则元数据（`rule_index`、`action_index`）。
- 满足 Flex 的两条核心规则：
  - 选择最长前缀。
  - 在同样长度下选择 `.l` 文件中最先出现的规则。

## 相关文件

- `src/automata/subset_construction.h`
- `src/automata/subset_construction.cpp`
- `src/automata/dfa_matcher.h`
- `src/automata/dfa_matcher.cpp`
- `src/types.h`

## 输入与输出

### 输入

- NFA 状态表：`std::vector<NfaState>`
- NFA 统一入口状态：`int nfa_start`

### 输出

- DFA 状态表：`std::vector<DfaState>`

每个 `DfaState` 包含：

- 对应的 NFA 状态集合 `nfa_state_ids`
- `transitions[256]` 转移表
- 是否接受态以及对应规则信息

## 子集构造流程

`SubsetConstruction::convert()` 的流程如下：

1. 计算 `{nfa_start}` 的 epsilon 闭包，作为 DFA 起始状态。
2. 用队列进行 BFS，逐个处理尚未展开的 DFA 状态。
3. 对每个输入字节 `c in [0,255]`：
   - 计算 `move(current_set, c)`。
   - 对结果再做 epsilon 闭包得到 `next_set`。
   - 若 `next_set` 是新集合，则创建新 DFA 状态并入队。
   - 记录 `current_state.transitions[c] = next_state_id`。
4. 队列为空后转换结束。

## 关键操作定义

### epsilon 闭包

`epsilon_closure(S)`：从状态集 `S` 出发，仅沿 epsilon 边可达的全部状态。

实现采用队列，直到没有新状态可加入。

### move

`move(S, c)`：从状态集 `S` 出发，沿字符 `c` 的非 epsilon 边可达状态集合。

## 接受态决策与规则优先级

### 同长度规则冲突

当一个 DFA 状态对应多个 NFA 接受态时，表示在“同一已读长度”上有多个规则可接受。

Flex 规则要求：同长度时选择 `.l` 文件中最先出现的规则。

实现方式：

- 在 `resolve_accept_metadata()` 中遍历 `nfa_set`。
- 仅考虑 `is_accept=true` 的 NFA 状态。
- 选择最小 `rule_index` 作为该 DFA 状态的 `rule_index` 与 `action_index`。

这保证了“同长度下按规则先后”的行为。

## 最长前缀策略

最长前缀属于“DFA 执行阶段”策略，不是在子集构造阶段直接决定。

`DfaMatcher::longest_prefix_match()` 的执行逻辑：

1. 从起始 DFA 状态开始读入字符。
2. 每走一步若到达接受态，更新 `last_accept_state` 和 `last_accept_pos`。
3. 遇到无法转移时停止。
4. 若存在 `last_accept_state`，返回该状态对应动作和匹配长度。
5. 否则返回不匹配。

该策略保证：

- 总是选择读取过程中最后一次出现的接受位置，即最长前缀。
- 若同一长度出现多个规则，依赖 DFA 状态中已选定的最小 `rule_index`。

## 正确性说明

对于任意输入前缀长度 `k`：

- 子集构造保证 DFA 状态精确对应 NFA 在该长度下可达状态集合。
- 该集合中若存在多个接受态，`resolve_accept_metadata()` 选择最小 `rule_index`。
- 执行时 `DfaMatcher` 保留“最后接受位置”，因此得到最大 `k`。

因此组合后满足 Flex 语义：

- 先按最大匹配长度；
- 长度相同按规则出现顺序。

## 当前范围与后续扩展

当前已覆盖：

- NFA 到 DFA 的核心转换。
- Flex 两条优先级规则在执行语义上的实现基础。

后续可以扩展：

- 起始条件 `%s/%x` 的多入口 DFA。
- trailing context 规则与回退长度。
- DFA 最小化和表压缩。
- 与代码生成模块联动，输出可编译 scanner。
