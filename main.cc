// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include <algorithm>
#include <array>
#include <cassert>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

// https://github.com/bazelbuild/starlark/blob/0829066b23341aa6f3d4599729f3324b9032f7b8/spec.md#grammar-reference
namespace starlark {
namespace token {

enum class Punctuator {
  Plus,              // +
  Minus,             // -
  Star,              // *
  Slash,             // /
  DoubleSlash,       // //
  Percent,           // %
  DoubleStar,        // **
  Tilde,             // ~
  Ampersand,         // &
  Pipe,              // |
  Caret,             // ^
  LShift,            // <<
  RShift,            // >>
  Dot,               // .
  Comma,             // ,
  Equals,            // =
  Semicolon,         // ;
  Colon,             // :
  LParen,            // (
  RParen,            // )
  LBracket,          // [
  RBracket,          // ]
  LBrace,            // {
  RBrace,            // }
  Less,              // <
  Greater,           // >
  GreaterOrEqual,    // >=
  LessOrEqual,       // <=
  EqualEqual,        // ==
  NotEqual,          // !=
  PlusEquals,        // +=
  MinusEquals,       // -=
  StarEquals,        // *=
  SlashEquals,       // /=
  DoubleSlashEquals, // //=
  PercentEquals,     // %=
  AmpersandEquals,   // &=
  PipeEquals,        // |=
  CaretEquals,       // ^=
  LShiftEquals,      // <<=
  RShiftEquals,      // >>=
};

constexpr std::string_view to_string(Punctuator p) {
  switch (p) {
  case Punctuator::Plus:
    return "+";
  case Punctuator::Minus:
    return "-";
  case Punctuator::Star:
    return "*";
  case Punctuator::Slash:
    return "/";
  case Punctuator::DoubleSlash:
    return "//";
  case Punctuator::Percent:
    return "%";
  case Punctuator::DoubleStar:
    return "**";
  case Punctuator::Tilde:
    return "~";
  case Punctuator::Ampersand:
    return "&";
  case Punctuator::Pipe:
    return "|";
  case Punctuator::Caret:
    return "^";
  case Punctuator::LShift:
    return "<<";
  case Punctuator::RShift:
    return ">>";
  case Punctuator::Dot:
    return ".";
  case Punctuator::Comma:
    return ",";
  case Punctuator::Equals:
    return "=";
  case Punctuator::Semicolon:
    return ";";
  case Punctuator::Colon:
    return ":";
  case Punctuator::LParen:
    return "(";
  case Punctuator::RParen:
    return ")";
  case Punctuator::LBracket:
    return "[";
  case Punctuator::RBracket:
    return "]";
  case Punctuator::LBrace:
    return "{";
  case Punctuator::RBrace:
    return "}";
  case Punctuator::Less:
    return "<";
  case Punctuator::Greater:
    return ">";
  case Punctuator::GreaterOrEqual:
    return ">=";
  case Punctuator::LessOrEqual:
    return "<=";
  case Punctuator::EqualEqual:
    return "==";
  case Punctuator::NotEqual:
    return "!=";
  case Punctuator::PlusEquals:
    return "+=";
  case Punctuator::MinusEquals:
    return "-=";
  case Punctuator::StarEquals:
    return "*=";
  case Punctuator::SlashEquals:
    return "/=";
  case Punctuator::DoubleSlashEquals:
    return "//=";
  case Punctuator::PercentEquals:
    return "%=";
  case Punctuator::AmpersandEquals:
    return "&=";
  case Punctuator::PipeEquals:
    return "|=";
  case Punctuator::CaretEquals:
    return "^=";
  case Punctuator::LShiftEquals:
    return "<<=";
  case Punctuator::RShiftEquals:
    return ">>=";
  }

  return "<unknown>";
}

enum class Keyword {
  And,      // and
  Else,     // else
  Load,     // load
  Break,    // break
  For,      // for
  Not,      // not
  Continue, // continue
  If,       // if
  Or,       // or
  Def,      // def
  In,       // in
  Pass,     // pass
  Elif,     // elif
  Lambda,   // lambda
  Return    // return
};

constexpr std::string_view to_string(Keyword k) {
  switch (k) {
  case Keyword::And:
    return "and";
  case Keyword::Else:
    return "else";
  case Keyword::Load:
    return "load";
  case Keyword::Break:
    return "break";
  case Keyword::For:
    return "for";
  case Keyword::Not:
    return "not";
  case Keyword::Continue:
    return "continue";
  case Keyword::If:
    return "if";
  case Keyword::Or:
    return "or";
  case Keyword::Def:
    return "def";
  case Keyword::In:
    return "in";
  case Keyword::Pass:
    return "pass";
  case Keyword::Elif:
    return "elif";
  case Keyword::Lambda:
    return "lambda";
  case Keyword::Return:
    return "return";
  }

  return "<unknown>";
}

struct Identifier {
  std::string name;
};

constexpr std::string_view to_string(const Identifier &id) { return id.name; }

struct StringLiteral {
  std::string value;
};

inline std::string to_string(const StringLiteral &str) {
  return std::format(R"("{}")", str.value);
}

struct Eof {};

constexpr std::string_view to_string(const Eof &) { return "<eof>"; }

} // namespace token

using Token = std::variant<token::Punctuator, token::Keyword, token::Identifier,
                           token::StringLiteral, token::Eof>;

inline std::string to_string(const Token &token) {
  return std::visit([](auto &&arg) { return std::string{to_string(arg)}; },
                    token);
}

class Tokenizer {
public:
  explicit Tokenizer(std::string_view input) : input_(input) {}

