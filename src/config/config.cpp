#include "config/config.h"
#include <cstring>
#include <filesystem>
#include <iostream>

namespace seulex {

void Config::print_usage(const char *prog_name) {
  std::cerr << "Usage: " << prog_name << " [OPTIONS] FILE\n"
            << "Options:\n"
            << "  -o FILE   Write output to FILE\n"
            << "  -t        Write generated code to stdout\n"
            << "  -d        Generate debug output\n"
            << "  -h        Show this help\n";
}

GeneratorConfig Config::parse(int argc, char *argv[]) {
  GeneratorConfig config;
  config.output_filename = "lex.yy.c";

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "-o") == 0) {
      if (i + 1 < argc) {
        config.output_filename = argv[++i];
      } else {
        std::cerr << "Error: -o requires a filename\n";
        exit(1);
      }
    } else if (strcmp(argv[i], "-t") == 0) {
      config.to_stdout = true;
    } else if (strcmp(argv[i], "-d") == 0) {
      config.debug = true;
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      exit(0);
    } else if (argv[i][0] == '-') {
      std::cerr << "Error: Unknown option: " << argv[i] << "\n";
      print_usage(argv[0]);
      exit(1);
    } else {
      if (config.input_filename.empty()) {
        config.input_filename = argv[i];
      } else {
        std::cerr << "Error: Multiple input files not supported\n";
        exit(1);
      }
    }
  }

  if (config.input_filename.empty()) {
    std::cerr << "Error: No input file specified\n";
    print_usage(argv[0]);
    exit(1);
  }

  if (!std::filesystem::exists(config.input_filename)) {
    std::cerr << "Error: Input file not found: " << config.input_filename
              << "\n";
    exit(1);
  }

  if (!config.to_stdout && config.output_filename == "lex.yy.c") {
    std::filesystem::path input_path(config.input_filename);
    std::string stem = input_path.stem().string();
    config.output_filename = stem + ".yy.c";
  }

  return config;
}

std::ostream &operator<<(std::ostream &os, const GeneratorConfig &gc) {
  os << "GeneratorConfig {\n"
     << "  input_filename: " << gc.input_filename << "\n"
     << "  output_filename: " << gc.output_filename << "\n"
     << "  to_stdout: " << (gc.to_stdout ? "true" : "false") << "\n"
     << "  debug: " << (gc.debug ? "true" : "false") << "\n"
     << "}";
  return os;
}
} // namespace seulex
