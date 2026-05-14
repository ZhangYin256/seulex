#include "automata/dfa_matcher.h"
#include "automata/dfa_minimizer.h"
#include "automata/nfa_builder.h"
#include "automata/subset_construction.h"
#include "parser/regex_parser.h"

#include <gtest/gtest.h>

namespace seulex {
namespace {

TEST(AutomataPipelineTest, BuildsAndMatchesSimpleRegex) {
  RegexParser parser;
  auto ast = parser.parse("ab");
  ASSERT_NE(ast, nullptr);

  NfaBuilder builder;
  auto frag = builder.build_from_ast(ast.get());
  auto &states = builder.states();
  states[frag.accept_id].is_accept = true;
  states[frag.accept_id].rule_index = 0;
  states[frag.accept_id].action_index = 0;

  int unified_start = builder.append_unified_entry_state({frag.start_id});

  SubsetConstruction subset;
  auto dfa = subset.convert(states, unified_start);
  ASSERT_FALSE(dfa.empty());

  DfaMatcher matcher;
  auto result = matcher.longest_prefix_match(dfa, 0, "abx");
  EXPECT_TRUE(result.matched);
  EXPECT_EQ(result.matched_length, 2u);
  EXPECT_EQ(result.rule_index, 0);
  EXPECT_EQ(result.action_index, 0);

  DfaMinimizer minimizer;
  auto min_dfa = minimizer.minimize(dfa, 0);
  EXPECT_GT(min_dfa.state_count, 0);
  EXPECT_GE(min_dfa.start_state, 0);
}

} // namespace
} // namespace seulex
