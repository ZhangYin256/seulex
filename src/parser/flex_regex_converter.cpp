#include "parser/flex_regex_converter.h"

#include "diagnostic/diagnostic.h"
#include <cctype>
#include <unordered_set>

namespace seulex {
namespace {
bool is_regex_meta(char c) {
  switch (c) {
  case '.':
  case '*':
  case '+':
  case '?':
  case '(':
  case ')':
  case '[':
  case ']':
  case '{':
  case '}':
  case '|':
  case '^':
  case '$':
  case '\\':
    return true;
  default:
    return false;
  }
}
} // namespace

std::string FlexRegexConverter::convert(const std::string &flex_regex,
                                        const DefinitionTable &macros) {
  std::unordered_set<std::string> visiting;
  return expand_text(flex_regex, macros, visiting);
}

std::string
FlexRegexConverter::expand_text(const std::string &input,
                                const DefinitionTable &macros,
                                std::unordered_set<std::string> &visiting) {
  std::string result;
  size_t i = 0;

  while (i < input.size()) {
    char c = input[i];

    if (c == '{') {
      size_t end = i + 1;
      while (end < input.size() && input[end] != '}') {
        ++end;
      }
      if (end < input.size()) {
        std::string name = input.substr(i + 1, end - i - 1);
        if (is_macro_name(name)) {
          result += expand_macro_reference(name, macros, visiting);
        } else {
          result += input.substr(i, end - i + 1);
        }
        i = end + 1;
        continue;
      }
    }

    if (c == '[') {
      result += read_char_class(input, i);
      continue;
    }

    if (c == '"') {
      result += read_quoted_literal(input, i);
      continue;
    }

    if (c == '\\') {
      if (i + 1 < input.size()) {
        result += input.substr(i, 2);
        i += 2;
      } else {
        result += c;
        ++i;
      }
      continue;
    }

    result += c;
    ++i;
  }

  return result;
}

std::string FlexRegexConverter::expand_macro_reference(
    const std::string &name, const DefinitionTable &macros,
    std::unordered_set<std::string> &visiting) {
  auto it = macros.find(name);
  if (it == macros.end()) {
    return "{" + name + "}";
  }

  if (!visiting.insert(name).second) {
    throw LexError("Cyclic macro definition detected: " + name);
  }

  std::string expanded = expand_text(it->second, macros, visiting);
  visiting.erase(name);
  return "(" + expanded + ")";
}

std::string FlexRegexConverter::read_quoted_literal(const std::string &input,
                                                    size_t &pos) {
  std::string literal;
  ++pos; // consume opening quote

  while (pos < input.size()) {
    char c = input[pos++];
    if (c == '"') {
      break;
    }
    if (c == '\\' && pos < input.size()) {
      literal += input[pos - 1];
      literal += input[pos++];
      continue;
    }
    literal += c;
  }

  return escape_literal(literal);
}

std::string FlexRegexConverter::read_char_class(const std::string &input,
                                                size_t &pos) {
  std::string result;
  result += input[pos++]; // consume '['

  bool escaped = false;
  while (pos < input.size()) {
    char c = input[pos++];
    result += c;
    if (escaped) {
      escaped = false;
      continue;
    }
    if (c == '\\') {
      escaped = true;
      continue;
    }
    if (c == ']') {
      break;
    }
  }

  return result;
}

std::string
FlexRegexConverter::escape_literal(const std::string &literal) const {
  std::string result;
  result.reserve(literal.size() * 2);
  for (char c : literal) {
    if (is_regex_meta(c)) {
      result += '\\';
    }
    result += c;
  }
  return result;
}

bool FlexRegexConverter::is_macro_name(const std::string &name) {
  if (name.empty() || (!(name[0] == '_' ||
                         std::isalpha(static_cast<unsigned char>(name[0]))))) {
    return false;
  }
  for (size_t i = 1; i < name.size(); ++i) {
    unsigned char c = static_cast<unsigned char>(name[i]);
    if (!(std::isalnum(c) || c == '_')) {
      return false;
    }
  }
  return true;
}

} // namespace seulex