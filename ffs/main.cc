// SPDX-FileCopyrightText: 2025 Robin Lindén <dev@robinlinden.eu>
//
// SPDX-License-Identifier: BSD-2-Clause

#include "starlark/token.h"
#include "starlark/tokenizer.h"

#include <fstream>
#include <iostream>
#include <iterator>
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

} // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    std::string_view name = argv[0] != nullptr ? argv[0] : "<bin>";
    std::cerr << "Usage: " << name << " <input_file>\n";
    return 1;
  }

  auto input = std::ifstream{argv[1]};
  if (!input) {
    std::cerr << "Error: Could not open file " << argv[1] << "\n";
    return 1;
  }

  std::string content{std::istreambuf_iterator<char>(input),
                      std::istreambuf_iterator<char>()};

  std::cout << "Input:\n" << content << "\n\n";

  auto tokens = starlark::tokenize(content);
  if (!tokens) {
    std::cerr << "Error: Failed to tokenize input.\n";
    return 1;
  }

  std::cout << "Tokens:\n" << to_string(*tokens) << "\n";
}
