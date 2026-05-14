#include "parser/lex_input_scanner.h"
#include "diagnostic/diagnostic.h"
#include <sstream>
namespace {
void update_brace_balance(const std::string &text, int &brace_count,
                          bool &in_single_quote, bool &in_double_quote,
                          bool &in_block_comment, bool &escape_next) {
  for (size_t i = 0; i < text.size(); ++i) {
    char c = text[i];
    char next = (i + 1 < text.size()) ? text[i + 1] : '\0';

    if (in_block_comment) {
      if (c == '*' && next == '/') {
        in_block_comment = false;
        ++i;
      }
      continue;
    }

    if (in_single_quote) {
      if (escape_next) {
        escape_next = false;
      } else if (c == '\\') {
        escape_next = true;
      } else if (c == '\'') {
        in_single_quote = false;
      }
      continue;
    }

    if (in_double_quote) {
      if (escape_next) {
        escape_next = false;
      } else if (c == '\\') {
        escape_next = true;
      } else if (c == '"') {
        in_double_quote = false;
      }
      continue;
    }

    if (c == '/' && next == '*') {
      in_block_comment = true;
      ++i;
      continue;
    }

    if (c == '/' && next == '/') {
      break;
    }

    if (c == '\'') {
      in_single_quote = true;
      escape_next = false;
      continue;
    }

    if (c == '"') {
      in_double_quote = true;
      escape_next = false;
      continue;
    }

    if (c == '{') {
      ++brace_count;
    } else if (c == '}') {
      --brace_count;
    }
  }
}

size_t find_rule_action_start(const std::string &line) {
  bool in_char_class = false;
  bool in_double_quote = false;
  bool escape_next = false;

  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];

    (void)c;
    (void)escape_next;
    (void)in_double_quote;
    (void)in_char_class;

    if (escape_next) {
      escape_next = false;
      continue;
    }

    if (c == '\\') {
      escape_next = true;
      continue;
    }

    if (in_double_quote) {
      if (c == '"')
        in_double_quote = false;
      continue;
    }

    if (in_char_class) {
      if (c == ']')
        in_char_class = false;
      continue;
    }

    if (c == '[') {
      in_char_class = true;
      continue;
    }

    if (c == '"') {
      in_double_quote = true;
      continue;
    }

    if (c == ' ' || c == '\t') {
      return i;
    }
  }

  return std::string::npos;
}
} // namespace
namespace seulex {
LexInputScanner::LexInputScanner(const std::string &filename)
    : filename_(filename), file_(filename) {
  if (!file_.is_open()) {
    throw LexError("Cannot open file: " + filename);
  }
}
SpecDocument LexInputScanner::parse() {
  parse_definition_section();
  parse_rules_section();
  parse_user_code_section();
  return std::move(doc_);
}

void LexInputScanner::parse_definition_section() {
  while (std::getline(file_, current_line_)) {
    line_number_++;
    if (!current_line_.empty() && current_line_.back() == '\r')
      current_line_.pop_back();

    // 跳过/删除定义区中的 C 风格多行注释：
    // - 如果注释在同一行闭合，则把注释段删除并继续处理该行；
    // - 如果注释跨多行，截断当前行并读取直到找到结束标记 "*/"。
    size_t cpos = 0;
    while ((cpos = current_line_.find("/*")) != std::string::npos) {
      size_t end = current_line_.find("*/", cpos + 2);
      if (end != std::string::npos) {
        // 注释在同一行结束，删除注释段并继续查找可能存在的下一个注释
        current_line_.erase(cpos, end - cpos + 2);
      } else {
        // 注释跨行：截断当前行注释开始处，并跳过直到结束标记
        current_line_.erase(cpos);
        read_until_end("*/");
        break;
      }
    }
    // 临时调试：可视化打印第 17 行，显示原始字符串和每个字节的十六进制
    // if (line_number_ == 17) {
    //   std::print("Line {}: \"{}\"\n", line_number_, current_line_);
    //   std::print("Bytes: ");
    //   for (unsigned char c : current_line_) {
    //     std::print("{:02x} ", static_cast<unsigned int>(c));
    //   }
    //   std::print("\n");
    // }
    // 如果处理后该行只包含空白，则跳过
    if (current_line_.find_first_not_of(" \t") == std::string::npos)
      continue;
    // 规则区开始
    if (current_line_ == "%%") {
      return;
    }
    // 重复的逻辑，没啥用
    // if (current_line_.empty())
    //   continue;

    if (current_line_.substr(0, 2) == "%{") {
      parse_include_block();
    } else {
      auto pos = current_line_.find_first_of(" \t");
      if (pos != std::string::npos) {
        std::string name = current_line_.substr(0, pos);
        std::string value = current_line_.substr(pos);

        auto start = value.find_first_not_of(" \t");
        if (start != std::string::npos) {
          value = value.substr(start);
        }

        if (!name.empty() && !value.empty()) {
          doc_.macros[name] = value;
        }
      }
    }
  }
}
void LexInputScanner::parse_rules_section() {
  int rule_number = 0;
  while (std::getline(file_, current_line_)) {
    line_number_++;
    if (!current_line_.empty() && current_line_.back() == '\r')
      current_line_.pop_back();

    // 规则区结束
    if (current_line_ == "%%") {
      return;
    }
    if (current_line_.find_first_not_of(" \t") == std::string::npos)
      continue;

    LexRule rule;
    rule.rule_number = rule_number++;
    size_t action_start = find_rule_action_start(current_line_);
    if (action_start != std::string::npos) {
      rule.regex_original = current_line_.substr(0, action_start);
      action_start = current_line_.find_first_not_of(" \t", action_start);
      if (action_start != std::string::npos) {
        std::string action = current_line_.substr(action_start);

        if (action.front() == '{') {
          int brace_count = 0;
          std::string full_action;
          bool in_single_quote = false;
          bool in_double_quote = false;
          bool in_block_comment = false;
          bool escape_next = false;

          full_action += action;
          update_brace_balance(action, brace_count, in_single_quote,
                               in_double_quote, in_block_comment, escape_next);

          while (brace_count > 0 && std::getline(file_, current_line_)) {
            line_number_++;
            if (!current_line_.empty() && current_line_.back() == '\r')
              current_line_.pop_back();
            full_action += "\n" + current_line_;
            update_brace_balance(current_line_, brace_count, in_single_quote,
                                 in_double_quote, in_block_comment,
                                 escape_next);
          }
          rule.action_index = static_cast<int>(doc_.action_codes.size());
          doc_.action_codes.push_back(full_action);
        } else {
          rule.action_index = static_cast<int>(doc_.action_codes.size());
          doc_.action_codes.push_back(action);
        }
      }
    }
    doc_.rules.push_back(std::move(rule));
  }
}
void LexInputScanner::parse_user_code_section() {
  std::ostringstream ss;
  while (std::getline(file_, current_line_)) {
    if (!current_line_.empty() && current_line_.back() == '\r')
      current_line_.pop_back();
    ss << current_line_ << "\n";
  }
  doc_.user_code_section = ss.str();
}
void LexInputScanner::parse_include_block() {
  std::string content;
  std::string line;

  while (std::getline(file_, line)) {
    line_number_++;
    if (line.find("%}") != std::string::npos) {
      break;
    }
    content += line + "\n";
  }

  doc_.prologue.push_back({content});
}
std::string LexInputScanner::read_until_end(const std::string &terminator) {
  std::string result;
  std::string line;
  while (std::getline(file_, line)) {
    line_number_++;
    if (line.find(terminator) != std::string::npos) {
      break;
    }
    result += line + "\n";
  }
  return result;
}

} // namespace seulex
