#include "parser/regex_parser.h"

#include "diagnostic/diagnostic.h"
#include <gtest/gtest.h>

namespace seulex {
namespace {

TEST(RegexParserTest, ParsesAlternationAndConcatenation) {
  RegexParser parser;
  auto ast = parser.parse("ab|c");

  ASSERT_NE(ast, nullptr);
  EXPECT_EQ(ast->type, RegexNodeType::Alter);
  ASSERT_NE(ast->binary.left, nullptr);
  ASSERT_NE(ast->binary.right, nullptr);
  EXPECT_EQ(ast->binary.right->type, RegexNodeType::Char);
}

TEST(RegexParserTest, ParsesCharClassRange) {
  RegexParser parser;
  auto ast = parser.parse("[a-c]");

  ASSERT_NE(ast, nullptr);
  ASSERT_EQ(ast->type, RegexNodeType::CharClass);
  ASSERT_NE(ast->char_class.ranges, nullptr);
  ASSERT_EQ(ast->char_class.ranges->size(), 1u);
  EXPECT_EQ((*ast->char_class.ranges)[0].low, static_cast<unsigned char>('a'));
  EXPECT_EQ((*ast->char_class.ranges)[0].high, static_cast<unsigned char>('c'));
}

TEST(RegexParserTest, ThrowsOnDanglingQuantifier) {
  RegexParser parser;
  EXPECT_THROW(parser.parse("*a"), LexError);
}

} // namespace
} // namespace seulex
