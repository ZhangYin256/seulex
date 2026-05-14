#pragma once

#include "types.h"
#include <vector>

namespace seulex {

struct NfaFragment {
  int start_id = -1;
  int accept_id = -1;
};

class NfaBuilder {
public:
  void reset();
  NfaFragment build_from_ast(const RegexNode *root);
  int append_unified_entry_state(const std::vector<int> &rule_start_ids);

  const std::vector<NfaState> &states() const { return states_; }
  std::vector<NfaState> &states() { return states_; }

private:
  int create_state();
  void add_epsilon(int from, int to);
  void add_char(int from, int to, unsigned char ch);
  NfaFragment build_node(const RegexNode *node);
  NfaFragment build_char_class(const RegexNode *node);

  std::vector<NfaState> states_;
};

} // namespace seulex
