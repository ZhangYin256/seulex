#pragma once

#include "types.h"
#include <string>
#include <unordered_set>

namespace seulex {

class FlexRegexConverter {
public:
  std::string convert(const std::string &flex_regex,
                      const DefinitionTable &macros);

private:
  std::string expand_text(const std::string &input,
                          const DefinitionTable &macros,
                          std::unordered_set<std::string> &visiting);
  std::string expand_macro_reference(const std::string &name,
                                     const DefinitionTable &macros,
                                     std::unordered_set<std::string> &visiting);
  std::string read_quoted_literal(const std::string &input, size_t &pos);
  std::string read_char_class(const std::string &input, size_t &pos);
  std::string escape_literal(const std::string &literal) const;
  static bool is_macro_name(const std::string &name);
};

} // namespace seulex