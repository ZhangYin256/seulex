#include "parser/flex_regex_converter.h"

#include "diagnostic/diagnostic.h"
#include <gtest/gtest.h>

namespace seulex {
namespace {

TEST(FlexRegexConverterTest, ExpandsMacroReference) {
  FlexRegexConverter converter;
  DefinitionTable macros;
  macros["DIGIT"] = "[0-9]";

  EXPECT_EQ(converter.convert("{DIGIT}+", macros), "([0-9])+");
}

TEST(FlexRegexConverterTest, EscapesQuotedLiteralMetacharacters) {
  FlexRegexConverter converter;
  DefinitionTable macros;

  EXPECT_EQ(converter.convert("\"a+b\"", macros), "a\\+b");
}

TEST(FlexRegexConverterTest, ThrowsOnCyclicMacroDefinition) {
  FlexRegexConverter converter;
  DefinitionTable macros;
  macros["A"] = "{B}";
  macros["B"] = "{A}";

  EXPECT_THROW(converter.convert("{A}", macros), LexError);
}

} // namespace
} // namespace seulex
