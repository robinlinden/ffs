// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STARLARK_TOKEN_H_
#define STARLARK_TOKEN_H_

#include <format>
#include <string>
#include <string_view>
#include <variant>

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

} // namespace starlark

#endif // STARLARK_TOKEN_H_
