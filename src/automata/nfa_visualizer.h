#pragma once

#include "types.h"
#include <string>
#include <vector>

namespace seulex {

class NfaVisualizer {
public:
  static std::string to_mermaid(const std::vector<NfaState> &states,
                                int start_state);
};

} // namespace seulex
