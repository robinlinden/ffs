// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "starlark/parser.h"

#include "etest/etest2.h"

#include <array>
#include <string>
#include <string_view>
#include <utility>

int main() {
  auto test_cases =
      std::to_array<std::pair<std::string_view, starlark::Program>>({
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
              R"(foo(bar = "baz", "qux"))",
              starlark::Program{
                  .statements{
                      starlark::ExpressionStmt{
                          .expr{
                              starlark::CallExpr{
                                  .target = "foo",
                                  .args{
                                      {
                                          starlark::Identifier{"bar"},
                                          starlark::StringLiteral{"baz"},
                                      },
                                      {
                                          std::nullopt,
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
              R"(foo(bar = baz, qux()))",
              starlark::Program{
                  .statements{
                      starlark::ExpressionStmt{
                          .expr{
                              starlark::CallExpr{
                                  .target = "foo",
                                  .args{
                                      {
                                          starlark::Identifier{"bar"},
                                          starlark::Identifier{"baz"},
                                      },
                                      {
                                          std::nullopt,
                                          starlark::CallExpr{
                                              .target = "qux",
                                              .args{},
                                          },
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
                                          .target = "baz",
                                          .args{},
                                      },
                                  },
                              },
                          },
                      },
                  },
              },
          },
      });

  // TODO(robinlinden): Return error codes from parser and use that here.
  static constexpr auto kExpectedParseFailures =
      std::to_array<std::string_view>({
          // CallExpr
          // Missing closing parenthesis.
          R"(foo(bar = "baz", "qux")",

          // ListExpr
          // Tokenization error.
          R"([")",
          // Parse error in element.
          R"([not)",
          // Tokenization error after element.
          R"(["foo" ")",
          // Unexpected token after element.
          R"(["foo" foo])",
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
