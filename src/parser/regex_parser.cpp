#include "parser/regex_parser.h"

#include "diagnostic/diagnostic.h"
#include <cctype>
#include <cstring>

namespace seulex {

// 解析规则
std::unique_ptr<RegexNode> RegexParser::parse(const std::string &pattern) {
  pattern_ = pattern;
  pos_ = 0;
  auto result = parse_alternation();
  if (!at_end()) {
    throw LexError("Unexpected character in regex: " + std::string(1, peek()));
  }
  return result;
}

// 解析 | 操作符
std::unique_ptr<RegexNode> RegexParser::parse_alternation() {
  auto left = parse_concatenation();

  while (!at_end() && peek() == '|') {
    advance();
    auto right = parse_concatenation();
    auto node = std::make_unique<RegexNode>(RegexNodeType::Alter);
    node->binary.left = left.release();
    node->binary.right = right.release();
    left = std::move(node);
  }

  return left;
}

std::unique_ptr<RegexNode> RegexParser::parse_concatenation() {
  auto left = parse_quantifier();

  while (!at_end() && peek() != '|' && peek() != ')') {
    auto right = parse_quantifier();
    auto node = std::make_unique<RegexNode>(RegexNodeType::Concat);
    node->binary.left = left.release();
    node->binary.right = right.release();
    left = std::move(node);
  }

  return left;
}

std::unique_ptr<RegexNode> RegexParser::parse_quantifier() {
  auto atom = parse_atom();

  if (!at_end()) {
    if (peek() == '*') {
      advance();
      auto node = std::make_unique<RegexNode>(RegexNodeType::Star);
      node->child = atom.release();
      return node;
    } else if (peek() == '+') {
      advance();
      auto node = std::make_unique<RegexNode>(RegexNodeType::Plus);
      node->child = atom.release();
      return node;
    } else if (peek() == '?') {
      advance();
      auto node = std::make_unique<RegexNode>(RegexNodeType::Option);
      node->child = atom.release();
      return node;
    }
  }

  return atom;
}

std::unique_ptr<RegexNode> RegexParser::parse_atom() {
  if (at_end()) {
    return std::make_unique<RegexNode>(RegexNodeType::Empty);
  }

  char c = peek();

  if (c == '(') {
    return parse_group();
  }
  if (c == '[') {
    return parse_char_class();
  }
  if (c == '.') {
    advance();
    auto node = std::make_unique<RegexNode>(RegexNodeType::CharClass);
    node->char_class.negated = true;
    node->char_class.ranges = new std::vector<Range>();
    node->char_class.ranges->push_back({10, 10});
    return node;
  }
  if (c == '\\') {
    advance();
    unsigned char ch = parse_escape();
    auto node = std::make_unique<RegexNode>(RegexNodeType::Char);
    node->ch = ch;
    return node;
  }
  if (c == '*' || c == '+' || c == '?') {
    throw LexError("Quantifier without preceding element");
  }

  advance();
  auto node = std::make_unique<RegexNode>(RegexNodeType::Char);
  node->ch = static_cast<unsigned char>(c);
  return node;
}

std::unique_ptr<RegexNode> RegexParser::parse_char_class() {
  advance(); // consume '['

  auto node = std::make_unique<RegexNode>(RegexNodeType::CharClass);
  node->char_class.ranges = new std::vector<Range>();

  if (!at_end() && peek() == '^') {
    node->char_class.negated = true;
    advance();
  } else {
    node->char_class.negated = false;
  }

  bool first = true;
  while (!at_end() && (first || peek() != ']')) {
    first = false;

    unsigned char low;
    if (peek() == '\\') {
      advance();
      low = parse_escape();
    } else {
      low = static_cast<unsigned char>(advance());
    }

    if (!at_end() && peek() == '-' && pos_ + 1 < pattern_.size() &&
        pattern_[pos_ + 1] != ']') {
      advance();
      unsigned char high;
      if (peek() == '\\') {
        advance();
        high = parse_escape();
      } else {
        high = static_cast<unsigned char>(advance());
      }
      node->char_class.ranges->push_back({low, high});
    } else {
      node->char_class.ranges->push_back({low, low});
    }
  }

  if (!at_end()) {
    advance();
  }

  return node;
}

std::unique_ptr<RegexNode> RegexParser::parse_group() {
  advance();
  auto inner = parse_alternation();
  if (!at_end() && peek() == ')') {
    advance();
  }
  return inner;
}

unsigned char RegexParser::parse_escape() {
  if (at_end()) {
    throw LexError("Unexpected end of regex in escape sequence");
  }

  char c = advance();
  switch (c) {
  case 'n':
    return '\n';
  case 't':
    return '\t';
  case 'r':
    return '\r';
  case 'f':
    return '\f';
  case 'v':
    return '\v';
  case 'a':
    return '\a';
  case 'b':
    return '\b';
  case '0':
    return '\0';
  case 'x':
    return parse_hex_escape();
  default:
    if (c >= '0' && c <= '7') {
      pos_--;
      return parse_octal_escape();
    }
    return static_cast<unsigned char>(c);
  }
}

unsigned char RegexParser::parse_hex_escape() {
  std::string hex;
  while (!at_end() && std::isxdigit(static_cast<unsigned char>(peek()))) {
    hex += advance();
  }
  if (hex.empty()) {
    throw LexError("Invalid hex escape sequence");
  }
  return static_cast<unsigned char>(std::stoul(hex, nullptr, 16));
}

unsigned char RegexParser::parse_octal_escape() {
  std::string oct;
  while (!at_end() && peek() >= '0' && peek() <= '7' && oct.size() < 3) {
    oct += advance();
  }
  if (oct.empty()) {
    throw LexError("Invalid octal escape sequence");
  }
  return static_cast<unsigned char>(std::stoul(oct, nullptr, 8));
}

char RegexParser::peek() const { return pattern_[pos_]; }

char RegexParser::advance() { return pattern_[pos_++]; }

bool RegexParser::at_end() const { return pos_ >= pattern_.size(); }

RegexNode::RegexNode(RegexNodeType t) : type(t) { memset(&ch, 0, sizeof(ch)); }

RegexNode::~RegexNode() {
  switch (type) {
  case RegexNodeType::CharClass:
    delete char_class.ranges;
    break;
  case RegexNodeType::Concat:
  case RegexNodeType::Alter:
    delete binary.left;
    delete binary.right;
    break;
  case RegexNodeType::Star:
  case RegexNodeType::Plus:
  case RegexNodeType::Option:
    delete child;
    break;
  default:
    break;
  }
}

} // namespace seulex