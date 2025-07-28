// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "starlark/tokenizer.h"

#include "starlark/token.h"

#include "etest/etest2.h"

#include <array>
#include <cassert>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace {

std::string to_string(std::span<starlark::Token const> tokens) {
  assert(!tokens.empty());

  std::stringstream ss;
  ss << starlark::to_string(tokens[0]);
  tokens = tokens.subspan(1);

  for (auto const &token : tokens) {
    ss << ' ' << starlark::to_string(token);
  }

  return std::move(ss).str();
}

} // namespace

constexpr auto kTestCases =
    std::to_array<std::pair<std::string_view, std::string_view>>({
        {
            R"(load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test"))",
            R"(load ( "@rules_cc//cc:defs.bzl" , "cc_library" , "cc_test" ))",
        },
    });

int main() {
  etest::Suite s{};

  for (const auto &[input, expected] : kTestCases) {
    s.add_test(std::string{input}, [input, expected](etest::IActions &a) {
      auto tokens = starlark::tokenize(input);
      a.require(tokens.has_value());
      a.expect_eq(to_string(*tokens), expected);
    });
  }

  return s.run();
}
