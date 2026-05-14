#include "automata/dfa_matcher.h"

namespace seulex {

DfaMatchResult
DfaMatcher::longest_prefix_match(const std::vector<DfaState> &dfa_states,
                                 int start_state, const std::string &input,
                                 std::size_t offset) const {
  DfaMatchResult result;

  if (start_state < 0 || start_state >= static_cast<int>(dfa_states.size())) {
    return result;
  }
  if (offset > input.size()) {
    return result;
  }

  int current_state = start_state;
  int last_accept_state = -1;
  std::size_t last_accept_pos = offset;

  if (dfa_states[current_state].is_accept) {
    last_accept_state = current_state;
    last_accept_pos = offset;
  }

  std::size_t pos = offset;
  while (pos < input.size()) {
    unsigned char c = static_cast<unsigned char>(input[pos]);
    int next = dfa_states[current_state].transitions[c];
    if (next == -1) {
      break;
    }

    current_state = next;
    ++pos;

    if (dfa_states[current_state].is_accept) {
      last_accept_state = current_state;
      last_accept_pos = pos;
    }
  }

  if (last_accept_state == -1) {
    return result;
  }

  result.matched = true;
  result.accept_state = last_accept_state;
  result.rule_index = dfa_states[last_accept_state].rule_index;
  result.action_index = dfa_states[last_accept_state].action_index;
  result.matched_length = last_accept_pos - offset;
  return result;
}

} // namespace seulex
