// SPDX-FileCopyrightText: 2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STARLARK_INTERPRETER_H_
#define STARLARK_INTERPRETER_H_

#include "starlark/ast.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace starlark {

struct Value {
    std::variant<std::int64_t, std::string, std::vector<Value>> v;
    constexpr bool operator==(Value const &) const = default;
};

// TODO(robinlinden): Error-handling.
class Interpreter {
  public:
    std::map<std::string, Value> variables;

    std::optional<Value> run(Program const &program) {
        std::optional<Value> result;

        for (auto const &stmt : program.statements) {
            result = std::visit([this](auto const &s) { return run(s); }, stmt);
        }

        return result;
    }

    std::optional<Value> run(auto const &) {
        // TODO(robinlinden): Delete this.
        return std::nullopt;
    }

    std::optional<Value> run(Statement const &stmt) {
        return std::visit([this](auto const &s) { return run(s); }, stmt);
    }

    std::optional<Value> run(AssignStmt const &stmt) {
        auto value = run(stmt.value);
        if (!value) {
            return std::nullopt;
        }

        return variables[stmt.target.name] = *value;
    }

    std::optional<Value> run(ExpressionStmt const &stmt) { return run(stmt.expr); }

    std::optional<Value> run(Expression const &expr) {
        return std::visit([this](auto const &e) { return run(e); }, expr);
    }

    std::optional<Value> run(Identifier const &ident) {
        auto it = variables.find(ident.name);
        if (it == variables.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    std::optional<Value> run(StringLiteral const &str) { return Value{str.value}; }
    std::optional<Value> run(IntLiteral const &i) { return Value{i.value}; }

    std::optional<Value> run(ListExpr const &list) {
        std::vector<Value> elements;
        elements.reserve(list.elements.size());

        for (auto const &elem : list.elements) {
            auto value = run(elem);
            if (!value) {
                return std::nullopt;
            }

            elements.push_back(std::move(*value));
        }

        return Value{.v = std::move(elements)};
    }

    std::optional<Value> run(SliceExpr const &se) {
        auto const target = run(*se.target);
        if (!target) {
            return std::nullopt;
        }

        if (auto const *str = std::get_if<std::string>(&target->v); str != nullptr) {
            auto maybe_idx = run(*se.index);
            if (!maybe_idx) {
                return std::nullopt;
            }

            auto const *idx = std::get_if<std::int64_t>(&maybe_idx->v);
            if (idx == nullptr || *idx < 0) {
                return std::nullopt;
            }

            if (static_cast<std::uint32_t>(*idx) >= str->size()) {
                return std::nullopt;
            }

            auto const char_res = (*str)[static_cast<std::uint32_t>(*idx)];
            return Value{std::string{char_res}};
        }

        if (auto const *list = std::get_if<std::vector<Value>>(&target->v); list != nullptr) {
            auto maybe_idx = run(*se.index);
            if (!maybe_idx) {
                return std::nullopt;
            }

            auto const *idx = std::get_if<std::int64_t>(&maybe_idx->v);
            if (idx == nullptr || *idx < 0) {
                return std::nullopt;
            }

            if (static_cast<std::uint32_t>(*idx) >= list->size()) {
                return std::nullopt;
            }

            return Value{(*list)[static_cast<std::uint32_t>(*idx)]};
        }

        return std::nullopt;
    }
};

} // namespace starlark

#endif
