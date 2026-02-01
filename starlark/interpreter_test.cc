// SPDX-FileCopyrightText: 2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "starlark/interpreter.h"

#include "starlark/ast.h"

#include "etest/etest2.h"

auto run(auto starlark) { return starlark::Interpreter{}.run(starlark); }

int main() {
    using namespace starlark;

    etest::Suite s{};

    s.add_test("Program", [](etest::IActions &a) {
        Program program{
            .statements{
                AssignStmt{
                    .target = Identifier{"B"},
                    .value = StringLiteral{"hello"},
                },
                AssignStmt{
                    .target = Identifier{"A"},
                    .value = Identifier{"B"},
                },
                ExpressionStmt{
                    .expr = Identifier{"A"},
                },
            },
        };

        a.expect_eq(run(program), Value{std::string{"hello"}});

        // Empty programs are fine too.
        a.expect_eq(run(Program{.statements{}}), std::nullopt);
    });

    s.add_test("AssignStmt", [](etest::IActions &a) {
        auto stmt = AssignStmt{
            .target = Identifier{"foo"},
            .value = StringLiteral{"bar"},
        };

        a.expect_eq(run(stmt), Value{std::string{"bar"}});
    });

    s.add_test("ExpressionStmt", [](etest::IActions &a) {
        auto stmt = ExpressionStmt{
            .expr = StringLiteral{"hello world"},
        };

        a.expect_eq(run(stmt), Value{std::string{"hello world"}});
    });

    s.add_test("Expression", [](etest::IActions &a) {
        auto expr = Expression{StringLiteral{"hello world"}};
        a.expect_eq(run(expr), Value{std::string{"hello world"}});
    });

    s.add_test("Identifier", [](etest::IActions &a) {
        Interpreter interpreter{};
        interpreter.variables["greeting"] = Value{std::string{"hello"}};

        auto ident = Identifier{"greeting"};
        a.expect_eq(interpreter.run(ident), Value{std::string{"hello"}});

        auto missing_ident = Identifier{"missing"};
        a.expect_eq(interpreter.run(missing_ident).has_value(), false);
    });

    s.add_test("StringLiteral", [](etest::IActions &a) {
        auto str_lit = StringLiteral{"hello world"};
        a.expect_eq(run(str_lit), Value{std::string{"hello world"}});
    });

    s.add_test("ListExpr", [](etest::IActions &a) {
        auto list_expr = ListExpr{
            .elements{
                Expression{StringLiteral{"foo"}},
                Expression{StringLiteral{"bar"}},
                Expression{StringLiteral{"baz"}},
            },
        };

        a.expect_eq(
            run(list_expr),
            Value{std::vector<Value>{
                Value{std::string{"foo"}},
                Value{std::string{"bar"}},
                Value{std::string{"baz"}},
            }});
    });

    return s.run();
}
