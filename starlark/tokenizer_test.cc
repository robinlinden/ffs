// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "starlark/tokenizer.h"

#include "starlark/token.h"

#include "etest/etest2.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

int main() {
    namespace t = starlark::token;
    using Tokens = std::vector<starlark::Token>;

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
            {"1234", Tokens{t::IntLiteral{1234}}},
            {"00001234", Tokens{t::IntLiteral{1234}}},
            {"123abc", std::nullopt},
            {"123.123", std::nullopt}, // TODO(robinlinden): Floats.
            {"-123", Tokens{t::IntLiteral{-123}}},
            {
                "9223372036854775807",
                Tokens{t::IntLiteral{std::numeric_limits<std::int64_t>::max()}},
            },
            {"9223372036854775808", std::nullopt}, // Out of range. :(
            {
                "-9223372036854775808",
                Tokens{t::IntLiteral{std::numeric_limits<std::int64_t>::min()}},
            },
            {"-9223372036854775809", std::nullopt}, // Out of range. :(
            {
                R"("hello world" 'hello world')",
                Tokens{t::StringLiteral{"hello world"}, t::StringLiteral{"hello world"}},
            },
            {
                R"("""hello
world""")",
                Tokens{t::StringLiteral{"hello\nworld"}},
            },
            {
                R"('''hello
world''')",
                Tokens{t::StringLiteral{"hello\nworld"}},
            },
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
