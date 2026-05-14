#pragma once

#include "types.h"

namespace seulex {

class ScannerGenerator {
public:
  // Generate a C scanner source file at output_path. Return true on success.
  bool generate(const MinDfa &min_dfa, const SpecDocument &spec,
                const GeneratorConfig &config) const;
};

} // namespace seulex
