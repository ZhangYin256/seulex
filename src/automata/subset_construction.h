#pragma once

#include "types.h"

#include <map>
#include <set>
#include <vector>

namespace seulex {

class SubsetConstruction {
public:
  std::vector<DfaState> convert(const std::vector<NfaState> &nfa_states,
                                int nfa_start) const;

private:
  std::set<int> epsilon_closure(const std::set<int> &states,
                                const std::vector<NfaState> &nfa_states) const;
  std::set<int> move(const std::set<int> &states, unsigned char c,
                     const std::vector<NfaState> &nfa_states) const;

  int get_or_create_state(const std::set<int> &nfa_set,
                          std::vector<DfaState> &dfa_states,
                          std::map<std::set<int>, int> &state_map,
                          const std::vector<NfaState> &nfa_states) const;

  bool resolve_accept_metadata(const std::set<int> &nfa_set,
                               const std::vector<NfaState> &nfa_states,
                               int &rule_index, int &action_index) const;
};

} // namespace seulex
