#include "automata/subset_construction.h"

#include <queue>

namespace seulex {

std::vector<DfaState>
SubsetConstruction::convert(const std::vector<NfaState> &nfa_states,
                            int nfa_start) const {
  std::vector<DfaState> dfa_states;
  if (nfa_start < 0 || nfa_start >= static_cast<int>(nfa_states.size())) {
    return dfa_states;
  }

  // NFA 状态集合到 DFA 状态 ID 的映射
  std::map<std::set<int>, int> state_map;
  // 工作队列，存储待处理的 NFA 状态集合
  std::queue<std::set<int>> worklist;

  std::set<int> start_set = {nfa_start};
  std::set<int> start_closure = epsilon_closure(start_set, nfa_states);
  get_or_create_state(start_closure, dfa_states, state_map, nfa_states);
  worklist.push(start_closure);

  while (!worklist.empty()) {
    std::set<int> current = worklist.front();
    worklist.pop();

    int current_id = state_map[current];
    for (int c = 0; c < 256; ++c) {
      std::set<int> next =
          move(current, static_cast<unsigned char>(c), nfa_states);
      if (next.empty()) {
        continue;
      }

      std::set<int> next_closure = epsilon_closure(next, nfa_states);
      if (state_map.find(next_closure) == state_map.end()) {
        get_or_create_state(next_closure, dfa_states, state_map, nfa_states);
        worklist.push(next_closure);
      }

      dfa_states[current_id].transitions[c] = state_map[next_closure];
    }
  }

  return dfa_states;
}

std::set<int> SubsetConstruction::epsilon_closure(
    const std::set<int> &states,
    const std::vector<NfaState> &nfa_states) const {
  std::set<int> closure = states;
  std::queue<int> worklist;

  for (int s : states) {
    worklist.push(s);
  }

  while (!worklist.empty()) {
    int current = worklist.front();
    worklist.pop();

    if (current < 0 || current >= static_cast<int>(nfa_states.size())) {
      continue;
    }

    // 遍历当前状态的所有转移，寻找 epsilon 转移
    for (const auto &tr : nfa_states[current].transitions) {
      if (!tr.epsilon) {
        continue;
      }
      // second是一个bool，真表示插入成功（即之前不存在该元素）
      if (closure.insert(tr.target_id).second) {
        worklist.push(tr.target_id);
      }
    }
  }

  return closure;
}

std::set<int>
SubsetConstruction::move(const std::set<int> &states, unsigned char c,
                         const std::vector<NfaState> &nfa_states) const {
  std::set<int> result;
  for (int s : states) {
    if (s < 0 || s >= static_cast<int>(nfa_states.size())) {
      continue;
    }
    for (const auto &tr : nfa_states[s].transitions) {
      if (!tr.epsilon && tr.input_char == c) {
        result.insert(tr.target_id);
      }
    }
  }
  return result;
}

int SubsetConstruction::get_or_create_state(
    const std::set<int> &nfa_set, std::vector<DfaState> &dfa_states,
    std::map<std::set<int>, int> &state_map,
    const std::vector<NfaState> &nfa_states) const {
  auto it = state_map.find(nfa_set);
  if (it != state_map.end()) {
    return it->second;
  }

  DfaState dfa;
  dfa.id = static_cast<int>(dfa_states.size());
  dfa.nfa_state_ids.assign(nfa_set.begin(), nfa_set.end());
  dfa.transitions.fill(-1);

  int rule_index = -1;
  int action_index = -1;
  if (resolve_accept_metadata(nfa_set, nfa_states, rule_index, action_index)) {
    dfa.is_accept = true;
    dfa.rule_index = rule_index;
    dfa.action_index = action_index;
  }

  dfa_states.push_back(std::move(dfa));
  state_map[nfa_set] = dfa_states.back().id;
  return dfa_states.back().id;
}

// 根据 NFA 状态集合确定对应的 DFA 接受状态元数据（规则索引和动作索引）
// 返回 true 表示该集合对应一个接受状态，false 表示非接受状态
bool SubsetConstruction::resolve_accept_metadata(
    const std::set<int> &nfa_set, const std::vector<NfaState> &nfa_states,
    int &rule_index, int &action_index) const {
  // Flex tie-break rule for same matched length: choose the earliest rule
  // in the Lex file (smallest rule_index).
  int min_rule = -1;
  int selected_action = -1;

  for (int s : nfa_set) {
    if (s < 0 || s >= static_cast<int>(nfa_states.size())) {
      continue;
    }

    const auto &nfa_state = nfa_states[s];
    if (!nfa_state.is_accept) {
      continue;
    }

    if (min_rule == -1 || nfa_state.rule_index < min_rule) {
      min_rule = nfa_state.rule_index;
      selected_action = nfa_state.action_index;
    }
  }

  if (min_rule == -1) {
    return false;
  }

  rule_index = min_rule;
  action_index = selected_action;
  return true;
}

} // namespace seulex
