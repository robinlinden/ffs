// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STARLARK_AST_H_
#define STARLARK_AST_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace starlark {

struct StringLiteral {
    std::string value;
    constexpr bool operator==(StringLiteral const &) const = default;
};

struct IntLiteral {
    std::int64_t value{};
    constexpr bool operator==(IntLiteral const &) const = default;
};

struct Identifier {
    std::string name;
    constexpr bool operator==(Identifier const &) const = default;
};

struct CallExpr;
struct DictExpr;
struct ListExpr;
struct ListComp;
struct SliceExpr;
struct MemberExpr;

using Expression = std::variant<
    CallExpr,
    StringLiteral,
    IntLiteral,
    Identifier,
    ListComp,
    ListExpr,
    DictExpr,
    SliceExpr,
    MemberExpr>;

struct Argument;

struct CallExpr {
    std::shared_ptr<Expression> target;
    std::vector<Argument> args;
    constexpr bool operator==(CallExpr const &) const;
};

struct DictExpr {
    std::vector<std::pair<Expression, Expression>> entries;
    // Clang 18 dies if this is defaulted.
    constexpr bool operator==(DictExpr const &) const;
};

struct SliceExpr {
    std::shared_ptr<Expression> target;
    std::shared_ptr<Expression> index;
    constexpr bool operator==(SliceExpr const &) const;
};

struct MemberExpr {
    std::shared_ptr<Expression> target;
    Identifier member;
    constexpr bool operator==(MemberExpr const &) const;
};

// TODO(robinlinden): shared_ptr is silly here, but right now the ast has to be
// copyable for some reason.
struct ListComp {
    std::shared_ptr<Expression> element;
    Identifier iterator_var;
    std::shared_ptr<Expression> iterable;
    constexpr bool operator==(ListComp const &) const;
};

struct ListExpr {
    std::vector<Expression> elements;
    constexpr bool operator==(ListExpr const &) const = default;
};

struct Argument {
    std::optional<Identifier> name;
    Expression expr;
    constexpr bool operator==(Argument const &) const = default;
};

constexpr bool CallExpr::operator==(CallExpr const &o) const {
    return *target == *o.target && args == o.args;
}

constexpr bool DictExpr::operator==(DictExpr const &o) const { return entries == o.entries; }

constexpr bool SliceExpr::operator==(SliceExpr const &o) const {
    return *target == *o.target && *index == *o.index;
}

constexpr bool ListComp::operator==(ListComp const &o) const {
    return *element == *o.element && iterator_var == o.iterator_var && *iterable == *o.iterable;
}

constexpr bool MemberExpr::operator==(MemberExpr const &o) const {
    return *target == *o.target && member == o.member;
}

struct AssignStmt {
    Identifier target;
    Expression value;
    constexpr bool operator==(AssignStmt const &) const = default;
};

struct ExpressionStmt {
    Expression expr;
    constexpr bool operator==(ExpressionStmt const &) const = default;
};

struct LoadStmt {
    std::string module_name;
    std::vector<std::pair<std::string, std::string>> symbols;
    constexpr bool operator==(LoadStmt const &) const = default;
};

using Statement = std::variant<AssignStmt, LoadStmt, ExpressionStmt>;

struct Program {
    std::vector<Statement> statements;
    constexpr bool operator==(Program const &) const = default;
};

} // namespace starlark

#endif // STARLARK_AST_H_
