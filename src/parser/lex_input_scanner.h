#pragma once
#include "types.h"
#include <format>
#include <fstream>
#include <string>

namespace seulex {

class LexInputScanner {
public:
  explicit LexInputScanner(const std::string &filename);
  SpecDocument parse();

private:
  void parse_definition_section();
  void parse_rules_section();
  void parse_user_code_section();
  void parse_top_block();
  void parse_include_block();
  void parse_start_conditions();
  void parse_option();
  std::string read_until_end(const std::string &terminator);

  std::string filename_;
  std::ifstream file_;
  int line_number_ = 0;
  SpecDocument doc_;
  std::string current_line_;
};

} // namespace seulex

namespace std {

template <> struct formatter<seulex::SpecDocument> {
  constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

  auto format(const seulex::SpecDocument &doc, std::format_context &ctx) const {
    auto out = ctx.out();

    out = std::format_to(out, "SpecDocument:\n");

    out = std::format_to(out, "  Macros:\n");
    for (const auto &[name, value] : doc.macros) {
      out = std::format_to(out, "    {} => {}\n", name, value);
    }

    out = std::format_to(out, "  Rules:\n");
    for (const auto &rule : doc.rules) {
      out = std::format_to(out, "    Rule {}: /{}/ -> action #{}\n",
                           rule.rule_number, rule.regex_original,
                           rule.action_index);
      if (!rule.regex_normal.empty()) {
        out = std::format_to(out, "      Normalized regex: {}\n",
                             rule.regex_normal);
      }
      if (rule.ast) {
        out = std::format_to(out, "      AST: available\n");
      }
    }

    out = std::format_to(out, "  Top Blocks:\n{}\n", doc.top_blocks);
    out = std::format_to(out, "  Prologue:\n");
    for (const auto &block : doc.prologue) {
      out = std::format_to(out, "    ---\n{}", block.content);
      if (!block.content.empty() && block.content.back() != '\n') {
        out = std::format_to(out, "\n");
      }
    }

    out = std::format_to(out, "  Action Codes:\n");
    for (std::size_t i = 0; i < doc.action_codes.size(); ++i) {
      out = std::format_to(out, "    [{}]\n{}", i, doc.action_codes[i]);
      if (!doc.action_codes[i].empty() && doc.action_codes[i].back() != '\n') {
        out = std::format_to(out, "\n");
      }
    }
    out = std::format_to(out, "  User Code Section:\n{}\n",
                         doc.user_code_section);
    return out;
  }
};

} // namespace std