  std::optional<Token> tokenize() {
    skip_comments_and_whitespace();

    if (pos_ >= input_.size()) {
      return token::Eof{};
    }

    if (input_.substr(pos_, 3) == R"(""")") {
      return tokenize_multiline_string();
    }

    if (input_[pos_] == '"') {
      return tokenize_string();
    }

    if (is_alpha(input_[pos_])) {
      return tokenize_identifier();
    }

    return tokenize_punctuator();
  }

  std::string_view remaining_input() const { return input_.substr(pos_); }

private:
  bool is_whitespace(char c) const {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
  }

  bool is_alpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
  }

  bool is_digit(char c) const { return c >= '0' && c <= '9'; }

  void skip_comments_and_whitespace() {
    bool continue_skipping = true;
    while (std::exchange(continue_skipping, false)) {
      while (pos_ < input_.size() && is_whitespace(input_[pos_])) {
        ++pos_;
      }

      if (pos_ < input_.size() && input_[pos_] == '#') {
        continue_skipping = true;
        while (pos_ < input_.size() && input_[pos_] != '\n') {
          ++pos_;
        }
      }
    }
  }

  // TODO(robinlinden): Support escapes.
  std::optional<Token> tokenize_multiline_string() {
    pos_ += 3;                // Move past the opening triple quotes
    std::size_t start = pos_; // Skip the opening triple quotes

    while (pos_ + 2 < input_.size() && input_.substr(pos_, 3) != R"(""")") {
      pos_++;
    }

    if (pos_ + 2 >= input_.size()) {
      return std::nullopt;
    }

    auto value = std::string{input_.substr(start, pos_ - start)};
    pos_ += 3; // Skip the closing triple quotes
    return token::StringLiteral{std::move(value)};
  }

  // TODO(robinlinden): Support escapes.
  std::optional<Token> tokenize_string() {
    assert(input_[pos_] == '"');
    std::size_t start = ++pos_;
    while (pos_ < input_.size() && input_[pos_] != '"') {
      ++pos_;
    }

    if (pos_ >= input_.size()) {
      return std::nullopt;
    }

    auto value = std::string{input_.substr(start, pos_ - start)};
    ++pos_;
    return token::StringLiteral{std::move(value)};
  }

  std::optional<Token> tokenize_identifier() {
    assert(is_alpha(input_[pos_]) || input_[pos_] == '_');

    std::size_t start = pos_;
    while (pos_ < input_.size() &&
           (is_alpha(input_[pos_]) || is_digit(input_[pos_]))) {
      ++pos_;
    }

    auto name = input_.substr(start, pos_ - start);
    if (name == "and")
      return token::Keyword::And;
    if (name == "else")
      return token::Keyword::Else;
    if (name == "load")
      return token::Keyword::Load;
    if (name == "break")
      return token::Keyword::Break;
    if (name == "for")
      return token::Keyword::For;
    if (name == "not")
      return token::Keyword::Not;
    if (name == "continue")
      return token::Keyword::Continue;
    if (name == "if")
      return token::Keyword::If;
    if (name == "or")
      return token::Keyword::Or;
    if (name == "def")
      return token::Keyword::Def;
    if (name == "in")
      return token::Keyword::In;
    if (name == "pass")
      return token::Keyword::Pass;
    if (name == "elif")
      return token::Keyword::Elif;
    if (name == "lambda")
      return token::Keyword::Lambda;
    if (name == "return")
      return token::Keyword::Return;

    return token::Identifier{std::string{name}};
  }

  std::optional<Token> tokenize_punctuator() {
    static constexpr auto kPunctuators = [] {
      auto punctuators =
          std::to_array<std::pair<std::string_view, token::Punctuator>>({
              {"+", token::Punctuator::Plus},
              {"-", token::Punctuator::Minus},
              {"*", token::Punctuator::Star},
              {"/", token::Punctuator::Slash},
              {"//", token::Punctuator::DoubleSlash},
              {"%", token::Punctuator::Percent},
              {"**", token::Punctuator::DoubleStar},
              {"&", token::Punctuator::Ampersand},
              {"|", token::Punctuator::Pipe},
              {"^", token::Punctuator::Caret},
              {"<<", token::Punctuator::LShift},
              {">>", token::Punctuator::RShift},
              {".", token::Punctuator::Dot},
              {",", token::Punctuator::Comma},
              {"=", token::Punctuator::Equals},
              {";", token::Punctuator::Semicolon},
              {":", token::Punctuator::Colon},
              {"(", token::Punctuator::LParen},
              {")", token::Punctuator::RParen},
              {"[", token::Punctuator::LBracket},
              {"]", token::Punctuator::RBracket},
              {"{", token::Punctuator::LBrace},
              {"}", token::Punctuator::RBrace},
              {"<", token::Punctuator::Less},
              {">", token::Punctuator::Greater},
              {"==", token::Punctuator::EqualEqual},
              {"!=", token::Punctuator::NotEqual},
              {"+=", token::Punctuator::PlusEquals},
              {"-=", token::Punctuator::MinusEquals},
              {"*=", token::Punctuator::StarEquals},
              {"/=", token::Punctuator::SlashEquals},
              {"%=", token::Punctuator::PercentEquals},
              {"~", token::Punctuator::Tilde},
              {"&=", token::Punctuator::AmpersandEquals},
              {"|=", token::Punctuator::PipeEquals},
              {"^=", token::Punctuator::CaretEquals},
              {"<=", token::Punctuator::LessOrEqual},
              {"<<=", token::Punctuator::LShiftEquals},
              {">=", token::Punctuator::GreaterOrEqual},
              {">>=", token::Punctuator::RShiftEquals},
          });

      // Sort by length descending to ensure longer punctuators are matched
      // first.
      std::ranges::sort(
          punctuators,
          [](auto const a, auto const b) { return a.size() > b.size(); },
          &decltype(punctuators)::value_type::first);

      return punctuators;
    }();

    for (const auto &[str, punctuator] : kPunctuators) {
      if (input_.substr(pos_, str.size()) == str) {
        pos_ += str.size();
        return punctuator;
      }
    }

    return std::nullopt;
  }

  std::string_view input_;
  std::size_t pos_ = 0;
};

