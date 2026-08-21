// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "starlark/parser.h"

#include "starlark/ast.h"

#include "etest/etest2.h"

#include <array>
#include <string>
#include <string_view>
#include <utility>

int main() {
    auto test_cases = std::to_array<std::pair<std::string_view, starlark::Program>>({
        {
            R"(load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test"))",
            starlark::Program{
                .statements{
                    starlark::LoadStmt{
                        .module_name = "@rules_cc//cc:defs.bzl",
                        .symbols{
                            {"cc_library", "cc_library"},
                            {"cc_test", "cc_test"},
                        },
                    },
                },
            },
        },
        {
            R"(load("@rules_cc//cc:defs.bzl", foo = "cc_library"))",
            starlark::Program{
                .statements{
                    starlark::LoadStmt{
                        .module_name = "@rules_cc//cc:defs.bzl",
                        .symbols{
                            {"foo", "cc_library"},
                        },
                    },
                },
            },
        },
        {
            R"("hello world")",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::StringLiteral{.value = "hello world"},
                        },
                    },
                },
            },
        },
        {
            R"(foo)",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{starlark::Identifier{"foo"}},
                    },
                },
            },
        },
        {
            R"(foo("qux", bar = "baz"))",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::CallExpr{
                                .target = std::make_shared<starlark::Expression>(
                                    starlark::Identifier{"foo"}),
                                .args{
                                    {
                                        std::nullopt,
                                        starlark::StringLiteral{"qux"},
                                    },
                                    {
                                        starlark::Identifier{"bar"},
                                        starlark::StringLiteral{"baz"},
                                    },
                                },
                            },
                        },
                    },
                },
            },
        },
        {
            R"(foo(qux(), bar = baz))",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::CallExpr{
                                .target = std::make_shared<starlark::Expression>(
                                    starlark::Identifier{"foo"}),
                                .args{
                                    {
                                        std::nullopt,
                                        starlark::CallExpr{
                                            .target = std::make_shared<starlark::Expression>(
                                                starlark::Identifier{"qux"}),
                                            .args{},
                                        },
                                    },
                                    {
                                        starlark::Identifier{"bar"},
                                        starlark::Identifier{"baz"},
                                    },
                                },
                            },
                        },
                    },
                },
            },
        },
        {
            R"(["foo", bar, baz()])",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::ListExpr{
                                .elements{
                                    starlark::StringLiteral{"foo"},
                                    starlark::Identifier{"bar"},
                                    starlark::CallExpr{
                                        .target = std::make_shared<starlark::Expression>(
                                            starlark::Identifier{"baz"}),
                                        .args{},
                                    },
                                },
                            },
                        },
                    },
                },
            },
        },
        {
            R"({"foo": bar, baz(): "qux"})",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::DictExpr{
                                .entries{
                                    {
                                        starlark::StringLiteral{"foo"},
                                        starlark::Identifier{"bar"},
                                    },
                                    {
                                        starlark::CallExpr{
                                            .target = std::make_shared<starlark::Expression>(
                                                starlark::Identifier{"baz"}),
                                            .args{},
                                        },
                                        starlark::StringLiteral{"qux"},
                                    },
                                },
                            },
                        },
                    },
                },
            },
        },
        {
            "{\n42\n:\nbar\n,\nbaz()\n:\n13\n}",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::DictExpr{
                                .entries{
                                    {
                                        starlark::IntLiteral{42},
                                        starlark::Identifier{"bar"},
                                    },
                                    {
                                        starlark::CallExpr{
                                            .target = std::make_shared<starlark::Expression>(
                                                starlark::Identifier{"baz"}),
                                            .args{},
                                        },
                                        starlark::IntLiteral{13},
                                    },
                                },
                            },
                        },
                    },
                },
            },
        },
        {
            "{}",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::DictExpr{
                                .entries{},
                            },
                        },
                    },
                },
            },
        },
        {
            "[]",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::ListExpr{
                                .elements{},
                            },
                        },
                    },
                },
            },
        },
        {
            R"(["foo"])",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::ListExpr{
                                .elements{
                                    starlark::Expression{starlark::StringLiteral{"foo"}},
                                },
                            },
                        },
                    },
                },
            },
        },
        {
            "[x, y, z]",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::ListExpr{
                                .elements{
                                    starlark::Expression{starlark::Identifier{"x"}},
                                    starlark::Expression{starlark::Identifier{"y"}},
                                    starlark::Expression{starlark::Identifier{"z"}},
                                },
                            },
                        },
                    },
                },
            },
        },
        {
            "[\nx\n,     \n        y           \n         ,\nz\n]",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::ListExpr{
                                .elements{
                                    starlark::Expression{starlark::Identifier{"x"}},
                                    starlark::Expression{starlark::Identifier{"y"}},
                                    starlark::Expression{starlark::Identifier{"z"}},
                                },
                            },
                        },
                    },
                },
            },
        },
        {
            "[x for x in y]",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::ListComp{
                                .element{std::make_shared<starlark::Expression>(
                                    starlark::Identifier{"x"})},
                                .iterator_var{"x"},
                                .iterable = std::make_shared<starlark::Expression>(
                                    starlark::Identifier{"y"}),
                            },
                        },
                    },
                },
            },
        },
        {
            R"([x for x in ["a", "b"]])",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::ListComp{
                                .element{std::make_shared<starlark::Expression>(
                                    starlark::Identifier{"x"})},
                                .iterator_var{"x"},
                                .iterable =
                                    std::make_shared<starlark::Expression>(starlark::ListExpr{
                                        .elements{
                                            starlark::Expression{starlark::StringLiteral{"a"}},
                                            starlark::Expression{starlark::StringLiteral{"b"}},
                                        },
                                    }),
                            },
                        },
                    },
                },
            },
        },
        {
            "A = B",
            starlark::Program{
                .statements{
                    starlark::AssignStmt{
                        .target = starlark::Identifier{"A"},
                        .value = starlark::Identifier{"B"},
                    },
                },
            },
        },
        {
            "def foo(x, y):\n    z = x\n    y\n",
            starlark::Program{
                .statements{
                    starlark::DefStmt{
                        .name = starlark::Identifier{"foo"},
                        .params{
                            starlark::Identifier{"x"},
                            starlark::Identifier{"y"},
                        },
                        .body{
                            starlark::AssignStmt{
                                .target = starlark::Identifier{"z"},
                                .value = starlark::Identifier{"x"},
                            },
                            starlark::ExpressionStmt{.expr = starlark::Identifier{"y"}},
                        },
                    },
                },
            },
        },
        {
            "def foo():\n    1\n",
            starlark::Program{
                .statements{
                    starlark::DefStmt{
                        .name = starlark::Identifier{"foo"},
                        .params{},
                        .body{
                            starlark::ExpressionStmt{.expr = starlark::IntLiteral{1}},
                        },
                    },
                },
            },
        },
        {
            "def foo():\n    def bar():\n        1\n    2\n",
            starlark::Program{
                .statements{
                    starlark::DefStmt{
                        .name = starlark::Identifier{"foo"},
                        .params{},
                        .body{
                            starlark::DefStmt{
                                .name = starlark::Identifier{"bar"},
                                .params{},
                                .body{
                                    starlark::ExpressionStmt{.expr = starlark::IntLiteral{1}},
                                },
                            },
                            starlark::ExpressionStmt{.expr = starlark::IntLiteral{2}},
                        },
                    },
                },
            },
        },
        {
            R"("foo"())",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::CallExpr{
                                .target = std::make_shared<starlark::Expression>(
                                    starlark::StringLiteral{"foo"}),
                                .args{},
                            },
                        },
                    },
                },
            },
        },
        {
            "42",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{starlark::IntLiteral{42}},
                    },
                },
            },
        },
        {
            "42\n",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{starlark::IntLiteral{42}},
                    },
                },
            },
        },
        {
            "42[5]",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::SliceExpr{
                                .target = std::make_shared<starlark::Expression>(
                                    starlark::IntLiteral{42}),
                                .index =
                                    std::make_shared<starlark::Expression>(starlark::IntLiteral{5}),
                            },
                        },
                    },
                },
            },
        },
        {
            "42\n[5]",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{starlark::IntLiteral{42}},
                    },
                    starlark::ExpressionStmt{.expr{
                        starlark::ListExpr{
                            .elements{
                                starlark::Expression{starlark::IntLiteral{5}},
                            },
                        },
                    }},
                },
            },
        },
        {
            "foo.bar",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::MemberExpr{
                                .target = std::make_shared<starlark::Expression>(
                                    starlark::Identifier{"foo"}),
                                .member = starlark::Identifier{"bar"},
                            },
                        },
                    },
                },
            },
        },
        {
            "foo.baz[3].qux().bar",
            starlark::Program{
                .statements{
                    starlark::ExpressionStmt{
                        .expr{
                            starlark::MemberExpr{
                                .target = std::make_shared<starlark::Expression>(starlark::CallExpr{
                                    .target =
                                        std::make_shared<starlark::Expression>(starlark::MemberExpr{
                                            .target = std::make_shared<
                                                starlark::Expression>(starlark::SliceExpr{
                                                .target = std::make_shared<starlark::Expression>(
                                                    starlark::MemberExpr{
                                                        .target =
                                                            std::make_shared<starlark::Expression>(
                                                                starlark::Identifier{"foo"}),
                                                        .member = starlark::Identifier{"baz"},
                                                    }),
                                                .index = std::make_shared<starlark::Expression>(
                                                    starlark::IntLiteral{3}),
                                            }),
                                            .member = starlark::Identifier{"qux"},
                                        }),
                                    .args{},
                                }),
                                .member = starlark::Identifier{"bar"},
                            },
                        },
                    },
                },
            },
        },
    });

    // TODO(robinlinden): Return error codes from parser and use that here.
    static constexpr auto kExpectedParseFailures = std::to_array<std::string_view>({
        // CallExpr
        // Missing closing parenthesis.
        R"(foo("qux")",
        // Positional argument after kw argument.
        R"(foo(bar = baz, qux()))",

        // ListExpr
        // Tokenization error.
        R"([")",
        // Parse error in element.
        R"([not)",
        // Tokenization error after element.
        R"(["foo" ")",
        // Unexpected token after element.
        R"(["foo" foo])",

        // DictExpr
        // Tokenization error.
        R"({")",
        // Parse error in key.
        R"({not: "value"})",
        // Missing colon after key.
        R"({"key" "value"})",
        // Tokenization error in value.
        R"({"key": ")",
        // Parse error in value.
        R"({"key": not})",
        // Tokenization error after entry.
        R"({"key": "value" ")",
        // Unexpected token after entry.
        R"({"key": "value" foo})",

        // ListComp
        // Abrupt end of input.
        "[e for",
        "[e for y",
        "[e for y in",
        "[e for y in z",
        // Parse error in iterable expression.
        "[e for y in '",

        // AssignStmt
        // Tokenization error in value.
        "A = \"",
        // Parse error in value.
        "A = foo(",
        // Non-ident target.
        "\"A\" = B",

        // MemberExpr
        // Missing member name.
        "foo.",
        // Invalid member name.
        "foo.5",

        // ExpressionStmt
        // Statements must be newline-separated.
        "42 42",

        // Bad indentation.
        "    x = 1",
    });

    etest::Suite s{};

    for (auto &[input, expected] : test_cases) {
        s.add_test(std::string{input}, [input, expected](etest::IActions &a) {
            auto program = starlark::parse(input);
            a.require(program.has_value());
            a.expect_eq(*program, expected);
        });
    }

    for (auto const &input : kExpectedParseFailures) {
        s.add_test(std::string{input}, [input](etest::IActions &a) {
            auto program = starlark::parse(input);
            a.expect(!program.has_value());
        });
    }

    return s.run();
}
