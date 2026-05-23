#include "automata/dfa_minimizer.h"

#include <map>

namespace seulex {

namespace {

std::pair<bool, int> make_partition_key(const DfaState &state) {
  return {state.is_accept, state.is_accept ? state.action_index : -1};
}

// 构造状态转移的划分键，基于当前状态的所有输入字符转移到的块 ID
std::vector<int> make_transition_key(const DfaState &state,
                                     const std::vector<int> &block_id) {
  std::vector<int> key;
  key.reserve(256);

  for (int c = 0; c < 256; ++c) {
    int next = state.transitions[c];
    int next_block = -1;
    if (next >= 0 && next < static_cast<int>(block_id.size())) {
      next_block = block_id[next];
    }
    key.push_back(next_block);
  }

  return key;
}

} // namespace

MinDfa DfaMinimizer::minimize(const std::vector<DfaState> &dfa_states,
                              int start_state) const {
  MinDfa result;
  if (dfa_states.empty() || start_state < 0 ||
      start_state >= static_cast<int>(dfa_states.size())) {
    result.start_state = -1;
    result.start_state_bol = -1;
    return result;
  }

  DfaMinimizer working;
  working.build_initial_partition(dfa_states);
  while (working.refine_partition(dfa_states)) {
  }

  // 构建最小化 DFA 的状态和转移表
  const int block_count = static_cast<int>(working.partition_.size());
  result.state_count = block_count;
  result.transition_table.resize(block_count);
  result.accept_action.assign(block_count, -1);
  // 暂时不支持尾部上下文接受态
  // result.trailing_original_rule.assign(block_count, -1);
  // result.trailing_length.assign(block_count, 0);

  for (auto &row : result.transition_table) {
    row.fill(-1);
  }
  // 构建旧状态 ID 到新状态 ID 的映射
  std::vector<int> old_to_new(dfa_states.size(), -1);
  int next_id = 0;
  for (const auto &block : working.partition_) {
    // 遍历每个划分块，给其中的状态分配新的状态 ID
    if (block.empty()) {
      continue;
    }
    for (int state_id : block) {
      old_to_new[state_id] = next_id;
    }
    ++next_id;
  }

  // 遍历每个划分块，构建最小化 DFA 的状态转移和接受态信息
  for (const auto &block : working.partition_) {
    if (block.empty()) {
      continue;
    }
    /*
    * 在 DFA 最小化中，一个等价类（块）中的所有原始状态在行为上完全等价：
    * 它们要么都是接受状态，要么都不是；
    * 若为接受状态，它们关联的语义动作（action_index）相同；
    * 对于每个输入字符，它们转移到的目标状态属于同一个等价类（即经过映射后指向同一新状态）。
    */
    const int representative = block.front(); // 第一个状态作为代表状态
    const int new_id = old_to_new[representative];
    const DfaState &state = dfa_states[representative];

    if (state.is_accept) {
      result.accept_action[new_id] = state.action_index;
    }
    // if (state.is_trailing_accept) {
    //   result.trailing_original_rule[new_id] = state.original_rule_index;
    //   result.trailing_length[new_id] = state.trailing_length;
    // }

    // 对该划分对应的状态构建转移表
    for (int c = 0; c < 256; ++c) {
      int next = state.transitions[c];
      if (next >= 0 && next < static_cast<int>(old_to_new.size())) {
        result.transition_table[new_id][c] = old_to_new[next];
      }
    }
  }
  // 设置起始状态，确保其在有效范围内
  if (start_state >= 0 && start_state < static_cast<int>(old_to_new.size())) {
    result.start_state = old_to_new[start_state];
  } else {
    result.start_state = 0;
  }
  // result.start_state_bol = result.start_state;

  return result;
}

// 构造初始划分
void DfaMinimizer::build_initial_partition(
    const std::vector<DfaState> &dfa_states) {
  partition_.clear();
  // 初始化 block_id_，-1 表示未分配块
  block_id_.assign(dfa_states.size(), -1);

  // 根据接受状态和动作索引进行初始划分
  std::map<std::pair<bool, int>, std::vector<int>> groups;
  for (int state_id = 0; state_id < static_cast<int>(dfa_states.size());
       ++state_id) {
    groups[make_partition_key(dfa_states[state_id])].push_back(state_id);
  }
  // 对map使用迭代器时，其值类型为std::pair<const Key, Value>，因此需要使用entry.second来访问value
  for (auto &entry : groups) {
    partition_.push_back(std::move(entry.second));
  }

  rebuild_block_ids();
}

// 划分细化，返回是否发生了划分变化
bool DfaMinimizer::refine_partition(const std::vector<DfaState> &dfa_states) {
  bool changed = false;
  std::vector<std::vector<int>> new_partition;
  // 当前的每个划分块
  for (const auto &block : partition_) {
    if (block.size() <= 1) {
      new_partition.push_back(block);
      continue;
    }
    // 当前划分块内划分之后的新分组
    std::map<std::vector<int>, std::vector<int>> groups;
    for (int state_id : block) {
      groups[make_transition_key(dfa_states[state_id], block_id_)].push_back(
          state_id);
    }

    if (groups.size() > 1) {
      changed = true;
    }

    for (auto &entry : groups) {
      new_partition.push_back(std::move(entry.second));
    }
  }

  if (changed) {
    partition_ = std::move(new_partition);
    rebuild_block_ids();
  }

  return changed;
}

// 基于新分组重建块 ID 映射
void DfaMinimizer::rebuild_block_ids() {
  block_id_.assign(block_id_.size(), -1);
  // 对每个块中的每个状态遍历
  for (int block_index = 0; block_index < static_cast<int>(partition_.size());
       ++block_index) {
    for (int state_id : partition_[block_index]) {
      if (state_id >= 0 && state_id < static_cast<int>(block_id_.size())) {
        block_id_[state_id] = block_index;
      }
    }
  }
}

} // namespace seulex