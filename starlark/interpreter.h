// SPDX-FileCopyrightText: 2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STARLARK_INTERPRETER_H_
#define STARLARK_INTERPRETER_H_

#include "starlark/ast.h"

#include <cstdint>
#include <expected>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace starlark {

struct Value;
struct NativeArgument;

enum class Error {
    UnknownIdentifier,
    InvalidSliceTarget,
    InvalidSliceIndex,
    SliceIndexOutOfBounds,
    NotCallable,
    ModuleLoaderUnavailable,
    ModuleLoadFailed,
    MissingExport,
};

constexpr std::string_view to_string(Error e) {
    switch (e) {
    case Error::UnknownIdentifier:
        return "UnknownIdentifier";
    case Error::InvalidSliceTarget:
        return "InvalidSliceTarget";
    case Error::InvalidSliceIndex:
        return "InvalidSliceIndex";
    case Error::SliceIndexOutOfBounds:
        return "SliceIndexOutOfBounds";
    case Error::NotCallable:
        return "NotCallable";
    case Error::ModuleLoaderUnavailable:
        return "ModuleLoaderUnavailable";
    case Error::ModuleLoadFailed:
        return "ModuleLoadFailed";
    case Error::MissingExport:
        return "MissingExport";
    }

    return "<unknown>";
}

using NativeFn = std::function<std::expected<Value, Error>(std::vector<NativeArgument>)>;
using NativeFnPtr = std::shared_ptr<NativeFn>;

struct Value {
    std::variant<std::int64_t, std::string, std::vector<Value>, NativeFnPtr> v;
    constexpr bool operator==(Value const &) const = default;
};

constexpr std::string to_string(Value const &value) {
    struct ValueStringifier {
        std::string operator()(std::int64_t v) { return std::to_string(v); }
        std::string operator()(std::string const &v) { return v; }
        std::string operator()(std::vector<Value> const &v) {
            std::string result = "[";

            bool first = true;
            for (auto const &elem : v) {
                if (!std::exchange(first, false)) {
                    result += ", ";
                }

                result += to_string(elem);
            }

            result += ']';
            return result;
        }

        std::string operator()(NativeFnPtr const &) { return "[NativeFunction]"; }
    };

    return std::visit(ValueStringifier{}, value.v);
}

struct NativeArgument {
    std::optional<std::string> name;
    Value value;
    constexpr bool operator==(NativeArgument const &) const = default;
};

using ModuleExports = std::map<std::string, Value, std::less<>>;
using ModuleLoader = std::function<std::optional<ModuleExports>(std::string_view)>;

class Interpreter {
public:
    std::map<std::string, Value, std::less<>> variables;
    ModuleLoader module_loader;

    std::expected<Value, Error> run(Program const &program) {
        std::expected<Value, Error> result = Value{};

        for (auto const &stmt : program.statements) {
            auto stmt_result = std::visit([this](auto const &s) { return run(s); }, stmt);
            if (!stmt_result) {
                return std::unexpected{stmt_result.error()};
            }

            result = std::move(stmt_result);
        }

        return result;
    }

    std::expected<Value, Error> run(AssignStmt const &stmt) {
        auto value = run(stmt.value);
        if (!value) {
            return std::unexpected{value.error()};
        }

        variables[stmt.target.name] = *value;
        return *value;
    }

    std::expected<Value, Error> run(ExpressionStmt const &stmt) { return run(stmt.expr); }

    std::expected<Value, Error> run(Expression const &expr) {
        return std::visit([this](auto const &e) { return run(e); }, expr);
    }

    std::expected<Value, Error> run(Identifier const &ident) {
        auto it = variables.find(ident.name);
        if (it == variables.end()) {
            return std::unexpected{Error::UnknownIdentifier};
        }

        return it->second;
    }

    std::expected<Value, Error> run(StringLiteral const &str) { return Value{str.value}; }
    std::expected<Value, Error> run(IntLiteral const &i) { return Value{i.value}; }

