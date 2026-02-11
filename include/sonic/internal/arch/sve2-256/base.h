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

#include <arm_neon.h>
#include <arm_sve.h>
#include "sonic/macro.h"
#include "../common/arm_common/base.h"

#ifndef VEC_LEN
#define VEC_LEN 32
#endif

namespace sonic_json {
namespace internal {
namespace sve2_256 {

using sonic_json::internal::arm_common::ClearLowestBit;
using sonic_json::internal::arm_common::CountOnes;
using sonic_json::internal::arm_common::InlinedMemcmp;
using sonic_json::internal::arm_common::InlinedMemcmpEq;
using sonic_json::internal::arm_common::LeadingZeroes;
using sonic_json::internal::arm_common::PrefixXor;
using sonic_json::internal::arm_common::TrailingZeroes;

sonic_force_inline svbool_t copy_get_escaped_mask_predicate(svbool_t pg, const char *src, char *dst)
{
    svuint8_t v = svld1_u8(pg, reinterpret_cast<const uint8_t *>(src));
    svst1_u8(pg, reinterpret_cast<uint8_t *>(dst), v);
    svbool_t m1 = svcmpeq_n_u8(pg, v, '\\');
    svbool_t m2 = svcmpeq_n_u8(pg, v, '"');
    svbool_t m3 = svcmplt_n_u8(pg, v, '\x20');
    svbool_t m4 = svorr_b_z(pg, m1, m2);
    svbool_t m5 = svorr_b_z(pg, m3, m4);
    return m5;
}

// The function returns the index of first (to the rigth) active elem
sonic_force_inline int get_first_active_index(svbool_t input)
{
    return svcntp_b8(svptrue_b8(), svbrkb_b_z(svptrue_b8(), input));
}


template <size_t ChunkSize>
sonic_force_inline void Xmemcpy(void* dst_, const void* src_, size_t chunks) {
  std::memcpy(dst_, src_, chunks * ChunkSize);
}

template <>
sonic_force_inline void Xmemcpy<32>(void* dst_, const void* src_,
                                    size_t chunks) {
  uint8_t* dst = reinterpret_cast<uint8_t*>(dst_);
  const uint8_t* src = reinterpret_cast<const uint8_t*>(src_);
  size_t blocks = chunks / 2;
  svbool_t ptrue = svptrue_b8();
  for (size_t i = 0; i < blocks; i++) {
      svuint8x2_t s_input = svld2_u8(ptrue, src);
      svst2_u8(ptrue, dst, s_input);
      src += 64;
      dst += 64;
  }
  if(chunks % 2) {
    std::memcpy(dst, src, 32);
  }
}

template <>
sonic_force_inline void Xmemcpy<16>(void* dst_, const void* src_,
                                    size_t chunks) {
  uint8_t* dst = reinterpret_cast<uint8_t*>(dst_);
  const uint8_t* src = reinterpret_cast<const uint8_t*>(src_);
  size_t blocks = chunks / 2;
  svbool_t ptrue = svptrue_b8();
  for (size_t i = 0; i < blocks; i++) {
      svuint8_t s_input = svld1_u8(ptrue, src);
      svst1_u8(ptrue, dst, s_input);
      src += 32;
      dst += 32;
  }
  if(chunks % 2) {
    std::memcpy(dst, src, 16);
  }
}

}  // namespace sve2_256
}  // namespace internal
}  // namespace sonic_json
