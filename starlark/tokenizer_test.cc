// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "starlark/tokenizer.h"

#include "starlark/token.h"

#include "etest/etest2.h"

#include <array>
#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

int main() {
    namespace t = starlark::token;

    auto const test_cases =
        std::to_array<std::pair<std::string_view, std::optional<std::vector<starlark::Token>>>>({
            {
                R"(load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test"))",
                std::vector<starlark::Token>{
                    t::Load{},
                    t::LParen{},
                    t::StringLiteral{"@rules_cc//cc:defs.bzl"},
                    t::Comma{},
                    t::StringLiteral{"cc_library"},
                    t::Comma{},
                    t::StringLiteral{"cc_test"},
                    t::RParen{},
                },
            },
            {"global", std::nullopt}, // Reserved identifier.
            {"globalist", std::vector<starlark::Token>{t::Identifier{"globalist"}}},
        });

    etest::Suite s{};

    for (auto const &[input, expected] : test_cases) {
        s.add_test(std::string{input}, [input, &expected](etest::IActions &a) {
            auto tokens = starlark::tokenize(input);
            a.expect_eq(tokens, expected);
        });
    }

    return s.run();
}