    std::expected<Value, Error> run(ListExpr const &list) {
        std::vector<Value> elements;
        elements.reserve(list.elements.size());

        for (auto const &elem : list.elements) {
            auto value = run(elem);
            if (!value) {
                return std::unexpected{value.error()};
            }

            elements.push_back(std::move(*value));
        }

        return Value{.v = std::move(elements)};
    }

    std::expected<Value, Error> run(SliceExpr const &se) {
        auto const target = run(*se.target);
        if (!target) {
            return std::unexpected{target.error()};
        }

        if (auto const *str = std::get_if<std::string>(&target->v); str != nullptr) {
            auto maybe_idx = run(*se.index);
            if (!maybe_idx) {
                return std::unexpected{maybe_idx.error()};
            }

            auto const *idx = std::get_if<std::int64_t>(&maybe_idx->v);
            if (idx == nullptr || *idx < 0) {
                return std::unexpected{Error::InvalidSliceIndex};
            }

            if (static_cast<std::uint32_t>(*idx) >= str->size()) {
                return std::unexpected{Error::SliceIndexOutOfBounds};
            }

            auto const char_res = (*str)[static_cast<std::uint32_t>(*idx)];
            return Value{std::string{char_res}};
        }

        if (auto const *list = std::get_if<std::vector<Value>>(&target->v); list != nullptr) {
            auto maybe_idx = run(*se.index);
            if (!maybe_idx) {
                return std::unexpected{maybe_idx.error()};
            }

            auto const *idx = std::get_if<std::int64_t>(&maybe_idx->v);
            if (idx == nullptr || *idx < 0) {
                return std::unexpected{Error::InvalidSliceIndex};
            }

            if (static_cast<std::uint32_t>(*idx) >= list->size()) {
                return std::unexpected{Error::SliceIndexOutOfBounds};
            }

            return Value{(*list)[static_cast<std::uint32_t>(*idx)]};
        }

        return std::unexpected{Error::InvalidSliceTarget};
    }

    std::expected<Value, Error> run(CallExpr const &ce) {
        auto maybe_target = run(*ce.target);
        if (!maybe_target) {
            return std::unexpected{maybe_target.error()};
        }

        auto const *target = std::get_if<NativeFnPtr>(&maybe_target->v);
        if (target == nullptr) {
            return std::unexpected{Error::NotCallable};
        }

        std::vector<NativeArgument> native_args;
        native_args.reserve(ce.args.size());
        for (auto const &arg : ce.args) {
            auto name = [&] -> std::optional<std::string> {
                if (!arg.name.has_value()) {
                    return std::nullopt;
                }

                return arg.name->name;
            }();

            auto val = run(arg.expr);
            if (!val) {
                return std::unexpected{val.error()};
            }

            native_args.emplace_back(std::move(name), std::move(*val));
        }

        return (**target)(std::move(native_args));
    }

    std::expected<Value, Error> run(LoadStmt const &load) {
        if (!module_loader) {
            return std::unexpected{Error::ModuleLoaderUnavailable};
        }

        auto exports = module_loader(load.module_name);
        if (!exports) {
            return std::unexpected{Error::ModuleLoadFailed};
        }

        std::vector<std::pair<std::string, Value>> bindings;
        bindings.reserve(load.symbols.size());
        for (auto const &[local_name, exported_name] : load.symbols) {
            auto exported = exports->find(exported_name);
            if (exported == exports->end()) {
                return std::unexpected{Error::MissingExport};
            }

            bindings.emplace_back(local_name, exported->second);
        }

        for (auto &[local_name, value] : bindings) {
            variables[local_name] = std::move(value);
        }

        return Value{};
    }

    std::expected<Value, Error> run(DefStmt const &def) {
        std::ignore = def; // TODO(robinlinden): Implement.
        return Value{};
    }

    std::expected<Value, Error> run(DictExpr const &dict_expr) {
        std::ignore = dict_expr; // TODO(robinlinden): Implement.
        return Value{};
    }

    std::expected<Value, Error> run(ListComp const &list_comp) {
        std::ignore = list_comp; // TODO(robinlinden): Implement.
        return Value{};
    }
};

} // namespace starlark

#endif
