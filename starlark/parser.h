// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STARLARK_PARSER_H_
#define STARLARK_PARSER_H_

#include "starlark/ast.h"
#include "starlark/token.h"
#include "starlark/tokenizer.h"

#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace starlark {

class Parser {
  public:
    explicit Parser(std::string_view input) : tokenizer_{input} {}

    std::optional<Program> parse() {
        Program program;
        bool needs_newline = false;

        for (auto maybe_token = next_token(); maybe_token; maybe_token = next_token()) {
            auto &token = *maybe_token;

            if (std::holds_alternative<token::Eof>(token)) {
                return program;
            }

            if (std::exchange(needs_newline, false)) {
                if (!std::holds_alternative<token::Newline>(token)) {
                    std::cerr << "Expected newline after previous statement, but got: "
                              << to_string(token) << '\n';
                    return std::nullopt;
                }

                continue;
            }

            if (std::holds_alternative<token::Newline>(token)) {
                continue;
            }

            needs_newline = true;

            if (std::holds_alternative<token::Load>(token)) {
                auto load = parse_load_stmt();
                if (!load) {
                    std::cerr << "Failed to parse load statement.\n";
                    return std::nullopt;
                }

                program.statements.push_back(std::move(*load));
                continue;
            }

            if (auto expr = parse_expression(token); expr.has_value()) {
                auto next = next_token();
                if (next.has_value() && std::holds_alternative<token::Equals>(*next)) {
                    if (!std::holds_alternative<Identifier>(*expr)) {
                        std::cerr << "Left-hand side of assignment must be an identifier.\n";
                        return std::nullopt;
                    }

                    auto rhs_token = next_token();
                    if (!rhs_token) {
                        std::cerr << "Unexpected end of input in assignment.\n";
                        return std::nullopt;
                    }

                    auto rhs = parse_expression(*rhs_token);
                    if (!rhs) {
                        std::cerr << "Failed to parse right-hand side of assignment.\n";
                        return std::nullopt;
                    }

                    program.statements.push_back(
                        AssignStmt{
                            .target = std::move(std::get<Identifier>(*expr)),
                            .value = std::move(*rhs),
                        });
                    continue;
                } else if (next.has_value()) {
                    reconsume(std::move(*next));
                }

                program.statements.push_back(ExpressionStmt{.expr = std::move(*expr)});
                continue;
            }

            std::cerr << "Unexpected token: " << to_string(token) << '\n';
            return std::nullopt;
        }

        return std::nullopt;
    }

  private:
    Tokenizer tokenizer_;
    std::optional<Token> peeked_token_;

    std::optional<Token> next_token() {
        if (peeked_token_) {
            return std::exchange(peeked_token_, std::nullopt);
        }

        return tokenizer_.tokenize();
    }

    std::optional<Token> next_non_newline_token() {
        while (true) {
            auto token = next_token();
            if (!token) {
                return std::nullopt;
            }

            if (!std::holds_alternative<token::Newline>(*token)) {
                return token;
            }
        }
    }

    void reconsume(Token token) { peeked_token_ = std::move(token); }

