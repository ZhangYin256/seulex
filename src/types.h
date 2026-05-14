#pragma once
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace seulex {

// Regex AST node types
enum class RegexNodeType {
  Char,      // 单个字符
  CharClass, // 字符类，如 [a-z0-9...]
  Concat,    // 连接表达式
  Alter,     // 选择表达式 |
  Star,      // 闭包 *
  Plus,      // 至少一次 +
  Option,    // 可选 ?
  Empty      // 空串 / epsilon
};

struct Range {
  unsigned char low;  // 区间下界
  unsigned char high; // 区间上界
};

// 正则表达式节点
struct RegexNode {
  RegexNodeType type; // 当前节点类型
  union {
    unsigned char ch; // 字符节点对应的字符值
    struct {
      bool negated;               // 是否取反字符类，例如 [^a-z]
      std::vector<Range> *ranges; // 字符类包含的区间集合
    } char_class;
    struct {
      RegexNode *left;  // 左子树
      RegexNode *right; // 右子树
    } binary;
    RegexNode *child; // 单目操作的子节点
  };

  explicit RegexNode(RegexNodeType t);
  ~RegexNode();

  RegexNode(const RegexNode &) = delete;
  RegexNode &operator=(const RegexNode &) = delete;
};

// NFA 转移
struct NfaTransition {
  bool epsilon;             // 是否为空转移
  unsigned char input_char; // 输入字符，epsilon=false 时有效
  int target_id;            // 目标状态编号
};

// NFA 状态
struct NfaState {
  int id;                                 // 状态编号
  std::vector<NfaTransition> transitions; // 出边转移列表
  bool is_accept = false;                 // 是否为接受态
  int rule_index = -1;                    // 对应规则索引
  int action_index = -1;                  // 对应动作索引
  bool is_trailing_accept = false;        // 是否为尾部上下文接受态
  int original_rule_index = -1;           // 原始规则编号
};

// DFA 状态
struct DfaState {
  int id;                           // DFA 状态编号
  std::vector<int> nfa_state_ids;   // 该 DFA 状态对应的 NFA 状态集合
  bool is_accept = false;           // 是否为接受态
  int rule_index = -1;              // 对应规则索引
  int action_index = -1;            // 对应动作索引
  bool is_trailing_accept = false;  // 是否为尾部上下文接受态
  int original_rule_index = -1;     // 原始规则编号
  int trailing_length = 0;          // 需要回退的尾部长度
  std::array<int, 256> transitions; // 状态转移表，-1 表示无转移
};

// 完整的flex语法需要
// // 起始条件
// struct StartCondition {
//   std::string name;         // 起始条件名称
//   bool exclusive = false;   // 是否为排他型起始条件
//   int start_state = -1;     // 普通模式下的起始状态
//   int start_state_bol = -1; // 行首锚定时的起始状态
// };

// 最小化后的 DFA，用于代码生成
struct MinDfa {
  int state_count;                                    // 状态总数
  std::vector<std::array<int, 256>> transition_table; // DFA 转移表
  std::vector<int> accept_action; // 接受态对应动作，-1 表示非接受态
  std::vector<int>
      trailing_original_rule;       // 尾部上下文对应的原始规则，-1 表示无
  std::vector<int> trailing_length; // 尾部回退长度
  int start_state;                  // 普通起始状态
  int start_state_bol;              // 行首锚定起始状态
};

// Lex 规则
struct LexRule {
  std::string regex_original;     // 原始正则表达式文本
  std::string regex_normal;       // 规范化后的正则表达式文本
  std::unique_ptr<RegexNode> ast; // 正则表达式 AST
  int action_index = 0;           // 动作代码索引
  int rule_number = 0;            // 规则序号
  // bool has_bol_anchor = false;         // 是否带有行首锚点 ^
  // bool has_trailing_context = false;   // 是否带有尾部上下文
  std::vector<int> start_cond_indices; // 该规则所属的起始条件索引集合

  LexRule() = default;
  LexRule(LexRule &&) = default;
  LexRule &operator=(LexRule &&) = default;
  LexRule(const LexRule &) = delete;
  LexRule &operator=(const LexRule &) = delete;
};

// Defination 表
using DefinitionTable = std::unordered_map<std::string, std::string>;

// 代码块
struct CodeBlock {
  std::string content; // 代码块内容
};

// 整个.l文档
struct SpecDocument {
  DefinitionTable macros;                // 宏定义表
  std::vector<LexRule> rules;            // 词法规则列表
  std::string user_code_section;         // 用户自定义代码区
  std::string top_blocks;                // 文件顶部代码块
  std::vector<CodeBlock> prologue;       // 前置代码块
  std::vector<std::string> action_codes; // 动作代码列表

  // 构造函数和移动语义
  SpecDocument() = default;
  SpecDocument(SpecDocument &&) = default;
  SpecDocument &operator=(SpecDocument &&) = default;
  // 禁止复制语义
  SpecDocument(const SpecDocument &) = delete;
  SpecDocument &operator=(const SpecDocument &) = delete;
};

// 生成器配置
struct GeneratorConfig {
  std::string input_filename;  // 输入文件名
  std::string output_filename; // 输出文件名
  bool to_stdout = false;      // 是否输出到标准输出
  bool debug = false;          // 是否启用调试输出
};

} // namespace seulex
