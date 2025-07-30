// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
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

struct LoadStmt {
  std::string module_name;
  std::vector<std::pair<std::string, std::string>> symbols;
  constexpr bool operator==(LoadStmt const &) const = default;
};

using Statement = std::variant<LoadStmt>;

struct Program {
  std::vector<Statement> statements;
  constexpr bool operator==(Program const &) const = default;
};

class Parser {
public:
  explicit Parser(std::string_view input) : tokenizer_{input} {}

  std::optional<Program> parse() {
    Program program;

    for (auto maybe_token = tokenizer_.tokenize(); maybe_token;
         maybe_token = tokenizer_.tokenize()) {
      auto &token = *maybe_token;

      if (std::holds_alternative<token::Eof>(token)) {
        return program;
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

      std::cerr << "Unexpected token: " << to_string(token) << '\n';
      return std::nullopt;
    }

    return std::nullopt;
  }

private:
  Tokenizer tokenizer_;

  // LoadStmt = 'load' '(' string {',' [identifier '='] string} [','] ')' .
  std::optional<LoadStmt> parse_load_stmt() {
    // load was consumed by the caller.
    if (auto lparen = tokenizer_.tokenize();
        !lparen || !std::holds_alternative<token::Punctuator>(*lparen) ||
        std::get<token::Punctuator>(*lparen) != token::Punctuator::LParen) {
      std::cerr << "Expected '(' after 'load'.\n";
      return std::nullopt;
    }

    auto module_name = tokenizer_.tokenize();
    if (!module_name ||
        !std::holds_alternative<token::StringLiteral>(*module_name)) {
      std::cerr << "Expected module name in load statement.\n";
      return std::nullopt;
    }

    std::vector<std::pair<std::string, std::string>> symbols;

    while (true) {
      auto maybe_comma_or_rparen = tokenizer_.tokenize();
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

      auto maybe_ident_or_symbol = tokenizer_.tokenize();
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
    auto next_token = tokenizer_.tokenize();
    if (!next_token) {
      std::cerr << "Unexpected end of input, expected " << to_string(expected)
                << ".\n";
      return false;
    }

    if (next_token != expected) {
      std::cerr << "Expected " << to_string(expected) << ", got "
                << to_string(*next_token) << ".\n";
      return false;
    }

    return true;
  }

  template <typename T> [[nodiscard]] std::optional<T> next_token_as() {
    auto next_token = tokenizer_.tokenize();
    if (!next_token) {
      std::cerr << "Unexpected end of input.\n";
      return std::nullopt;
    }

    if (auto *t = std::get_if<T>(&*next_token); t != nullptr) {
      return std::move(*t);
    }

    std::cerr << "Expected token of type " << typeid(T).name() << ", got "
              << to_string(*next_token) << ".\n";
    return std::nullopt;
  }
};

inline std::optional<Program> parse(std::string_view input) {
  return Parser{input}.parse();
}

} // namespace starlark

#endif // STARLARK_PARSER_H_
