// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STARLARK_TOKENIZER_H_
#define STARLARK_TOKENIZER_H_

#include "starlark/token.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace starlark {

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

  private:
    bool is_whitespace(char c) const { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }

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
        while (pos_ < input_.size() && (is_alpha(input_[pos_]) || is_digit(input_[pos_]))) {
            ++pos_;
        }

        auto name = input_.substr(start, pos_ - start);
        if (name == "and")
            return token::And{};
        if (name == "else")
            return token::Else{};
        if (name == "load")
            return token::Load{};
        if (name == "break")
            return token::Break{};
        if (name == "for")
            return token::For{};
        if (name == "not")
            return token::Not{};
        if (name == "continue")
            return token::Continue{};
        if (name == "if")
            return token::If{};
        if (name == "or")
            return token::Or{};
        if (name == "def")
            return token::Def{};
        if (name == "in")
            return token::In{};
        if (name == "pass")
            return token::Pass{};
        if (name == "elif")
            return token::Elif{};
        if (name == "lambda")
            return token::Lambda{};
        if (name == "return")
            return token::Return{};

        static constexpr auto kReserved = std::to_array<std::string_view>({
            "as",
            "global",
            "assert",
            "import",
            "async",
            "is",
            "await",
            "nonlocal",
            "class",
            "raise",
            "del",
            "try",
            "except",
            "while",
            "finally",
            "with",
            "from",
            "yield",
        });

        // TODO(robinlinden): Return nice error codes.
        if (std::ranges::contains(kReserved, name)) {
            return std::nullopt;
        }

        return token::Identifier{std::string{name}};
    }

    std::optional<Token> tokenize_punctuator() {
        static auto const puncts = [] {
            auto punctuators = std::to_array<std::pair<std::string_view, Token>>({
                {"+", token::Plus{}},
                {"-", token::Minus{}},
                {"*", token::Star{}},
                {"/", token::Slash{}},
                {"//", token::DoubleSlash{}},
                {"%", token::Percent{}},
                {"**", token::DoubleStar{}},
                {"&", token::Ampersand{}},
                {"|", token::Pipe{}},
                {"^", token::Caret{}},
                {"<<", token::LShift{}},
                {">>", token::RShift{}},
                {".", token::Dot{}},
                {",", token::Comma{}},
                {"=", token::Equals{}},
                {";", token::Semicolon{}},
                {":", token::Colon{}},
                {"(", token::LParen{}},
                {")", token::RParen{}},
                {"[", token::LBracket{}},
                {"]", token::RBracket{}},
                {"{", token::LBrace{}},
                {"}", token::RBrace{}},
                {"<", token::Less{}},
                {">", token::Greater{}},
                {"==", token::EqualEqual{}},
                {"!=", token::NotEqual{}},
                {"+=", token::PlusEquals{}},
                {"-=", token::MinusEquals{}},
                {"*=", token::StarEquals{}},
                {"/=", token::SlashEquals{}},
                {"%=", token::PercentEquals{}},
                {"~", token::Tilde{}},
                {"&=", token::AmpersandEquals{}},
                {"|=", token::PipeEquals{}},
                {"^=", token::CaretEquals{}},
                {"<=", token::LessOrEqual{}},
                {"<<=", token::LShiftEquals{}},
                {">=", token::GreaterOrEqual{}},
                {">>=", token::RShiftEquals{}},
            });

            // Sort by length descending to ensure longer punctuators are
            // matched first.
            std::ranges::sort(
                punctuators,
                [](auto const a, auto const b) { return a.size() > b.size(); },
                &decltype(punctuators)::value_type::first);

            return punctuators;
        }();

        for (auto const &[str, punctuator] : puncts) {
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

inline std::optional<std::vector<Token>> tokenize(std::string_view input) {
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

} // namespace starlark

#endif // STARLARK_TOKENIZER_H_
