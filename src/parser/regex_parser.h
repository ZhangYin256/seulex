#pragma once

#include "types.h"
#include <cstdio>
#include <format>
#include <functional>
#include <memory>
#include <string>

namespace seulex {

class RegexParser {
public:
  std::unique_ptr<RegexNode> parse(const std::string &pattern);

private:
  std::unique_ptr<RegexNode> parse_alternation();
  std::unique_ptr<RegexNode> parse_concatenation();
  std::unique_ptr<RegexNode> parse_quantifier();
  std::unique_ptr<RegexNode> parse_atom();
  std::unique_ptr<RegexNode> parse_char_class();
  std::unique_ptr<RegexNode> parse_group();

  unsigned char parse_escape();
  unsigned char parse_hex_escape();
  unsigned char parse_octal_escape();

  std::string pattern_;
  size_t pos_ = 0;

  char peek() const;
  char advance();
  bool at_end() const;
};

} // namespace seulex

namespace std {
template <> struct formatter<seulex::RegexNode> {
  constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

  auto format(const seulex::RegexNode &node, std::format_context &ctx) const {
    // recursive lambda to format nodes
    std::function<std::string(const seulex::RegexNode *)> fmt;
    auto esc = [](unsigned char c) -> std::string {
      switch (c) {
      case '\\':
        return "\\\\";
      case '"':
        return "\\\"";
      case '\n':
        return "\\n";
      case '\r':
        return "\\r";
      case '\t':
        return "\\t";
      default:
        if (std::isprint(c))
          return std::string(1, static_cast<char>(c));
        char buf[8];
        std::snprintf(buf, sizeof(buf), "\\x%02X", c);
        return std::string(buf);
      }
    };

    fmt = [&](const seulex::RegexNode *n) -> std::string {
      if (!n)
        return "(null)";
      using T = seulex::RegexNodeType;
      switch (n->type) {
      case T::Char: {
        return std::format("'{}'", esc(n->ch));
      }
      case T::CharClass: {
        std::string s = "[";
        if (n->char_class.negated)
          s += "^";
        bool first = true;
        for (const auto &r : *n->char_class.ranges) {
          if (!first)
            s += ",";
          first = false;
          if (r.low == r.high)
            s += esc(r.low);
          else
            s += std::format("{}-{}", esc(r.low), esc(r.high));
        }
        s += "]";
        return s;
      }
      case T::Concat:
        return std::format("({}{})", fmt(n->binary.left).c_str(),
                           fmt(n->binary.right).c_str());
      case T::Alter:
        return std::format("({}|{})", fmt(n->binary.left).c_str(),
                           fmt(n->binary.right).c_str());
      case T::Star:
        return std::format("({})*", fmt(n->child).c_str());
      case T::Plus:
        return std::format("({})+", fmt(n->child).c_str());
      case T::Option:
        return std::format("({})?", fmt(n->child).c_str());
      case T::Empty:
        return std::string("ε");
      }
      return std::string("<unknown>");
    };

    std::string out = fmt(&node);
    return std::format_to(ctx.out(), "{}", out);
  }
};
} // namespace std