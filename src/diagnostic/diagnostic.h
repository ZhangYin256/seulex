#pragma once
#include <stdexcept>
#include <string>
namespace seulex {
class LexError : public std::runtime_error {
public:
  LexError(const std::string &msg, const std::string &file = "", int line = 0,
           int col = 0)
      : std::runtime_error(msg), filename_(file), line_(line), column_(col) {}

  const std::string &filename() const { return filename_; }
  int line() const { return line_; }
  int column() const { return column_; }

private:
  std::string filename_;
  int line_;
  int column_;
};

} // namespace seulex