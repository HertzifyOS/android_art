/*
 * Copyright (C) 2026 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "synthetic_class_format_util.h"

#include <format>
#include <string_view>

#include "dex/descriptors_names.h"

namespace art {

std::optional<std::string> RewriteSyntheticProfileClassIfNeeded(std::string_view klass) {
  if (klass.empty()) {
    return {};
  }

  DCHECK(IsValidDescriptor(std::string(klass).c_str()));

  // Ignore primitives or primitive arrays.
  if (!klass.ends_with(';')) {
    return {};
  }

  // Strip the trailing semicolon.
  std::string_view body = klass.substr(0, klass.size() - 1);

  // Find the split point where digits start at the end (e.g., "LBar$1" -> "LBar$"+"1").
  size_t last_non_digit_idx = body.find_last_not_of("0123456789");
  if (last_non_digit_idx == std::string::npos) {
    return {};
  }

  size_t digit_idx = last_non_digit_idx + 1;
  if (digit_idx >= body.size()) {
    return {};
  }

  std::string_view prefix = body.substr(0, digit_idx);
  std::string_view digits = body.substr(digit_idx);

  // Case 1: Minimization ($$ExternalSynthetic -> $)
  if (auto pos = prefix.find("$$ExternalSynthetic"); pos != std::string_view::npos) {
    return std::format("{}${};", prefix.substr(0, pos), digits);
  }

  // Case 2: Expansion ($ -> $$ExternalSyntheticLambda)
  // Note: This rewrite is purely *speculative*. We assume Lambda only because that has historically
  // been the most represented synthetic present in platform profiles. A more general solution
  // would be to use wildcard expansion for any synthetic that might use the same ordinal.
  if (prefix.back() == '$') {
    return std::format("{}$ExternalSyntheticLambda{};", prefix, digits);
  }

  return {};
}

}  // namespace art
