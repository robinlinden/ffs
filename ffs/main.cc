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

std::string to_string(std::vector<starlark::Token> const &tokens) {
    std::stringstream ss;

    for (auto const &token : tokens) {
        ss << starlark::to_string(token) << ' ';
    }

    return std::move(ss).str();
}

// Find the first directory w/ a MODULE.bazel above a given path.
std::optional<std::filesystem::path> project_root(std::filesystem::path from) {
    assert(std::filesystem::is_directory(from));

    do {
        if (std::filesystem::exists(from / "MODULE.bazel")) {
            return from;
        }

        from = from.parent_path();
        std::cerr << from << std::endl;
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

    auto root_path = project_root(from);
    if (!root_path) {
        std::cerr << "Unable to find ffs project root for folder '" << from << "'.\n";
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
        return std::nullopt;
    }

    std::string content{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    auto program = starlark::parse(content);
    if (!program) {
        return std::nullopt;
    }

    std::vector<Target> targets;

    starlark::Interpreter interpreter;

    static constexpr auto kIgnore = [](auto const &) { return starlark::Value{}; };
    auto store_target_name = [&](std::vector<starlark::NativeArgument> args) {
        auto it = std::ranges::find_if(
            args, [](auto &name) { return name == "name"; }, &starlark::NativeArgument::name);
        if (it != std::ranges::end(args) && std::holds_alternative<std::string>(it->value.v)) {
            targets.emplace_back(std::move(std::get<std::string>(it->value.v)));
        }

        return starlark::Value{0};
    };

    interpreter.variables["cc_binary"] =
        starlark::Value{std::make_shared<starlark::NativeFn>(store_target_name)};
    interpreter.variables["cc_library"] =
        starlark::Value{std::make_shared<starlark::NativeFn>(store_target_name)};
    interpreter.variables["cc_test"] =
        starlark::Value{std::make_shared<starlark::NativeFn>(store_target_name)};
    interpreter.variables["glob"] = starlark::Value{std::make_shared<starlark::NativeFn>(kIgnore)};

    auto res = interpreter.run(*program);
    if (!res) {
        std::cerr << "Query failed. :(\n";
        return std::nullopt;
    }

    return targets;
}

int run_debug(int argc, char **argv) {
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

int run_query(int argc, char **argv) {
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
