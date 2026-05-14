# SeuLex 词法分析器卡住问题修复报告

## 现象

将 SeuLex 生成的词法分析器与 Bison 生成的语法分析器联用时，`yyparse()` 会在输入扫描阶段表现为卡住，通常是同一个 token 被重复返回，或者语法分析器一直等不到能推进状态的下一个 token。

## 根因

问题出在生成器输出的 `yylex()` 控制流，而不是 Bison 本身。

### 1. 动作执行前没有先推进主游标

原来的生成代码在匹配完成后先进入 `switch (last_accept)` 执行动作，最后才执行：

- `seulex_pos = last_pos;`

这在动作包含 `return TOKEN;` 时会直接出错，因为 `return` 会提前退出 `yylex()`，导致主游标根本没有更新。下一次 Bison 再调用 `yylex()` 时，扫描器仍然从同一位置开始，于是会重复返回同一个 token，表现为“卡住”。

### 2. 动作内的 `input()` 使用了独立游标

用户规则里存在类似 `comment();` 的动作，`comment()` 内部会调用 `input()` 消费字符。原先生成器提供的 `input()` 读的是独立变量，而不是当前 token 匹配后的扫描位置，这会让动作消费的字符和主扫描位置脱节，进一步造成扫描状态错乱。

## 修改内容

修改了 [src/codegen/scanner_generator.cpp](../src/codegen/scanner_generator.cpp) 中生成的运行时代码：

- 新增 `seulex_action_pos` 作为动作执行期间的辅助游标。
- 在执行动作前先同步 `seulex_pos = last_pos`，保证即使动作立刻 `return`，主扫描位置也已经前进。
- 将 `input()` 改为读取和推进 `seulex_action_pos`，使动作内的字符消费和当前 token 位置一致。
- 在动作正常结束后，再把 `seulex_pos` 同步到 `seulex_action_pos`，覆盖像注释处理这种会继续消费输入但不立即返回的动作。

## 影响

这次修复同时覆盖了两类典型规则：

- `return(...)` 型规则，不再因为提前返回而重复读取同一位置。
- `comment()` 这类会继续调用 `input()` 的动作，不再与主扫描位置脱节。

## 验证建议

建议重新生成 `c99.yy.c`，再用它与 Bison 生成的 `c99.tab.c` 一起编译并运行已有 `test/c99.l` / `test/c99.y` 组合，重点观察：

- token 是否能持续前进。
- 注释和空白规则是否能正确跳过。
- 语法分析是否不再在第一批 token 上停住。