std::optional<std::vector<Token>> tokenize(std::string_view input) {
  Tokenizer tokenizer{input};
  std::vector<Token> tokens;

  while (true) {
    auto token = tokenizer.tokenize();
    if (!token) {
      return std::nullopt;
    }

    if (std::holds_alternative<token::Eof>(*token)) {
      break;
    }

    tokens.push_back(std::move(*token));
  }

  return tokens;
}

std::string to_string(std::vector<Token> const &tokens) {
  std::stringstream ss;

  for (auto const &token : tokens) {
    ss << to_string(token) << ' ';
  }

  return std::move(ss).str();
}

} // namespace starlark

int main(int argc, char **argv) {
  if (argc != 2) {
    std::string_view name = argv[0] != nullptr ? argv[0] : "<bin>";
    std::cerr << "Usage: " << name << " <input_file>\n";
    return 1;
  }

  auto input = std::ifstream{argv[1]};
  if (!input) {
    std::cerr << "Error: Could not open file " << argv[1] << "\n";
    return 1;
  }

  std::string content{std::istreambuf_iterator<char>(input),
                      std::istreambuf_iterator<char>()};

  std::cout << "Input:\n" << content << "\n\n";

  auto tokens = starlark::tokenize(content);
  if (!tokens) {
    std::cerr << "Error: Failed to tokenize input.\n";
    return 1;
  }

  std::cout << "Tokens:\n" << starlark::to_string(*tokens) << "\n";
}
