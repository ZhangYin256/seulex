#include "codegen/scanner_generator.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace seulex {
namespace {

MinDfa make_min_dfa() {
  MinDfa dfa;
  dfa.state_count = 1;
  dfa.transition_table.resize(1);
  dfa.transition_table[0].fill(-1);
  dfa.accept_action = {-1};
  dfa.trailing_original_rule = {-1};
  dfa.trailing_length = {0};
  dfa.start_state = 0;
  dfa.start_state_bol = 0;
  return dfa;
}

TEST(ScannerGeneratorTest, WritesOutputFileWhenNotStdout) {
  ScannerGenerator generator;
  auto dfa = make_min_dfa();
  SpecDocument spec;

  auto output = std::filesystem::temp_directory_path() /
                std::filesystem::path("seulex_scanner_generator_test.yy.c");

  GeneratorConfig cfg;
  cfg.output_filename = output.string();
  cfg.to_stdout = false;

  ASSERT_TRUE(generator.generate(dfa, spec, cfg));
  ASSERT_TRUE(std::filesystem::exists(output));

  std::ifstream in(output);
  std::string content((std::istreambuf_iterator<char>(in)),
                      std::istreambuf_iterator<char>());
  EXPECT_NE(content.find("int yylex(void)"), std::string::npos);
  EXPECT_NE(content.find("transition_table"), std::string::npos);
}

TEST(ScannerGeneratorTest, WritesToStdoutWhenRequested) {
  ScannerGenerator generator;
  auto dfa = make_min_dfa();
  SpecDocument spec;

  GeneratorConfig cfg;
  cfg.to_stdout = true;

  testing::internal::CaptureStdout();
  ASSERT_TRUE(generator.generate(dfa, spec, cfg));
  std::string output = testing::internal::GetCapturedStdout();

  EXPECT_NE(output.find("#include <stdio.h>"), std::string::npos);
  EXPECT_NE(output.find("int yylex(void)"), std::string::npos);
}

} // namespace
} // namespace seulex
