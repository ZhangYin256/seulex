#include "codegen/scanner_generator.h"

#include <fstream>
#include <iostream>

namespace seulex {

static void write_prologue(std::ostream &os, const SpecDocument &spec) {
  os << "/* 前置代码块 */\n";
  for (const auto &blk : spec.prologue) {
    os << blk.content << "\n";
  }
}

bool ScannerGenerator::generate(const MinDfa &min_dfa, const SpecDocument &spec,
                                const GeneratorConfig &config) const {
  std::ostream *ofs = &std::cout;
  std::ofstream ofs_file;
  if (!config.to_stdout) {
    ofs_file.open(config.output_filename, std::ios::out | std::ios::trunc);
    if (!ofs_file.is_open()) {
      std::cerr << "Failed to open output file: " << config.output_filename
                << std::endl;
      return false;
    }
    ofs = &ofs_file;
  }
  // yylex function: reads stdin on first call and returns next token
  (*ofs) << "#include <stddef.h>\n";
  (*ofs) << "#include <stdio.h>\n";
  (*ofs) << "#include <stdlib.h>\n";
  (*ofs) << "#include <string.h>\n";
  // 没有必要，c99.l文件里自带
  // (*ofs) << "#include \"y.tab.h\"\n\n";
  (*ofs) << "#ifndef SEULEX_YYTEXT_MAX\n#define SEULEX_YYTEXT_MAX 65536\n#endif\n";
  (*ofs) << "static char *seulex_buf = NULL;\n";
  (*ofs) << "static size_t seulex_n = 0;\n";
  (*ofs) << "static size_t seulex_pos = 0;\n";
  (*ofs) << "static char yytext[SEULEX_YYTEXT_MAX];\n";
  (*ofs) << "int yyleng = 0;\n";

  // 前置代码块
  (*ofs) << "\n/* Prologue code blocks */\n";
  write_prologue(*ofs, spec);
  // transition table
  (*ofs) << "static int transition_table[" << min_dfa.state_count
         << "][256] = {\n";
  for (int s = 0; s < min_dfa.state_count; ++s) {
    (*ofs) << "  { ";
    for (int c = 0; c < 256; ++c) {
      (*ofs) << min_dfa.transition_table[s][c];
      if (c + 1 < 256)
        (*ofs) << ",";
    }
    (*ofs) << " }";
    if (s + 1 < min_dfa.state_count)
      (*ofs) << ",\n";
    else
      (*ofs) << "\n";
  }
  (*ofs) << "};\n\n";

  // accept actions
  (*ofs) << "static int accept_action[" << min_dfa.state_count << "] = { ";
  for (int s = 0; s < min_dfa.state_count; ++s) {
    (*ofs) << min_dfa.accept_action[s];
    if (s + 1 < min_dfa.state_count)
      (*ofs) << ", ";
  }
  (*ofs) << " };\n\n";

  (*ofs) << "int yylex(void) {\n";
  (*ofs) << "  if (!seulex_buf) {\n";
  (*ofs) << "    size_t cap = 4096; seulex_buf = (char *)malloc(cap); if "
            "(!seulex_buf) return 0;\n";
  (*ofs) << "    int ch; while ((ch = fgetc(stdin)) != EOF) { if (seulex_n + 1 >= "
            "cap) { cap *= 2; char *tmp = (char *)realloc(seulex_buf, cap); if "
            "(!tmp) { free(seulex_buf); seulex_buf = NULL; return 0; } seulex_buf "
            "= tmp; } seulex_buf[seulex_n++] = (char)ch; }\n";
  (*ofs) << "    if (!seulex_buf) return 0;\n";
  (*ofs) << "  }\n";

  (*ofs) << "  while (seulex_pos < seulex_n) {\n";
  (*ofs) << "    int state = " << min_dfa.start_state << ";\n";
  (*ofs) << "    size_t cur = seulex_pos; int last_accept = -1; size_t last_pos = "
            "seulex_pos;\n";
  (*ofs) << "    while (cur < seulex_n) { unsigned char ch = (unsigned "
            "char)seulex_buf[cur]; int next = transition_table[state][ch]; if "
            "(next == -1) break; state = next; cur++; if (accept_action[state] != "
            "-1) { last_accept = accept_action[state]; last_pos = cur; } }\n";
  (*ofs) << "    if (last_accept == -1) { seulex_pos++; continue; }\n";
  (*ofs) << "    size_t matched_len = last_pos - seulex_pos;\n";
  (*ofs) << "    size_t copy_len = matched_len < (SEULEX_YYTEXT_MAX - 1) ? "
            "matched_len : (SEULEX_YYTEXT_MAX - 1);\n";
  (*ofs) << "    memcpy(yytext, seulex_buf + seulex_pos, copy_len); "
            "yytext[copy_len] = '\\0'; yyleng = (int)copy_len;\n";
  (*ofs) << "    seulex_pos = last_pos;\n";
  (*ofs) << "    // Execute action for matched token\n";
  (*ofs) << "    switch (last_accept) {\n";
  for (size_t i = 0; i < spec.action_codes.size(); ++i) {
    (*ofs) << "    case " << i << ": {\n";
    std::string code = spec.action_codes[i];
    if (!code.empty() && code.front() == '{' && code.back() == '}') {
      code = code.substr(1, code.size() - 2);
    }
    (*ofs) << code << "\n      break;\n    }\n";
    // If action didn't return, continue scanning; but many actions return token
    // values.
  }
  (*ofs) << "    default: break;\n";
  (*ofs) << "    }\n";
  (*ofs) << "  }\n";
  (*ofs) << "  return 0;\n";
  (*ofs) << "}\n";

  // optional standalone main for testing
  (*ofs) << "#ifdef SEULEX_STANDALONE\n";
  (*ofs) << "int main(int argc, char **argv) {\n";
  (*ofs) << "  int tok; while ((tok = yylex()) != 0) { printf(\"TOKEN %d\\n\", "
            "tok); } return 0; }\n";
  (*ofs) << "#endif\n";

  // Compatibility shims for embedded action code
  (*ofs) << "\n/* Compatibility shims for embedded action code */\n";
  (*ofs) << "#ifndef ECHO\n#define ECHO fwrite(yytext, 1, yyleng, "
            "stdout)\n#endif\n\n";
  (*ofs) << "static int seulex_input_pos = 0;\n";
  (*ofs) << "static int input(void) { if (seulex_input_pos < (int)seulex_n) "
            "return (unsigned char)seulex_buf[seulex_input_pos++]; return 0; "
            "}\n\n";
  // Append user code section
  if (!spec.user_code_section.empty()) {
    (*ofs) << "\n/* User code section */\n";
    (*ofs) << spec.user_code_section << "\n";
  }

  return true;
}

} // namespace seulex
