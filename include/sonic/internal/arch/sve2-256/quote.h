/*
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
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

#define VEC_LEN 32

#include "../common/arm_common/quote.h"
#include "../common/unicode_common.h"
#include "simd.h"
using sonic_json::internal::common::handle_unicode_codepoint;

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#ifdef __GNUC__
#if defined(__SANITIZE_THREAD__) || defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_LEAK__) || \
    defined(__SANITIZE_UNDEFINED__)
#ifndef SONIC_USE_SANITIZE
#define SONIC_USE_SANITIZE
#endif
#endif
#endif

#if defined(__clang__)
#if defined(__has_feature)
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer) || __has_feature(memory_sanitizer) || \
    __has_feature(undefined_behavior_sanitizer) || __has_feature(leak_sanitizer)
#ifndef SONIC_USE_SANITIZE
#define SONIC_USE_SANITIZE
#endif
#endif
#endif
#endif

#ifndef VEC_LEN
#error "You should define VEC_LEN before including quote.h!"
#endif

#define MOVE_N_CHARS(src, N) \
    do {                     \
        (src) += (N);        \
        nb -= (N);           \
        dst += (N);          \
    } while (0)

namespace sonic_json {
namespace internal {
namespace sve2_256 {
sonic_force_inline char *Quote(const char *src, size_t nb, char *dst)
{
    *dst++ = '"';
    sonic_assert(nb < (1LL << 0x20));
    auto svelen = svcntb();
    svbool_t ptrue = svptrue_b8();
    while (nb > svelen) {
        svbool_t mask = copy_get_escaped_mask_predicate(ptrue, src, dst);
        if (svptest_any(ptrue, mask)) {
            auto cn = get_first_active_index(mask);
            MOVE_N_CHARS(src, cn);
            DoEscape(src, dst, nb);
        } else {
            MOVE_N_CHARS(src, svelen);
        }
    }
    while (nb > 0) {
        svbool_t predicate = svwhilelt_b8_u64(0, nb);
        svbool_t mask = copy_get_escaped_mask_predicate(predicate, src, dst);
        if (svptest_any(predicate, mask)) {
            auto cn = get_first_active_index(mask);
            MOVE_N_CHARS(src, cn);
            DoEscape(src, dst, nb);
        } else {
            auto active_elems = svcntp_b8(predicate, predicate);
            MOVE_N_CHARS(src, active_elems);
        }
    }
    *dst++ = '"';
    return dst;
}

sonic_force_inline size_t parseStringInplace(uint8_t *&src, SonicError &err) {
  uint8_t *dst = src;
  uint8_t *sdst = src;
  svbool_t ptrue = svptrue_b8();
  auto svelen = svcntb();
  while (1) {
  find:
    __asm__ volatile("prfm pldl1keep, [%0, %1]" : : "r"(src), "r"(256ull));
    svuint8_t v = svld1_u8(ptrue, reinterpret_cast<const uint8_t *>(src));
    svbool_t quote_bits = svcmpeq_n_u8(ptrue, v, (uint8_t)('"'));
    svbool_t escape_bits = svcmple_n_u8(ptrue, v, (uint8_t)('\x1f'));
    svbool_t bs_bits = svcmpeq_n_u8(ptrue, v, (uint8_t)('\\'));
    auto bs_escape = svorr_b_z(ptrue, bs_bits, escape_bits);
    auto quote_first_bits = svbrkb_b_z(ptrue, quote_bits);
    if (svptest_any(svbrkb_b_z(ptrue, bs_escape), quote_bits)) {
      size_t index = svcntp_b8(quote_first_bits, quote_first_bits);
      src += index;
      *src++ = '\0';
      return src - sdst - 1;
    }
    
    if (sonic_unlikely(svptest_any(quote_first_bits, escape_bits))) {
      err = kParseErrorUnEscaped;
      return 0;
    }

    if (!svptest_any(quote_first_bits, bs_bits)) {
      src += svelen;
      goto find;     
    }

    /* find out where the backspace is */
    auto bs_dist = svcntp_b8(ptrue, svbrkb_b_z(ptrue, bs_bits));
    src += bs_dist;
    dst = src;

  cont:
    uint8_t escape_char = src[1];
    if (sonic_unlikely(escape_char == 'u')) {
      if (!handle_unicode_codepoint(const_cast<const uint8_t **>(&src), &dst)) {
        err = kParseErrorEscapedUnicode;
        return 0;
      }
    } else {
      *dst = kEscapedMap[escape_char];
      if (sonic_unlikely(*dst == 0u)) {
        err = kParseErrorEscapedFormat;
        return 0;
      }
      src += 2;
      dst += 1;
    }
    // fast path for continous escaped chars
    if (*src == '\\') {
      bs_dist = 0;
      goto cont;
    }

  find_and_move:
    // Copy the next n bytes, and find the backslash and quote in them.
    v = svld1_u8(ptrue, reinterpret_cast<const uint8_t *>(src));
    quote_bits = svcmpeq_n_u8(ptrue, v, (uint8_t)('"'));
    bs_bits = svcmpeq_n_u8(ptrue, v, (uint8_t)('\\'));
    escape_bits = svcmple_n_u8(ptrue, v, (uint8_t)('\x1f'));
    bs_escape = svorr_b_z(ptrue, bs_bits, escape_bits);
    quote_first_bits = svbrkb_b_z(ptrue, quote_bits);
    // If the next thing is the end quote, copy and return
    if (svptest_any(svbrkb_b_z(ptrue, bs_escape), quote_bits)) {
      size_t index = svcntp_b8(quote_first_bits, quote_first_bits);
      svst1_u8(quote_first_bits, reinterpret_cast<uint8_t *>(dst), v);
      dst += index;
      *dst = '\0';
      src += index + 1;
      return dst - sdst;
    }

    if (sonic_unlikely(svptest_any(quote_first_bits, escape_bits))) {
      err = kParseErrorUnEscaped;
      return 0;
    }

    if (!svptest_any(quote_first_bits, bs_bits)) {
      /* they are the same. Since they can't co-occur, it means we
        * encountered neither. */
      svst1_u8(ptrue, reinterpret_cast<uint8_t *>(dst), v);
      src += svelen;
      dst += svelen;
      goto find_and_move;
    }
    auto bs_first_bits = svbrkb_b_z(ptrue, bs_bits);
    size_t index = svcntp_b8(bs_first_bits, bs_first_bits);
    svst1_u8(bs_first_bits, reinterpret_cast<uint8_t *>(dst), v);
    src += index;
    dst += index;
    goto cont;
  }
  sonic_assert(false);
}

}  // namespace sve2_256
}  // namespace internal
}  // namespace sonic_json

#undef VEC_LEN