    std::optional<Expression> parse_operand(Token &token) {
        if (auto const *ident = std::get_if<token::Identifier>(&token)) {
            return Identifier{.name = std::move(ident->name)};
        }

        if (auto *sl = std::get_if<token::StringLiteral>(&token)) {
            return StringLiteral{.value = std::move(sl->value)};
        }

        if (auto *il = std::get_if<token::IntLiteral>(&token)) {
            return IntLiteral{.value = il->value};
        }

        if (std::holds_alternative<token::LBracket>(token)) {
            std::vector<Expression> elements;
            while (true) {
                auto maybe_token = next_non_newline_token();
                if (!maybe_token) {
                    std::cerr << "Tokenization error in list expression.\n";
                    return std::nullopt;
                }

                auto &t = *maybe_token;

                if (std::holds_alternative<token::RBracket>(t)) {
                    break;
                }

                auto element_expr = parse_expression(t);
                if (!element_expr) {
                    std::cerr << "Failed to parse expression for list element.\n";
                    return std::nullopt;
                }

                // On the first iteration, we check if this is a list comprehension.
                auto maybe_next = next_non_newline_token();
                if (elements.empty() && maybe_next.has_value() &&
                    std::holds_alternative<token::For>(*maybe_next)) {
                    auto var_token = next_non_newline_token();
                    if (!var_token || !std::holds_alternative<token::Identifier>(*var_token)) {
                        std::cerr << "Expected identifier in list comprehension.\n";
                        return std::nullopt;
                    }

                    auto in_token = next_non_newline_token();
                    if (!in_token || !std::holds_alternative<token::In>(*in_token)) {
                        std::cerr << "Expected 'in' in list comprehension.\n";
                        return std::nullopt;
                    }

                    auto iterable_token = next_non_newline_token();
                    if (!iterable_token) {
                        std::cerr << "Unexpected end of input in list comprehension.\n";
                        return std::nullopt;
                    }

                    auto iterable_expr = parse_expression(*iterable_token);
                    if (!iterable_expr) {
                        std::cerr << "Failed to parse iterable expression in list comprehension.\n";
                        return std::nullopt;
                    }

                    auto maybe_closing = next_non_newline_token();
                    if (!maybe_closing ||
                        !std::holds_alternative<token::RBracket>(*maybe_closing)) {
                        std::cerr << "Expected closing ']' in list comprehension.\n";
                        return std::nullopt;
                    }

                    // TODO(robinlinden): Handle optional 'if' clause.

                    return ListComp{
                        .element = std::make_shared<Expression>(std::move(*element_expr)),
                        .iterator_var =
                            Identifier{.name = std::get<token::Identifier>(*var_token).name},
                        .iterable = std::make_shared<Expression>(std::move(*iterable_expr)),
                    };
                }

                elements.push_back(std::move(*element_expr));

                if (!maybe_next) {
                    std::cerr << "Tokenization error in list expression.\n";
                    return std::nullopt;
                }

                auto const &next_token = *maybe_next;
                if (std::holds_alternative<token::RBracket>(next_token)) {
                    break;
                }

                if (std::holds_alternative<token::Comma>(next_token)) {
                    continue;
                }

                std::cerr << "Expected ',' or ']' in list expression, got '"
                          << to_string(next_token) << "'.\n";
                return std::nullopt;
            }

            return ListExpr{.elements = std::move(elements)};
        }

        if (std::holds_alternative<token::LBrace>(token)) {
            std::vector<std::pair<Expression, Expression>> entries;

            while (true) {
                auto maybe_token = next_non_newline_token();
                if (!maybe_token) {
                    std::cerr << "Tokenization error in dict expression.\n";
                    return std::nullopt;
                }

                auto &t = *maybe_token;

                if (std::holds_alternative<token::RBrace>(t)) {
                    break;
                }

                auto key_expr = parse_expression(t);
                if (!key_expr) {
                    std::cerr << "Failed to parse expression for dict key.\n";
                    return std::nullopt;
                }

                auto colon_token = next_non_newline_token();
                if (!colon_token || !std::holds_alternative<token::Colon>(*colon_token)) {
                    std::cerr << "Expected ':' after dict key.\n";
                    return std::nullopt;
                }

                auto value_token = next_non_newline_token();
                if (!value_token) {
                    std::cerr << "Unexpected end of input in dict expression.\n";
                    return std::nullopt;
                }

                auto value_expr = parse_expression(*value_token);
                if (!value_expr) {
                    std::cerr << "Failed to parse expression for dict value.\n";
                    return std::nullopt;
                }

                entries.emplace_back(std::move(*key_expr), std::move(*value_expr));

                auto maybe_next = next_non_newline_token();
                if (!maybe_next) {
                    std::cerr << "Tokenization error in dict expression.\n";
                    return std::nullopt;
                }

                auto const &next_token = *maybe_next;
                if (std::holds_alternative<token::RBrace>(next_token)) {
                    break;
                }

                if (std::holds_alternative<token::Comma>(next_token)) {
                    continue;
                }

                std::cerr << "Expected ',' or '}' in dict expression, got '"
                          << to_string(next_token) << "'.\n";
                return std::nullopt;
            }

            return DictExpr{.entries = std::move(entries)};
        }

        std::cerr << "Unexpected token in operand: " << to_string(token) << ".\n";
        return std::nullopt;
    }

