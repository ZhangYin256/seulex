#pragma once

#include "types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace seulex {

struct DfaMatchResult {
  bool matched = false;
  int rule_index = -1;
  int action_index = -1;
  int accept_state = -1;
  std::size_t matched_length = 0;
};

class DfaMatcher {
public:
  DfaMatchResult longest_prefix_match(const std::vector<DfaState> &dfa_states,
                                      int start_state, const std::string &input,
                                      std::size_t offset = 0) const;
};

} // namespace seulex
