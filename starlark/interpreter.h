// SPDX-FileCopyrightText: 2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#ifndef STARLARK_INTERPRETER_H_
#define STARLARK_INTERPRETER_H_

#include "starlark/ast.h"

#include <algorithm>
#include <cassert>
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

using Dict = std::vector<std::pair<Value, Value>>;

enum class Error {
    UnknownIdentifier,
    BadArgument,
    DuplicateDictKey,
    InvalidSliceTarget,
    InvalidSliceIndex,
    SliceIndexOutOfBounds,
    NotCallable,
    NotIterable,
    ModuleLoaderUnavailable,
    ModuleLoadFailed,
    MissingExport,
};

constexpr std::string_view to_string(Error e) {
    switch (e) {
    case Error::UnknownIdentifier:
        return "UnknownIdentifier";
    case Error::BadArgument:
        return "BadArgument";
    case Error::DuplicateDictKey:
        return "DuplicateDictKey";
    case Error::InvalidSliceTarget:
        return "InvalidSliceTarget";
    case Error::InvalidSliceIndex:
        return "InvalidSliceIndex";
    case Error::SliceIndexOutOfBounds:
        return "SliceIndexOutOfBounds";
    case Error::NotCallable:
        return "NotCallable";
    case Error::NotIterable:
        return "NotIterable";
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
    std::variant<std::int64_t, std::string, std::vector<Value>, Dict, NativeFnPtr> v;
    constexpr bool operator==(Value const &) const;
};

constexpr bool Value::operator==(Value const &) const = default;

enum class ValueType {
    Integer,
    String,
    List,
    Dictionary,
    NativeFunction,
};

constexpr ValueType value_type(Value const &value) {
    if (std::holds_alternative<std::int64_t>(value.v)) {
        return ValueType::Integer;
    }

    if (std::holds_alternative<std::string>(value.v)) {
        return ValueType::String;
    }

    if (std::holds_alternative<std::vector<Value>>(value.v)) {
        return ValueType::List;
    }

    if (std::holds_alternative<Dict>(value.v)) {
        return ValueType::Dictionary;
    }

    return ValueType::NativeFunction;
}

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

        std::string operator()(Dict const &v) {
            std::string result = "{";

            bool first = true;
            for (auto const &[key, value] : v) {
                if (!std::exchange(first, false)) {
                    result += ", ";
                }

                result += to_string(key);
                result += ": ";
                result += to_string(value);
            }

            result += '}';
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

using MemberFunction = std::function<std::expected<Value, Error>(Value const &target)>;

struct Builtins {
    std::map<std::string, Value, std::less<>> identifiers{
        // TODO(robinlinden): Bless these with keywordness.
        {"True", Value{std::int64_t{1}}},
        {"False", Value{std::int64_t{0}}},
    };

    std::map<ValueType, std::map<std::string, MemberFunction, std::less<>>> members{
        {
            ValueType::String,
            {
                {
                    "removesuffix",
                    [](Value const &target) -> std::expected<Value, Error> {
                        assert(std::holds_alternative<std::string>(target.v));

                        return Value{std::make_shared<NativeFn>(
                            [original = std::get<std::string>(target.v)](
                                std::vector<NativeArgument> args) -> std::expected<Value, Error> {
                                if (args.size() != 1) {
                                    return std::unexpected{Error::BadArgument};
                                }

                                auto const *suffix =
                                    std::get_if<std::string>(&args.front().value.v);
                                if (suffix == nullptr) {
                                    return std::unexpected{Error::BadArgument};
                                }

                                if (original.ends_with(*suffix)) {
                                    return Value{
                                        original.substr(0, original.size() - suffix->size())};
                                }

                                return Value{original};
                            })};
                    },
                },
            },
        },
    };
};

class Interpreter {
public:
    std::map<std::string, Value, std::less<>> variables;
    Builtins builtins;
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
        if (it != variables.end()) {
            return it->second;
        }

        auto builtin = builtins.identifiers.find(ident.name);
        if (builtin != builtins.identifiers.end()) {
            return builtin->second;
        }

        return std::unexpected{Error::UnknownIdentifier};
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

    std::expected<Value, Error> run(MemberExpr const &member) {
        auto target = run(*member.target);
        if (!target) {
            return std::unexpected{target.error()};
        }

        auto type_members = builtins.members.find(value_type(*target));
        if (type_members == builtins.members.end()) {
            return std::unexpected{Error::UnknownIdentifier};
        }

        auto member_function = type_members->second.find(member.member.name);
        if (member_function == type_members->second.end()) {
            return std::unexpected{Error::UnknownIdentifier};
        }

        return member_function->second(*target);
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
        Dict entries;
        entries.reserve(dict_expr.entries.size());

        for (auto const &[key_expr, value_expr] : dict_expr.entries) {
            auto key = run(key_expr);
            if (!key) {
                return std::unexpected{key.error()};
            }

            auto value = run(value_expr);
            if (!value) {
                return std::unexpected{value.error()};
            }

            auto existing = std::ranges::find(entries, *key, &Dict::value_type::first);
            if (existing != entries.end()) {
                return std::unexpected{Error::DuplicateDictKey};
            }

            entries.emplace_back(std::move(*key), std::move(*value));
        }

        return Value{std::move(entries)};
    }

    std::expected<Value, Error> run(ListComp const &list_comp) {
        auto iterable = run(*list_comp.iterable);
        if (!iterable) {
            return iterable;
        }

        auto const *values = std::get_if<std::vector<Value>>(&iterable->v);
        if (values == nullptr) {
            return std::unexpected{Error::NotIterable};
        }

        auto previous = variables.find(list_comp.iterator_var.name);
        std::optional<Value> previous_value;
        if (previous != variables.end()) {
            previous_value = std::exchange(previous->second, {});
        }

        std::vector<Value> result;
        result.reserve(values->size());
        for (auto const &value : *values) {
            variables[list_comp.iterator_var.name] = value;
            auto element = run(*list_comp.element);
            if (!element) {
                if (previous_value) {
                    variables[list_comp.iterator_var.name] = std::move(*previous_value);
                } else {
                    variables.erase(list_comp.iterator_var.name);
                }

                return element;
            }

            result.push_back(std::move(*element));
        }

        if (previous_value) {
            variables[list_comp.iterator_var.name] = std::move(*previous_value);
        } else {
            variables.erase(list_comp.iterator_var.name);
        }

        return Value{std::move(result)};
    }
};

} // namespace starlark

#endif