    std::optional<Expression> parse_expression(Token &token) {
        auto expr = parse_operand(token);
        if (!expr) {
            std::cerr << "Failed to parse operand.\n";
            return std::nullopt;
        }

        while (true) {
            auto next = next_token();
            if (!next) {
                return expr;
            }

            if (std::holds_alternative<token::LParen>(*next)) {
                auto args = parse_argument_list();
                if (!args) {
                    std::cerr << "Failed to parse argument list.\n";
                    return std::nullopt;
                }

                expr = CallExpr{
                    .target = std::make_shared<Expression>(std::move(*expr)),
                    .args = std::move(*args),
                };

                continue;
            }

            if (std::holds_alternative<token::LBracket>(*next)) {
                next = next_token();
                if (!next) {
                    std::cerr << "Unexpected end of input after '['.\n";
                    return std::nullopt;
                }

                auto index_expr = parse_expression(*next);
                if (!index_expr) {
                    std::cerr << "Failed to parse index expression.\n";
                    return std::nullopt;
                }

                if (!expect_next_token(token::RBracket{})) {
                    return std::nullopt;
                }

                expr = SliceExpr{
                    .target = std::make_shared<Expression>(std::move(*expr)),
                    .index = std::make_shared<Expression>(std::move(*index_expr)),
                };

                continue;
            }

            if (std::holds_alternative<token::Dot>(*next)) {
                auto member_token = next_token();
                if (!member_token || !std::holds_alternative<token::Identifier>(*member_token)) {
                    std::cerr << "Expected identifier after '.'.\n";
                    return std::nullopt;
                }

                expr = MemberExpr{
                    .target = std::make_shared<Expression>(std::move(*expr)),
                    .member = Identifier{.name = std::get<token::Identifier>(*member_token).name},
                };

                continue;
            }

            // We didn't end up using the next token, so put it back for later.
            reconsume(std::move(*next));
            return expr;
        }
    }

    std::optional<std::vector<Argument>> parse_argument_list() {
        std::vector<Argument> args;
        bool seen_kw_arg = false;

        while (true) {
            auto maybe_token = next_non_newline_token();
            if (!maybe_token) {
                std::cerr << "Unexpected end of input in argument list.\n";
                return std::nullopt;
            }

            if (!args.empty()) {
                auto const &token = *maybe_token;
                if (!std::holds_alternative<token::Comma>(token) &&
                    !std::holds_alternative<token::RParen>(token)) {
                    std::cerr << "Expected ',' or ')' in argument list, got '" << to_string(token)
                              << "'.\n";
                    return std::nullopt;
                }

                if (std::holds_alternative<token::Comma>(token)) {
                    maybe_token = next_non_newline_token();
                    if (!maybe_token) {
                        std::cerr << "Unexpected end of input in argument list.\n";
                        return std::nullopt;
                    }
                }
            }

            auto &token = *maybe_token;

            if (std::holds_alternative<token::RParen>(token)) {
                break;
            }

            auto &arg = args.emplace_back();
            std::optional<Identifier> &name = arg.name;
            Expression &expr = arg.expr;

            if (auto *ident = std::get_if<token::Identifier>(&token)) {
                auto next = next_non_newline_token();
                if (!next) {
                    std::cerr << "Unexpected end of input in argument list.\n";
                    return std::nullopt;
                }

                if (std::holds_alternative<token::Equals>(*next)) {
                    seen_kw_arg = true;
                    name = Identifier{.name = std::move(ident->name)};

                    auto value_token = next_non_newline_token();
                    if (!value_token) {
                        std::cerr << "Unexpected end of input in argument list.\n";
                        return std::nullopt;
                    }

                    auto value_expr = parse_expression(*value_token);
                    if (!value_expr) {
                        std::cerr << "Failed to parse expression for argument "
                                     "value.\n";
                        return std::nullopt;
                    }

                    expr = std::move(*value_expr);
                    continue;
                }

                // Not a named argument, fall through to regular arg handling.
                reconsume(std::move(*next));
            }

            if (seen_kw_arg) {
                std::cerr << "Positional argument may not follow keyword argument.\n";
                return std::nullopt;
            }

            auto value_expr = parse_expression(token);
            if (!value_expr) {
                std::cerr << "Failed to parse expression for argument value.\n";
                return std::nullopt;
            }

            expr = std::move(*value_expr);
            continue;
        }

        return args;
    }

