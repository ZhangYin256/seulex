#include "automata/nfa_builder.h"

#include <array>

namespace seulex {

void NfaBuilder::reset() { states_.clear(); }

NfaFragment NfaBuilder::build_from_ast(const RegexNode *root) {
  return build_node(root);
}

int NfaBuilder::append_unified_entry_state(
    const std::vector<int> &rule_start_ids) {
  int unified_entry_state = create_state();
  for (int id : rule_start_ids) {
    add_epsilon(unified_entry_state, id);
  }
  return unified_entry_state;
}

int NfaBuilder::create_state() {
  int id = static_cast<int>(states_.size());
  NfaState state;
  state.id = id;
  states_.push_back(std::move(state));
  return id;
}

void NfaBuilder::add_epsilon(int from, int to) {
  states_[from].transitions.push_back({true, 0, to});
}

void NfaBuilder::add_char(int from, int to, unsigned char ch) {
  states_[from].transitions.push_back({false, ch, to});
}

NfaFragment NfaBuilder::build_char_class(const RegexNode *node) {
  int s = create_state();
  int a = create_state();

  std::array<bool, 256> allow{};
  allow.fill(!node->char_class.negated);

  if (node->char_class.negated) {
    allow.fill(true);
    for (const auto &r : *node->char_class.ranges) {
      for (int c = r.low; c <= r.high; ++c) {
        allow[static_cast<unsigned char>(c)] = false;
      }
    }
  } else {
    allow.fill(false);
    for (const auto &r : *node->char_class.ranges) {
      for (int c = r.low; c <= r.high; ++c) {
        allow[static_cast<unsigned char>(c)] = true;
      }
    }
  }

  for (int c = 0; c < 256; ++c) {
    if (allow[static_cast<unsigned char>(c)]) {
      add_char(s, a, static_cast<unsigned char>(c));
    }
  }

  return {s, a};
}

NfaFragment NfaBuilder::build_node(const RegexNode *node) {
  if (node == nullptr || node->type == RegexNodeType::Empty) {
    int s = create_state();
    int a = create_state();
    add_epsilon(s, a);
    return {s, a};
  }

  switch (node->type) {
  case RegexNodeType::Char: {
    int s = create_state();
    int a = create_state();
    add_char(s, a, node->ch);
    return {s, a};
  }
  case RegexNodeType::CharClass:
    return build_char_class(node);
  case RegexNodeType::Concat: {
    NfaFragment left = build_node(node->binary.left);
    NfaFragment right = build_node(node->binary.right);
    add_epsilon(left.accept_id, right.start_id);
    return {left.start_id, right.accept_id};
  }
  case RegexNodeType::Alter: {
    NfaFragment left = build_node(node->binary.left);
    NfaFragment right = build_node(node->binary.right);
    int s = create_state();
    int a = create_state();
    add_epsilon(s, left.start_id);
    add_epsilon(s, right.start_id);
    add_epsilon(left.accept_id, a);
    add_epsilon(right.accept_id, a);
    return {s, a};
  }
  case RegexNodeType::Star: {
    NfaFragment child = build_node(node->child);
    int s = create_state();
    int a = create_state();
    add_epsilon(s, child.start_id);
    add_epsilon(s, a);
    add_epsilon(child.accept_id, child.start_id);
    add_epsilon(child.accept_id, a);
    return {s, a};
  }
  case RegexNodeType::Plus: {
    NfaFragment child = build_node(node->child);
    int s = create_state();
    int a = create_state();
    add_epsilon(s, child.start_id);
    add_epsilon(child.accept_id, child.start_id);
    add_epsilon(child.accept_id, a);
    return {s, a};
  }
  case RegexNodeType::Option: {
    NfaFragment child = build_node(node->child);
    int s = create_state();
    int a = create_state();
    add_epsilon(s, child.start_id);
    add_epsilon(s, a);
    add_epsilon(child.accept_id, a);
    return {s, a};
  }
  case RegexNodeType::Empty:
    break;
  }

  int s = create_state();
  int a = create_state();
  add_epsilon(s, a);
  return {s, a};
}

} // namespace seulex
