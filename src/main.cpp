#include "automata/dfa_minimizer.h"
#include "automata/nfa_builder.h"
#include "automata/nfa_visualizer.h"
#include "automata/subset_construction.h"
#include "codegen/scanner_generator.h"
#include "config/config.h"
#include "parser/flex_regex_converter.h"
#include "parser/lex_input_scanner.h"
#include "parser/regex_parser.h"
#include <iostream>
using namespace seulex;
int main(int argc, char *argv[]) {
  auto config = Config::parse(argc, argv);
  LexInputScanner scanner(config.input_filename);
  auto spec = scanner.parse();
  // std::cerr << "加载了" << spec.macros.size() << "个宏定义\n";
  // std::cerr << "加载了" << spec.rules.size() << "条规则\n";
  // std::cerr << "加载了" << spec.prologue.size() << "组前置代码\n";
  // std::cerr << "用户代码区大小: " << spec.user_code_section.size() << "\n";

  FlexRegexConverter converter;
  RegexParser parser;
  NfaBuilder nfa_builder;
  std::vector<int> rule_starts;

  // 处理每条规则的正则表达式，构建 AST，并生成对应的 NFA 片段
  for (auto &rule : spec.rules) {
    // 替换宏
    rule.regex_normal = converter.convert(rule.regex_original, spec.macros);
    // 构造抽象语法树（AST）
    rule.ast = parser.parse(rule.regex_normal);
    if (rule.ast) {
      std::cout << std::format("  AST: {}\n", *rule.ast);
    }
    // 根据 AST 构建 NFA 片段
    auto frag = nfa_builder.build_from_ast(rule.ast.get());
    rule_starts.push_back(frag.start_id);
    auto &states = nfa_builder.states();
    // 标记接受状态
    states[frag.accept_id].is_accept = true;
    states[frag.accept_id].rule_index = rule.rule_number;
    states[frag.accept_id].action_index = rule.action_index;
    std::cerr << "Rule " << rule.rule_number << ": " << rule.regex_original
              << " => " << rule.regex_normal << "\n";
  }

  // 将所有规则的 NFA 片段连接起来，形成一个大的 NFA
  int unified_entry_state = nfa_builder.append_unified_entry_state(rule_starts);
  // std::cerr << "NFA状态数: " << nfa_builder.states().size() << "\n";
  // std::cerr << "NFA统一入口状态: " << unified_entry_state << "\n";

  SubsetConstruction subset;
  auto dfa_states = subset.convert(nfa_builder.states(), unified_entry_state);
  // std::cerr << "DFA状态数: " << dfa_states.size() << "\n";

  DfaMinimizer minimizer;
  auto min_dfa = minimizer.minimize(dfa_states, dfa_states.empty() ? -1 : 0);
  // std::cerr << "最小化后 DFA状态数: " << min_dfa.state_count << "\n";

  // Generate scanner source
  ScannerGenerator generator;
  if (!generator.generate(min_dfa, spec, config)) {
    std::cerr << "Code generation failed\n";
    return 2;
  }

  if (config.debug) {
    std::cout << "--- MERMAID NFA BEGIN ---\n";
    std::cout << NfaVisualizer::to_mermaid(nfa_builder.states(),
                                           unified_entry_state);
    std::cout << "--- MERMAID NFA END ---\n";
  }

  return 0;
}