    // LoadStmt = 'load' '(' string {',' [identifier '='] string} [','] ')' .
    std::optional<LoadStmt> parse_load_stmt() {
        // load was consumed by the caller.
        if (auto lparen = next_token();
            !lparen || !std::holds_alternative<token::LParen>(*lparen)) {
            std::cerr << "Expected '(' after 'load'.\n";
            return std::nullopt;
        }

        auto module_name = next_token();
        if (!module_name || !std::holds_alternative<token::StringLiteral>(*module_name)) {
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

            if (std::holds_alternative<token::RParen>(*maybe_comma_or_rparen)) {
                break;
            }

            if (!std::holds_alternative<token::Comma>(*maybe_comma_or_rparen)) {
                std::cerr << "Expected ',' or ')' in load statement, got '"
                          << to_string(*maybe_comma_or_rparen) << "'.\n";
                return std::nullopt;
            }

            auto maybe_ident_or_symbol = next_token();
            if (!maybe_ident_or_symbol) {
                std::cerr << "Unexpected end of input in load statement.\n";
                return std::nullopt;
            }

            if (auto *symbol = std::get_if<token::StringLiteral>(&*maybe_ident_or_symbol)) {
                auto name = symbol->value;
                symbols.emplace_back(std::move(name), std::move(symbol->value));
                continue;
            }

            auto *ident = std::get_if<token::Identifier>(&*maybe_ident_or_symbol);
            if (!ident) {
                return std::nullopt;
            }

            if (!expect_next_token(token::Equals{})) {
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
            std::move(std::get<token::StringLiteral>(*module_name).value), std::move(symbols)};
    }

    [[nodiscard]] bool expect_next_token(Token const &expected) {
        auto next = next_token();
        if (!next) {
            std::cerr << "Unexpected end of input, expected " << to_string(expected) << ".\n";
            return false;
        }

        if (next != expected) {
            std::cerr << "Expected " << to_string(expected) << ", got '" << to_string(*next)
                      << "'.\n";
            return false;
        }

        return true;
    }

    template<typename T>
    [[nodiscard]] std::optional<T> next_token_as() {
        auto next = next_token();
        if (!next) {
            std::cerr << "Unexpected end of input.\n";
            return std::nullopt;
        }

        if (auto *t = std::get_if<T>(&*next); t != nullptr) {
            return std::move(*t);
        }

        std::cerr << "Expected token of type " << typeid(T).name() << ", got '" << to_string(*next)
                  << "'.\n";
        return std::nullopt;
    }
};

inline std::optional<Program> parse(std::string_view input) { return Parser{input}.parse(); }

} // namespace starlark

#endif // STARLARK_PARSER_H_
