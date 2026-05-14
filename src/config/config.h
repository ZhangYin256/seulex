#pragma once
#include <format>
#include "types.h"

namespace seulex {

class Config {
public:
  static GeneratorConfig parse(int argc, char *argv[]);
  static void print_usage(const char *prog_name);
};

} // namespace seulex

namespace std {
// 为 GeneratorConfig 定义一个格式化器，方便调试输出
template <> struct formatter<seulex::GeneratorConfig> {
  // parse: 什么都不做，直接返回起始迭代器
  constexpr auto parse(std::format_parse_context &ctx) {
    return ctx.begin(); // 表示不解析任何格式说明符
  }

  // format: 按你的喜好格式化
  auto format(const seulex::GeneratorConfig &cfg,
              std::format_context &ctx) const {
    return std::format_to(ctx.out(),
                          "GeneratorConfig:\n"
                          "  input_filename    = \"{}\"\n"
                          "  output_filename   = \"{}\"\n"
                          "  to_stdout         = {}\n"
                          "  debug             = {}\n",
                          cfg.input_filename, cfg.output_filename,
                          cfg.to_stdout, cfg.debug);
  }
};
} // namespace std