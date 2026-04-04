// SPDX-FileCopyrightText: 2025-2026 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

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
        if (std::filesystem::exists(dir.path() / "BUILD.bazel")) {
            found.push_back(dir.path() / "BUILD.bazel");
        }
    }

    return found;
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
    auto build_files = build_files_from_pattern(cwd, argv[2]);
    if (!build_files) {
        std::cerr << "Failed to find build files.\n";
        return 1;
    }

    // TODO(robinlinden): Get targets from build files.
    for (auto const &build_file : *build_files) {
        std::cout << build_file << '\n';
    }

    return 0;
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
