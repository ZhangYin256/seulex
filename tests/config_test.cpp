#include "config/config.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace seulex {
namespace {

std::vector<char *> make_argv(std::vector<std::string> &args) {
  std::vector<char *> argv;
  argv.reserve(args.size());
  for (auto &arg : args) {
    argv.push_back(arg.data());
  }
  return argv;
}

std::filesystem::path make_temp_lex_file() {
  auto file = std::filesystem::temp_directory_path() /
              std::filesystem::path("seulex_config_test_input.l");
  std::ofstream os(file);
  os << "%%\n[a] { return 1; }\n%%\n";
  os.close();
  return file;
}

void parse_with_removed_option_and_exit(const std::string &input_path) {
  std::vector<std::string> args = {
      "seulex",
      "-L",
      input_path,
  };
  auto argv = make_argv(args);
  (void)Config::parse(static_cast<int>(argv.size()), argv.data());
  std::exit(0);
}

TEST(ConfigTest, ParsesBasicOptions) {
  auto input = make_temp_lex_file();

  std::vector<std::string> args = {
      "seulex", "-d", "-o", "out.yy.c", input.string(),
  };
  auto argv = make_argv(args);

  auto cfg = Config::parse(static_cast<int>(argv.size()), argv.data());
  EXPECT_TRUE(cfg.debug);
  EXPECT_FALSE(cfg.to_stdout);
  EXPECT_EQ(cfg.output_filename, "out.yy.c");
  EXPECT_EQ(cfg.input_filename, input.string());
}

TEST(ConfigTest, UsesStemAsDefaultOutputName) {
  auto input = make_temp_lex_file();

  std::vector<std::string> args = {
      "seulex",
      input.string(),
  };
  auto argv = make_argv(args);

  auto cfg = Config::parse(static_cast<int>(argv.size()), argv.data());
  EXPECT_EQ(cfg.output_filename, "seulex_config_test_input.yy.c");
}

TEST(ConfigTest, RejectsUnknownOption) {
  auto input = make_temp_lex_file();

  EXPECT_EXIT(parse_with_removed_option_and_exit(input.string()),
              ::testing::ExitedWithCode(1), "Unknown option");
}

} // namespace
} // namespace seulex
