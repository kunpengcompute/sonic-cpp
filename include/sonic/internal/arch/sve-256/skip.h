// Copyright (c) 2025 Huawei Technologies Co., Ltd.
// Copyright 2022 ByteDance Inc.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#define VEC_LEN 16

#include <sonic/internal/arch/common/skip_common.h>

#include "base.h"
#include "quote.h"
#include "simd.h"
#include "sonic/dom/json_pointer.h"
#include "sonic/error.h"
#include "sonic/internal/utils.h"
#include "sonic/macro.h"
#include "unicode.h"

namespace sonic_json {
namespace internal {
namespace sve_256 {
using sonic_json::internal::common::EqBytes4;
using sonic_json::internal::common::SkipLiteral;

#ifndef VEC_LEN
#error "Define vector length firstly!"
#endif

// GetNextToken find the next characters in tokens and update the position to
// it.
template <size_t N>
sonic_force_inline uint8_t GetNextToken(const uint8_t *data, size_t &pos, size_t len, const char (&tokens)[N])
{
    auto svelen = svcntb();
    for (; pos < len; pos += svelen) {
        svbool_t predicate = svwhilele_b8_u64(pos, len);
        svuint8_t v = svld1_u8(predicate, reinterpret_cast<const uint8_t *>(data + pos));
        svbool_t vor = svpfalse_b();
        for (size_t i = 0; i < N - 1; i++) {
            svbool_t vtmp = svcmpeq_n_u8(predicate, v, (uint8_t)(tokens[i]));
            vor = svorr_b_z(predicate, vor, vtmp);
        }
        if (svptest_any(predicate, vor)) {
            pos += get_first_active_index(vor);
            return data[pos];
        }
    }
    return '\0';
}

// pos is the after the ending quote
sonic_force_inline int SkipString(const uint8_t *data, size_t &pos, size_t len)
{
    const static int kEscaped = 2;
    const static int kNormal = 1;
    const static int kUnclosed = 0;
    auto svelen = svcntb();
    svbool_t ptrue = svptrue_b8();
    int ret = kNormal;
    while (pos + svelen < len) {
        svuint8_t v = svld1_u8(ptrue, reinterpret_cast<const uint8_t *>(data + pos));
        svbool_t bs_bits = svcmpeq_n_u8(ptrue, v, (uint8_t)('\\'));
        svbool_t quote_bits = svcmpeq_n_u8(ptrue, v, (uint8_t)('"'));

        auto bs_first_bits = svbrkb_b_z(ptrue, bs_bits);
        if (svptest_any(ptrue, svand_b_z(ptrue, quote_bits, bs_first_bits))) {
            pos += get_first_active_index(quote_bits) + 1;
            return ret;
        }

        if (svptest_any(ptrue, bs_bits)) {
            ret = kEscaped;
            pos += svcntp_b8(bs_first_bits, bs_first_bits) + 0x2;
            while (pos < len) {
                if (data[pos] == '\\') {
                    pos += 0x2;
                } else {
                    break;
                }
            }
        } else {
            pos += svelen;
        }
    }

    while (pos < len) {
        svbool_t predicate = svwhilelt_b8_u64(pos, len);
        svuint8_t v = svld1_u8(predicate, reinterpret_cast<const uint8_t *>(data + pos));
        svbool_t bs_bits = svcmpeq_n_u8(predicate, v, (uint8_t)('\\'));
        svbool_t quote_bits = svcmpeq_n_u8(predicate, v, (uint8_t)('"'));

        auto bs_first_bits = svbrkb_b_z(ptrue, bs_bits);
        if (svptest_any(predicate, svand_b_z(predicate, quote_bits, bs_first_bits))) {
            pos += get_first_active_index(quote_bits) + 1;
            return ret;
        }

        if (svptest_any(predicate, bs_bits)) {
            ret = kEscaped;
            pos += svcntp_b8(bs_first_bits, bs_first_bits) + 0x2;
            while (pos < len) {
                if (data[pos] == '\\') {
                    pos += 0x2;
                } else {
                    break;
                }
            }
        } else {
            pos += svcntp_b8(predicate, predicate);
        }
    }

    return kUnclosed;
}

template <typename T>
sonic_force_inline uint64_t GetStringBits(const uint8_t *data, uint64_t &prev_instring, uint64_t &prev_escaped)
{
    const T v(data);
    uint64_t escaped = 0;
    uint64_t bs_bits = v.eq('\\');
    if (bs_bits) {
        escaped = common::GetEscaped<64>(prev_escaped, bs_bits);
    } else {
        escaped = prev_escaped;
        prev_escaped = 0;
    }
    uint64_t quote_bits = v.eq('"') & ~escaped;
    uint64_t in_string = PrefixXor(quote_bits) ^ prev_instring;
    prev_instring = uint64_t(static_cast<int64_t>(in_string) >> 63);
    return in_string;
}

// return true if container is closed.
template <typename T>
sonic_force_inline bool skip_container(const uint8_t *data, size_t &pos, size_t len, uint8_t left, uint8_t right)
{
    uint64_t prev_instring = 0, prev_escaped = 0, instring;
    int rbrace_num = 0, lbrace_num = 0, last_lbrace_num;
    const uint8_t *p;
    while (pos + 64 <= len) {
        p = data + pos;

        instring = GetStringBits<T>(p, prev_instring, prev_escaped);
        T v(p);
        last_lbrace_num = lbrace_num;
        uint64_t rbrace = v.eq(right) & ~instring;
        uint64_t lbrace = v.eq(left) & ~instring;
        /* traverse each '}' */
        while (rbrace > 0) {
            rbrace_num++;
            lbrace_num = last_lbrace_num + CountOnes((rbrace - 1) & lbrace);
            bool is_closed = lbrace_num < rbrace_num;
            if (is_closed) {
                sonic_assert(rbrace_num == lbrace_num + 1);
                pos += TrailingZeroes(rbrace) + 1;
                return true;
            }
            rbrace &= (rbrace - 1);
        }
        lbrace_num = last_lbrace_num + CountOnes(lbrace);

        pos += 64;
    }
    uint8_t buf[64] = { 0 };
    std::memcpy(buf, data + pos, len - pos);
    p = buf;

    instring = GetStringBits<T>(p, prev_instring, prev_escaped);
    T v(p);
    last_lbrace_num = lbrace_num;
    uint64_t rbrace = v.eq(right) & ~instring;
    uint64_t lbrace = v.eq(left) & ~instring;
    /* traverse each '}' */
    while (rbrace > 0) {
        rbrace_num++;
        lbrace_num = last_lbrace_num + CountOnes((rbrace - 1) & lbrace);
        bool is_closed = lbrace_num < rbrace_num;
        if (is_closed) {
            sonic_assert(rbrace_num == lbrace_num + 1);
            pos += TrailingZeroes(rbrace) + 1;
            return true;
        }
        rbrace &= (rbrace - 1);
    }
    lbrace_num = last_lbrace_num + CountOnes(lbrace);

    return false;
}

sonic_force_inline bool SkipContainer(const uint8_t *data, size_t &pos, size_t len, uint8_t left, uint8_t right)
{
    return skip_container<simd8x64<uint8_t>>(data, pos, len, left, right);
}

sonic_force_inline uint8_t skip_space(const uint8_t *data, size_t &pos, size_t &, uint64_t &)
{
    // fast path for single space
    if (!IsSpace(data[pos++]))
        return data[pos - 1];
    if (!IsSpace(data[pos++]))
        return data[pos - 1];

    svbool_t ptrue = svptrue_b8();
    auto svelen = svcntb();
    // current pos is out of block
    while (1) {
        svuint8_t v = svld1_u8(ptrue, reinterpret_cast<const uint8_t *>(data + pos));
        svbool_t m1 = svcmpeq_n_u8(ptrue, v, '\r');
        svbool_t m2 = svcmpeq_n_u8(ptrue, v, '\n');
        svbool_t m3 = svcmpeq_n_u8(ptrue, v, '\t');
        svbool_t m4 = svcmpeq_n_u8(ptrue, v, ' ');
        svbool_t m5 = svorr_b_z(ptrue, m1, m2);
        svbool_t m6 = svorr_b_z(ptrue, m3, m4);
        svbool_t mask = svnor_b_z(ptrue, m5, m6);
        if (svptest_any(ptrue, mask)) {
            pos += get_first_active_index(mask);
            return data[pos++];
        } else {
            pos += svelen;
        }
    }

    sonic_assert(false && "!should not happen");
}

sonic_force_inline uint8_t skip_space_safe(const uint8_t *data, size_t &pos, size_t len, size_t &, uint64_t &)
{
    while (pos < len && IsSpace(data[pos])) {
        pos++;
    }
    pos++;
    // if not found, still return the space chars
    return data[pos - 1];
}

}  // namespace sve_256
}  // namespace internal
}  // namespace sonic_json

#undef VEC_LEN
