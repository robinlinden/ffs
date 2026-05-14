// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STARLARK_TOKEN_H_
#define STARLARK_TOKEN_H_

#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <variant>

namespace starlark {
namespace token {

struct Plus {
    constexpr bool operator==(Plus const &) const = default;
};

struct Minus {
    constexpr bool operator==(Minus const &) const = default;
};

struct Star {
    constexpr bool operator==(Star const &) const = default;
};

struct Slash {
    constexpr bool operator==(Slash const &) const = default;
};

constexpr std::string_view to_string(Plus) { return "+"; }
constexpr std::string_view to_string(Minus) { return "-"; }
constexpr std::string_view to_string(Star) { return "*"; }
constexpr std::string_view to_string(Slash) { return "/"; }

struct DoubleSlash {
    constexpr bool operator==(DoubleSlash const &) const = default;
};

struct Percent {
    constexpr bool operator==(Percent const &) const = default;
};

struct DoubleStar {
    constexpr bool operator==(DoubleStar const &) const = default;
};

constexpr std::string_view to_string(DoubleSlash) { return "//"; }
constexpr std::string_view to_string(Percent) { return "%"; }
constexpr std::string_view to_string(DoubleStar) { return "**"; }

struct Tilde {
    constexpr bool operator==(Tilde const &) const = default;
};

struct Ampersand {
    constexpr bool operator==(Ampersand const &) const = default;
};

struct Pipe {
    constexpr bool operator==(Pipe const &) const = default;
};

struct Caret {
    constexpr bool operator==(Caret const &) const = default;
};

constexpr std::string_view to_string(Tilde) { return "~"; }
constexpr std::string_view to_string(Ampersand) { return "&"; }
constexpr std::string_view to_string(Pipe) { return "|"; }
constexpr std::string_view to_string(Caret) { return "^"; }

struct LShift {
    constexpr bool operator==(LShift const &) const = default;
};

struct RShift {
    constexpr bool operator==(RShift const &) const = default;
};

struct Dot {
    constexpr bool operator==(Dot const &) const = default;
};

struct Comma {
    constexpr bool operator==(Comma const &) const = default;
};

constexpr std::string_view to_string(LShift) { return "<<"; }
constexpr std::string_view to_string(RShift) { return ">>"; }
constexpr std::string_view to_string(Dot) { return "."; }
constexpr std::string_view to_string(Comma) { return ","; }

struct Equals {
    constexpr bool operator==(Equals const &) const = default;
};

struct Semicolon {
    constexpr bool operator==(Semicolon const &) const = default;
};

struct Colon {
    constexpr bool operator==(Colon const &) const = default;
};

constexpr std::string_view to_string(Equals) { return "="; }
constexpr std::string_view to_string(Semicolon) { return ";"; }
constexpr std::string_view to_string(Colon) { return ":"; }

struct LParen {
    constexpr bool operator==(LParen const &) const = default;
};

struct RParen {
    constexpr bool operator==(RParen const &) const = default;
};

constexpr std::string_view to_string(LParen) { return "("; }
constexpr std::string_view to_string(RParen) { return ")"; }

struct LBracket {
    constexpr bool operator==(LBracket const &) const = default;
};

struct RBracket {
    constexpr bool operator==(RBracket const &) const = default;
};

constexpr std::string_view to_string(LBracket) { return "["; }
constexpr std::string_view to_string(RBracket) { return "]"; }

struct LBrace {
    constexpr bool operator==(LBrace const &) const = default;
};

struct RBrace {
    constexpr bool operator==(RBrace const &) const = default;
};

constexpr std::string_view to_string(LBrace) { return "{"; }
constexpr std::string_view to_string(RBrace) { return "}"; }

struct Less {
    constexpr bool operator==(Less const &) const = default;
};

struct Greater {
    constexpr bool operator==(Greater const &) const = default;
};

struct GreaterOrEqual {
    constexpr bool operator==(GreaterOrEqual const &) const = default;
};

struct LessOrEqual {
    constexpr bool operator==(LessOrEqual const &) const = default;
};

struct EqualEqual {
    constexpr bool operator==(EqualEqual const &) const = default;
};

struct NotEqual {
    constexpr bool operator==(NotEqual const &) const = default;
};

constexpr std::string_view to_string(Less) { return "<"; }
constexpr std::string_view to_string(Greater) { return ">"; }
constexpr std::string_view to_string(GreaterOrEqual) { return ">="; }
constexpr std::string_view to_string(LessOrEqual) { return "<="; }
constexpr std::string_view to_string(EqualEqual) { return "=="; }
constexpr std::string_view to_string(NotEqual) { return "!="; }

struct PlusEquals {
    constexpr bool operator==(PlusEquals const &) const = default;
};

struct MinusEquals {
    constexpr bool operator==(MinusEquals const &) const = default;
};

struct StarEquals {
    constexpr bool operator==(StarEquals const &) const = default;
};

struct SlashEquals {
    constexpr bool operator==(SlashEquals const &) const = default;
};

struct DoubleSlashEquals {
    constexpr bool operator==(DoubleSlashEquals const &) const = default;
};

struct PercentEquals {
    constexpr bool operator==(PercentEquals const &) const = default;
};

struct AmpersandEquals {
    constexpr bool operator==(AmpersandEquals const &) const = default;
};

struct PipeEquals {
    constexpr bool operator==(PipeEquals const &) const = default;
};

struct CaretEquals {
    constexpr bool operator==(CaretEquals const &) const = default;
};

struct LShiftEquals {
    constexpr bool operator==(LShiftEquals const &) const = default;
};

struct RShiftEquals {
    constexpr bool operator==(RShiftEquals const &) const = default;
};

constexpr std::string_view to_string(PlusEquals) { return "+="; }
constexpr std::string_view to_string(MinusEquals) { return "-="; }
constexpr std::string_view to_string(StarEquals) { return "*="; }
constexpr std::string_view to_string(SlashEquals) { return "/="; }
constexpr std::string_view to_string(DoubleSlashEquals) { return "//="; }
constexpr std::string_view to_string(PercentEquals) { return "%="; }
constexpr std::string_view to_string(AmpersandEquals) { return "&="; }
constexpr std::string_view to_string(PipeEquals) { return "|="; }
constexpr std::string_view to_string(CaretEquals) { return "^="; }
constexpr std::string_view to_string(LShiftEquals) { return "<<="; }
constexpr std::string_view to_string(RShiftEquals) { return ">>="; }

struct And {
    constexpr bool operator==(And const &) const = default;
};

struct Else {
    constexpr bool operator==(Else const &) const = default;
};

struct Load {
    constexpr bool operator==(Load const &) const = default;
};

struct Break {
    constexpr bool operator==(Break const &) const = default;
};

struct For {
    constexpr bool operator==(For const &) const = default;
};

struct Not {
    constexpr bool operator==(Not const &) const = default;
};

struct Continue {
    constexpr bool operator==(Continue const &) const = default;
};

struct If {
    constexpr bool operator==(If const &) const = default;
};

struct Or {
    constexpr bool operator==(Or const &) const = default;
};

struct Def {
    constexpr bool operator==(Def const &) const = default;
};

struct In {
    constexpr bool operator==(In const &) const = default;
};

struct Pass {
    constexpr bool operator==(Pass const &) const = default;
};

struct Elif {
    constexpr bool operator==(Elif const &) const = default;
};

struct Lambda {
    constexpr bool operator==(Lambda const &) const = default;
};

struct Return {
    constexpr bool operator==(Return const &) const = default;
};

constexpr std::string_view to_string(And) { return "and"; }
constexpr std::string_view to_string(Else) { return "else"; }
constexpr std::string_view to_string(Load) { return "load"; }
constexpr std::string_view to_string(Break) { return "break"; }
constexpr std::string_view to_string(For) { return "for"; }
constexpr std::string_view to_string(Not) { return "not"; }
constexpr std::string_view to_string(Continue) { return "continue"; }
constexpr std::string_view to_string(If) { return "if"; }
constexpr std::string_view to_string(Or) { return "or"; }
constexpr std::string_view to_string(Def) { return "def"; }
constexpr std::string_view to_string(In) { return "in"; }
constexpr std::string_view to_string(Pass) { return "pass"; }
constexpr std::string_view to_string(Elif) { return "elif"; }
constexpr std::string_view to_string(Lambda) { return "lambda"; }
constexpr std::string_view to_string(Return) { return "return"; }

struct Identifier {
    std::string name;
    constexpr bool operator==(Identifier const &) const = default;
};

constexpr std::string_view to_string(Identifier const &id) { return id.name; }

struct IntLiteral {
    std::int64_t value{};
    constexpr bool operator==(IntLiteral const &) const = default;
};

inline std::string to_string(IntLiteral const &i) { return std::to_string(i.value); }

struct StringLiteral {
    std::string value;
    constexpr bool operator==(StringLiteral const &) const = default;
};

inline std::string to_string(StringLiteral const &str) { return std::format(R"("{}")", str.value); }

struct Newline {
    constexpr bool operator==(Newline const &) const = default;
};

constexpr std::string_view to_string(Newline const &) { return "<newline>"; }

struct Eof {
    constexpr bool operator==(Eof const &) const = default;
};

constexpr std::string_view to_string(Eof const &) { return "<eof>"; }

} // namespace token

using Token = std::variant<
    token::Plus,
    token::Minus,
    token::Star,
    token::Slash,
    token::DoubleSlash,
    token::Percent,
    token::DoubleStar,
    token::Tilde,
    token::Ampersand,
    token::Pipe,
    token::Caret,
    token::LShift,
    token::RShift,
    token::Dot,
    token::Comma,
    token::Equals,
    token::Semicolon,
    token::Colon,
    token::LParen,
    token::RParen,
    token::LBracket,
    token::RBracket,
    token::LBrace,
    token::RBrace,
    token::Less,
    token::Greater,
    token::GreaterOrEqual,
    token::LessOrEqual,
    token::EqualEqual,
    token::NotEqual,
    token::PlusEquals,
    token::MinusEquals,
    token::StarEquals,
    token::SlashEquals,
    token::DoubleSlashEquals,
    token::PercentEquals,
    token::AmpersandEquals,
    token::PipeEquals,
    token::CaretEquals,
    token::LShiftEquals,
    token::RShiftEquals,
    token::And,
    token::Else,
    token::Load,
    token::Break,
    token::For,
    token::Not,
    token::Continue,
    token::If,
    token::Or,
    token::Def,
    token::In,
    token::Pass,
    token::Elif,
    token::Lambda,
    token::Return,
    token::Identifier,
    token::IntLiteral,
    token::StringLiteral,
    token::Newline,
    token::Eof>;

inline std::string to_string(Token const &token) {
    return std::visit([](auto &&arg) { return std::string{to_string(arg)}; }, token);
}

} // namespace starlark

#endif // STARLARK_TOKEN_H_
