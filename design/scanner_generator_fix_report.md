# Scanner generator 修复报告

日期: 2026-05-06

## 概要

在将生成的词法器与 Bison（或 bison 兼容的 parser）结合使用时，运行时出现“parser 卡住”或“yylex 反复返回相同 token”的现象。该问题已经在 `scanner_generator` 中修复。本文档说明根因、修改内容、验证步骤，以及后续建议的回归要点。

## 根本原因

- 原实现把对输入流的推进（scanner 的主游标 `seulex_pos`）放在动作执行后执行：当动作代码（action）在其内部直接执行 `return <TOKEN>` 时，控制流会离开 `yylex`，导致位于动作后面的 `seulex_pos = last_pos;` 没有机会执行，主游标停留在旧位置。结果是下一次 `yylex` 再次从相同位置匹配并返回相同 token，表现为 parser 卡住或无限循环。
- 另一个并发问题是嵌入的动作/辅助函数（例如 `comment()`）使用了与主扫描器不同的输入游标（旧实现中 `input()` 读的是独立游标），两者不同步会导致动作内部读取输入后，主扫描游标并未同步更新，从而造成词流错位或停滞。

这两个问题共同导致了偶发但严重的卡住行为，尤其当 action 含 `return`，或 action 内调用像 `input()`、`comment()` 这样的输入消费函数时。

## 修改概要

在生成器（`seulex/src/codegen/scanner_generator.cpp`）做了如下关键改动：

- 在执行 action 之前，先把匹配到的尾位置（`last_pos`）写入主游标；同时记录一个“动作游标”变量用于动作内部读取：
  - 新增 `seulex_action_pos` 以供动作内部读取与推进。
  - 在生成的 `yylex` 中，先执行 `seulex_pos = last_pos; seulex_action_pos = last_pos;`，然后执行 switch(action)（即动作代码）。
- 修改 `input()` shim，使其读取并推进 `seulex_action_pos`（动作游标），而不是与主扫描器分离的旧游标。动作内部若调用 `input()`，会推进 `seulex_action_pos`。
- 在动作执行后（无论动作是否返回），将主游标与动作游标同步：`seulex_pos = seulex_action_pos;`。这样：
  - 如果动作直接 `return`，由于我们在 action 前已经把 `seulex_pos` 先更新到 `last_pos`，从 parser 角度输入已经被消费；action 内若再次调用 `input()` 推进 `seulex_action_pos`，主游标也会在动作结束路径被同步（如果 action 没有机会执行同步语句，主游标也至少已被更新到 `last_pos`，避免无限返回同一 token）。

主要受影响的文件（参照仓库）：

- [seulex/src/codegen/scanner_generator.cpp](seulex/src/codegen/scanner_generator.cpp)

（实现细节：在生成的 scanner 源文件里加入 `static size_t seulex_action_pos`，并在 `yylex` 中按上文逻辑调整位置推进与 `input()` shim。）

## 为什么这能修复卡住

- 将“主游标推进”操作尽量靠前，确保即使动作通过 `return` 离开 `yylex`，主游标已经至少前进到匹配结束位置，避免下一次重复匹配同一输入片段。
- 使用统一的动作游标 `seulex_action_pos` 使动作内部通过 `input()` 消费的字符能反映到主游标更新上，从而保持扫描器与动作读取的一致性。

## 验证步骤（已执行）

1. 生成并编译修复后的 scanner（仓库内已有生成器流程），并与现有 Bison 生成的 parser 进行联调。示例（仓库 `test_outputs` 目录中）：

```bash
cd /home/zhangyin/seulex/test_outputs
cc ./c99.tab.c c99.yy.c -lfl
printf 'int x;\n' | timeout 5s ./c99parser
```

输出应正常返回并结束（在我的本地运行中会输出 `int x;` 并退出），未复现“卡住”。

2. 针对注释与 `input()` 的场景测试：

```bash
printf '/* comment */ int x;\n' | timeout 5s ./c99parser
printf '/* multi\ncomment */ int x;\n' | timeout 5s ./c99parser
```

3. 测试动作内部使用 `input()` / `comment()` 的规则（包括合法与不闭合注释），确认动作能正确推进扫描游标，或在报错路径（比如未闭合注释）下按预期失败并退出。

以上测试均在受影响的示例 `c99.l`/`c99.y` 上执行通过，未见再次卡住。

## 回归测试建议

- 为生成器添加自动化回归：至少包含以下测试用例
  - 一个小样例：简单声明（`int x;`）
  - 注释消费：`/* ... */`、`//...`、多行注释
  - 动作内部调用 `input()` 或自定义 `comment()` 的规则
  - 动作返回 `return` 的规则（确保主游标已进）

- 把这些用例加入 CI，确保在修改生成器时能捕获回归。

## 实施建议与注意事项

- 在将来扩展生成器的时候，任何允许 action 内继续读写输入的 shim（例如 `input()`, `unput()`）必须使用与主扫描器共享的游标或通过显式的同步协议（例如在 action 前/后同步游标）。
- 尽量避免 action 内直接操作主扫描器的内部状态（除非通过清晰的 API）。
- 在生成器文档中注明：动作可能 `return`，生成器需保证在动作执行前完成必要的状态更新或保底同步。

## 变更清单（供审计）

- `seulex/src/codegen/scanner_generator.cpp`:
  - 新增 `seulex_action_pos` 变量
  - 在 `yylex` 中，匹配后先设置 `seulex_pos = last_pos; seulex_action_pos = last_pos;`，执行 action 后 `seulex_pos = seulex_action_pos;`
  - `input()` shim 改为推进 `seulex_action_pos`。

（可在 PR 中附带生成的 scanner 源文件差异以便代码审计。）

## 结论

该修复通过将主游标推进前置并引入动作游标同步机制，解决了动作内 `return` 与动作内部读输入所造成的扫描位置不同步问题。已在 `c99` 示例上进行端到端验证，未再出现 parser 卡住现象。建议把针对注释和动作内 `input()` 的测试纳入回归测试套件，以防未来变更回退该修复。

----

作者: 自动生成（由仓库分析工具协助）
