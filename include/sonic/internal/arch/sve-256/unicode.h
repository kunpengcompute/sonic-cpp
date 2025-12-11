// Copyright 2018-2019 The simdjson authors

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This file may have been modified by ByteDance authors/Huawei authors. All ByteDance
// Modifications are Copyright 2022 ByteDance Authors. All Huawei Modifications are
// Copyright (c) 2025 Huawei Technologies Co., Ltd.
#pragma once

#include <sonic/macro.h>

#include <cstdint>
#include <cstring>

#include "../common/unicode_common.h"
#include "base.h"
#include "simd.h"

namespace sonic_json {
namespace internal {
namespace sve_256 {
using sonic_json::internal::common::handle_unicode_codepoint;

struct StringBlock {
public:
    sonic_force_inline static StringBlock find(const uint8_t *src);
    sonic_force_inline static StringBlock find(uint8x16_t &v);
    sonic_force_inline bool has_quote_first() const
    {
        return (((bs_bits - 1) & quote_bits) != 0) && !has_unescaped();
    }
    sonic_force_inline bool has_back_slash() const
    {
        return ((quote_bits - 1) & bs_bits) != 0;
    }
    sonic_force_inline bool has_unescaped() const
    {
        return ((quote_bits - 1) & unescaped_bits) != 0;
    }
    sonic_force_inline int quote_index() const
    {
        return TrailingZeroes(quote_bits) >> 0x2;
    }
    sonic_force_inline int bs_index() const
    {
        return TrailingZeroes(bs_bits) >> 0x2;
    }

    uint64_t bs_bits;
    uint64_t quote_bits;
    uint64_t unescaped_bits;
};

sonic_force_inline StringBlock StringBlock::find(const uint8_t *src)
{
    uint8x16_t v = vld1q_u8(src);
    return {
        to_bitmask(vceqq_u8(v, vdupq_n_u8('\\'))),
        to_bitmask(vceqq_u8(v, vdupq_n_u8('"'))),
        to_bitmask(vcleq_u8(v, vdupq_n_u8('\x1f'))),
    };
}

sonic_force_inline StringBlock StringBlock::find(uint8x16_t &v)
{
    return {
        to_bitmask(vceqq_u8(v, vdupq_n_u8('\\'))),
        to_bitmask(vceqq_u8(v, vdupq_n_u8('"'))),
        to_bitmask(vcleq_u8(v, vdupq_n_u8('\x1f'))),
    };
}

} // namespace sve_256
} // namespace internal
} // namespace sonic_json
