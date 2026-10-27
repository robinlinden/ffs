// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "starlark/interpreter.h"
#include "starlark/parser.h"
#include "starlark/token.h"
#include "starlark/tokenizer.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr std::optional<std::pair<std::string_view, std::string_view>>
split_once(std::string_view str, char sep) {
    if (auto p = str.find(sep); p != std::string_view::npos) {
        return std::pair{str.substr(0, p), str.substr(p + 1)};
    }

    return std::nullopt;
}

std::string to_string(std::vector<starlark::Token> const &tokens) {
    std::stringstream ss;

    for (auto const &token : tokens) {
        ss << starlark::to_string(token) << ' ';
    }

    return std::move(ss).str();
}

// Find the first directory w/ a MODULE.bazel above a given path.
std::optional<std::filesystem::path> project_root(std::filesystem::path from) {
    do {
        if (std::filesystem::exists(from / "MODULE.bazel")) {
            return from;
        }

        from = from.parent_path();
    } while (from != from.parent_path());

    return std::nullopt;
}

std::optional<std::vector<std::filesystem::path>>
build_files_from_pattern(std::filesystem::path const &from, std::string_view pattern) {
    if (pattern != "...") {
        // TODO(robinlinden): Support patterns.
        std::cerr << "Unsupported pattern '" << pattern << "'.\n";
        return std::nullopt;
    }

    std::vector<std::filesystem::path> found;
    for (auto const &dir : std::filesystem::recursive_directory_iterator{from}) {
        if (dir.path().filename() == "BUILD.bazel") {
            found.push_back(dir);
        }
    }

    return found;
}

struct Target {
    std::string name;
};

// TODO(robinlinden): This should be less silly.
std::optional<std::vector<Target>> targets_from_build_file(std::filesystem::path bf) {
    std::ifstream input{bf};
    if (!input) {
        std::cerr << "Failed to open build file '" << bf << "'.\n";
        return std::nullopt;
    }

    std::string content{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    auto program = starlark::parse(content);
    if (!program) {
        std::cerr << "Unable to parse program for build file '" << bf << ".\n";
        return std::nullopt;
    }

    std::vector<Target> targets;

    auto store_target_name = starlark::Value{
        std::make_shared<starlark::NativeFn>([&](std::vector<starlark::NativeArgument> args) {
            auto it = std::ranges::find_if(
                args, [](auto &name) { return name == "name"; }, &starlark::NativeArgument::name);
            if (it != std::ranges::end(args) && std::holds_alternative<std::string>(it->value.v)) {
                targets.emplace_back(std::move(std::get<std::string>(it->value.v)));
            }

            return starlark::Value{0};
        })};

    static constexpr auto kIgnore = [](auto const &) { return starlark::Value{}; };

    starlark::Interpreter interpreter{
        .variables{},
        .module_loader = [&](std::string_view module) -> std::optional<starlark::ModuleExports> {
            if (module.starts_with("//")) {
                auto root = project_root(bf);
                assert(root.has_value());

                auto split = split_once(module, ':');
                if (!split) {
                    std::cerr << "Unable to find filename for '" << module
                              << "' in module loading.\n";
                    return std::nullopt;
                }

                auto [path, file] = *split;
                auto target = *root / path.substr(2) / file;
                std::ifstream target_ifs{target};
                if (!target_ifs) {
                    std::cerr << "Unable to open '" << target << "'.\n";
                    return std::nullopt;
                }

                std::string c{
                    std::istreambuf_iterator<char>(target_ifs), std::istreambuf_iterator<char>()};
                auto p = starlark::parse(c);
                if (!p) {
                    std::cerr << "Unable to parse '\n" << c << "\n' as a program.\n";
                    return std::nullopt;
                }

                starlark::Interpreter i;
                i.variables["glob"] =
                    starlark::Value{std::make_shared<starlark::NativeFn>(kIgnore)};
                i.variables["select"] =
                    starlark::Value{std::make_shared<starlark::NativeFn>(kIgnore)};
                auto result = i.run(*p);
                if (!result) {
                    std::cerr << "Running '" << target << "' failed: " << to_string(result.error())
                              << ".\n";
                    return std::nullopt;
                }

                return std::move(i.variables);
            }

            if (module.starts_with("@rules_cc/")) {
                return starlark::ModuleExports{
                    {"cc_binary", store_target_name},
                    {"cc_library", store_target_name},
                    {"cc_test", store_target_name},
                };
            }

            if (module.starts_with("@rules_fuzzing/")) {
                return starlark::ModuleExports{
                    {"cc_fuzz_test", store_target_name},
                };
            }

            std::cerr << "Gave up on loading '" << module << "'.\n";
            return std::nullopt;
        },
    };

    interpreter.variables["glob"] = starlark::Value{std::make_shared<starlark::NativeFn>(kIgnore)};
    interpreter.variables["select"] =
        starlark::Value{std::make_shared<starlark::NativeFn>(kIgnore)};

    auto res = interpreter.run(*program);
    if (!res) {
        std::cerr << "Query for '" << bf << "' failed w/ reason '" << to_string(res.error())
                  << "'. :(\n";
        return std::nullopt;
    }

    return targets;
}

int run_debug([[maybe_unused]] int argc, char **argv) {
    assert(argc == 2);

    auto input = std::ifstream{argv[1]};
    if (!input) {
        std::cerr << "Error: Could not open file " << argv[1] << "\n";
        return 1;
    }

    std::string content{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};

    std::cout << "Input:\n" << content << "\n\n";

    auto tokens = starlark::tokenize(content);
    if (!tokens) {
        std::cerr << "Error: Failed to tokenize input.\n";
        return 1;
    }

    std::cout << "Tokens:\n" << to_string(*tokens) << "\n";

    auto program = starlark::parse(content);
    if (!program) {
        std::cerr << "Error: Failed to parse input.\n";
        return 1;
    }

    return 0;
}

int run_query([[maybe_unused]] int argc, char **argv) {
    assert(argc == 3);

    auto cwd = std::filesystem::current_path();
    auto root_path = project_root(cwd);
    if (!root_path) {
        std::cerr << "Unable to find ffs project root for folder '" << cwd << "'.\n";
        return 1;
    }

    auto build_files = build_files_from_pattern(cwd, argv[2]);
    if (!build_files) {
        std::cerr << "Failed to find build files.\n";
        return 1;
    }

    bool failure = false;
    for (auto const &build_file : *build_files) {
        auto targets = targets_from_build_file(build_file);
        if (!targets) {
            std::cerr << "Resolving targets for build file " << build_file << " failed.\n";
            failure = true;
            continue;
        }

        auto package = build_file.parent_path().lexically_relative(*root_path).string();
        for (auto const &target : *targets) {
            std::cout << "//" << package << ":" << target.name << '\n';
        }
    }

    return failure ? 1 : 0;
}

} // namespace

int main(int argc, char **argv) {
    if (argc == 3 && argv[1] == std::string_view{"query"}) {
        return run_query(argc, argv);
    }

    if (argc != 2) {
        std::string_view name = argv[0] != nullptr ? argv[0] : "<bin>";
        std::cerr << "Usage: " << name << " <input_file>\n";
        return 1;
    }

    return run_debug(argc, argv);
}
