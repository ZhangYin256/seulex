#pragma once

#include "types.h"

#include <vector>

namespace seulex {

class DfaMinimizer {
public:
  MinDfa minimize(const std::vector<DfaState> &dfa_states,
                  int start_state) const;

private:
  void build_initial_partition(const std::vector<DfaState> &dfa_states);
  bool refine_partition(const std::vector<DfaState> &dfa_states);
  void rebuild_block_ids();

  std::vector<std::vector<int>> partition_;
  std::vector<int> block_id_;
};

} // namespace seulex