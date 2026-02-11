/*
 * Copyright 2022 ByteDance Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cstdint>

namespace sonic_json {
namespace internal {

static sonic_force_inline bool IsSpace(uint8_t ch) {
  if(ch > 32) return false;
  static const uint64_t mask =
        (1ull << ' ') | (1ull << '\r') | (1ull << '\n') | (1ull << '\t');
  return (mask & (1ull << ch)) != 0;
}

}  // namespace internal
}  // namespace sonic_json
