#include "automata/nfa_visualizer.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace seulex {
namespace {

std::string char_token(unsigned char c) {
  switch (c) {
  case '\\':
    return "\\\\";
  case '"':
    return "\\\"";
  case '\n':
    return "\\n";
  case '\r':
    return "\\r";
  case '\t':
    return "\\t";
  case '\f':
    return "\\f";
  case '\v':
    return "\\v";
  default:
    break;
  }

  if (std::isprint(c)) {
    return std::string(1, static_cast<char>(c));
  }

  std::ostringstream oss;
  oss << "0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
      << static_cast<int>(c);
  return oss.str();
}

std::string compact_char_set(std::vector<unsigned char> chars) {
  if (chars.empty()) {
    return "";
  }

  std::sort(chars.begin(), chars.end());
  chars.erase(std::unique(chars.begin(), chars.end()), chars.end());

  std::vector<std::string> parts;
  size_t i = 0;
  while (i < chars.size()) {
    size_t j = i;
    while (j + 1 < chars.size() && chars[j + 1] == chars[j] + 1) {
      ++j;
    }

    if (j > i + 1) {
      parts.push_back(char_token(chars[i]) + "-" + char_token(chars[j]));
    } else if (j == i + 1) {
      parts.push_back(char_token(chars[i]));
      parts.push_back(char_token(chars[j]));
    } else {
      parts.push_back(char_token(chars[i]));
    }

    i = j + 1;
  }

  std::ostringstream oss;
  for (size_t k = 0; k < parts.size(); ++k) {
    if (k > 0) {
      oss << ",";
    }
    oss << parts[k];
  }
  return oss.str();
}

} // namespace

std::string NfaVisualizer::to_mermaid(const std::vector<NfaState> &states,
                                      int start_state) {
  std::ostringstream out;
  out << "graph TD\n";

  if (states.empty()) {
    out << "  empty[\"(empty NFA)\"]\n";
    return out.str();
  }

  out << "  start([start])\n";
  if (start_state >= 0 && start_state < static_cast<int>(states.size())) {
    out << "  start --> q" << start_state << "\n";
  }

  for (const auto &st : states) {
    if (st.is_accept) {
      out << "  q" << st.id << "(((" << st.id << ")))\n";
    } else {
      out << "  q" << st.id << "((" << st.id << "))\n";
    }
  }

  for (const auto &st : states) {
    std::set<int> eps_targets;
    std::map<int, std::vector<unsigned char>> char_targets;

    for (const auto &tr : st.transitions) {
      if (tr.epsilon) {
        eps_targets.insert(tr.target_id);
      } else {
        char_targets[tr.target_id].push_back(tr.input_char);
      }
    }

    for (int target : eps_targets) {
      out << "  q" << st.id << " -->|eps| q" << target << "\n";
    }

    for (const auto &[target, chars] : char_targets) {
      out << "  q" << st.id << " -->|" << compact_char_set(chars) << "| q"
          << target << "\n";
    }
  }

  out << "  classDef startState fill:#bbdefb,stroke:#1e88e5,stroke-width:2px\n";
  out << "  classDef acceptState "
         "fill:#c8e6c9,stroke:#2e7d32,stroke-width:2px\n";

  if (start_state >= 0 && start_state < static_cast<int>(states.size())) {
    out << "  class q" << start_state << " startState\n";
  }

  for (const auto &st : states) {
    if (st.is_accept) {
      out << "  class q" << st.id << " acceptState\n";
    }
  }

  return out.str();
}

} // namespace seulex
