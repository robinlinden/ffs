// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STARLARK_PARSER_H_
#define STARLARK_PARSER_H_

#include "starlark/token.h"
#include "starlark/tokenizer.h"

#include <cassert>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace starlark {

struct StringLiteral {
  std::string value;
  constexpr bool operator==(StringLiteral const &) const = default;
};

struct Identifier {
  std::string name;
  constexpr bool operator==(Identifier const &) const = default;
};

using Expression = std::variant<StringLiteral, Identifier>;

struct ExpressionStmt {
  Expression expr;
  constexpr bool operator==(ExpressionStmt const &) const = default;
};

struct LoadStmt {
  std::string module_name;
  std::vector<std::pair<std::string, std::string>> symbols;
  constexpr bool operator==(LoadStmt const &) const = default;
};

using Statement = std::variant<LoadStmt, ExpressionStmt>;

struct Program {
  std::vector<Statement> statements;
  constexpr bool operator==(Program const &) const = default;
};

class Parser {
public:
  explicit Parser(std::string_view input) : tokenizer_{input} {}

  std::optional<Program> parse() {
    Program program;

    for (auto maybe_token = next_token(); maybe_token;
         maybe_token = next_token()) {
      auto &token = *maybe_token;

      if (std::holds_alternative<token::Eof>(token)) {
        return program;
      }

      if (auto *sl = std::get_if<token::StringLiteral>(&token)) {
        program.statements.push_back(ExpressionStmt{
            .expr = StringLiteral{.value = std::move(sl->value)},
        });

        continue;
      }

      if (auto const *kw = std::get_if<token::Keyword>(&token)) {
        if (*kw == token::Keyword::Load) {
          auto load = parse_load_stmt();
          if (!load) {
            std::cerr << "Failed to parse load statement.\n";
            return std::nullopt;
          }

          program.statements.push_back(std::move(*load));
          continue;
        }

        std::cerr << "Unexpected keyword: " << to_string(*kw) << '\n';
        break;
      }

      if (auto const *ident = std::get_if<token::Identifier>(&token)) {
        program.statements.push_back(ExpressionStmt{
            .expr = Identifier{.name = std::move(ident->name)},
        });

        continue;
      }

      std::cerr << "Unexpected token: " << to_string(token) << '\n';
      return std::nullopt;
    }

    return std::nullopt;
  }

private:
  Tokenizer tokenizer_;

  std::optional<Token> next_token() { return tokenizer_.tokenize(); }

  // LoadStmt = 'load' '(' string {',' [identifier '='] string} [','] ')' .
  std::optional<LoadStmt> parse_load_stmt() {
    // load was consumed by the caller.
    if (auto lparen = next_token();
        !lparen || !std::holds_alternative<token::Punctuator>(*lparen) ||
        std::get<token::Punctuator>(*lparen) != token::Punctuator::LParen) {
      std::cerr << "Expected '(' after 'load'.\n";
      return std::nullopt;
    }

    auto module_name = next_token();
    if (!module_name ||
        !std::holds_alternative<token::StringLiteral>(*module_name)) {
      std::cerr << "Expected module name in load statement.\n";
      return std::nullopt;
    }

    std::vector<std::pair<std::string, std::string>> symbols;

    while (true) {
      auto maybe_comma_or_rparen = next_token();
      if (!maybe_comma_or_rparen) {
        std::cerr << "Unexpected end of input in load statement.\n";
        return std::nullopt;
      }

      auto const *comma_or_rparen =
          std::get_if<token::Punctuator>(&*maybe_comma_or_rparen);
      if (comma_or_rparen == nullptr) {
        std::cerr << "Expected ',' or ')' in load statement, got "
                  << to_string(*maybe_comma_or_rparen) << ".\n";
        return std::nullopt;
      }

      if (*comma_or_rparen == token::Punctuator::RParen) {
        break;
      }

      if (*comma_or_rparen != token::Punctuator::Comma) {
        std::cerr << "Expected ',' or ')' in load statement, got "
                  << to_string(*comma_or_rparen) << ".\n";
        return std::nullopt;
      }

      auto maybe_ident_or_symbol = next_token();
      if (!maybe_ident_or_symbol) {
        std::cerr << "Unexpected end of input in load statement.\n";
        return std::nullopt;
      }

      if (auto *symbol =
              std::get_if<token::StringLiteral>(&*maybe_ident_or_symbol)) {
        auto name = symbol->value;
        symbols.emplace_back(std::move(name), std::move(symbol->value));
        continue;
      }

      auto *ident = std::get_if<token::Identifier>(&*maybe_ident_or_symbol);
      if (!ident) {
        return std::nullopt;
      }

      if (!expect_next_token(token::Punctuator::Equals)) {
        return std::nullopt;
      }

      auto symbol = next_token_as<token::StringLiteral>();
      if (!symbol) {
        return std::nullopt;
      }

      symbols.emplace_back(std::move(ident->name), std::move(symbol->value));
    }

    if (symbols.empty()) {
      std::cerr << "Expected at least one symbol in load statement.\n";
      return std::nullopt;
    }

    return LoadStmt{
        std::move(std::get<token::StringLiteral>(*module_name).value),
        std::move(symbols)};
  }

  [[nodiscard]] bool expect_next_token(Token const &expected) {
    auto next = next_token();
    if (!next) {
      std::cerr << "Unexpected end of input, expected " << to_string(expected)
                << ".\n";
      return false;
    }

    if (next != expected) {
      std::cerr << "Expected " << to_string(expected) << ", got "
                << to_string(*next) << ".\n";
      return false;
    }

    return true;
  }

  template <typename T> [[nodiscard]] std::optional<T> next_token_as() {
    auto next = next_token();
    if (!next) {
      std::cerr << "Unexpected end of input.\n";
      return std::nullopt;
    }

    if (auto *t = std::get_if<T>(&*next); t != nullptr) {
      return std::move(*t);
    }

    std::cerr << "Expected token of type " << typeid(T).name() << ", got "
              << to_string(*next) << ".\n";
    return std::nullopt;
  }
};

inline std::optional<Program> parse(std::string_view input) {
  return Parser{input}.parse();
}

} // namespace starlark

#endif // STARLARK_PARSER_H_
