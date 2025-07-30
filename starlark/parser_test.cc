// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
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
      });

  etest::Suite s{};

  for (auto &[input, expected] : test_cases) {
    s.add_test(std::string{input}, [input, expected](etest::IActions &a) {
      auto program = starlark::parse(input);
      a.require(program.has_value());
      a.expect_eq(*program, expected);
    });
  }

  return s.run();
}
