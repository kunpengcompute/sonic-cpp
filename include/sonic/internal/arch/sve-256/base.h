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

#include <arm_neon.h>
#include <arm_sve.h>
#include "sonic/macro.h"
#include "../common/arm_common/base.h"

#ifndef VEC_LEN
#define VEC_LEN 16
#endif

namespace sonic_json {
namespace internal {
namespace sve_256 {

using sonic_json::internal::arm_common::ClearLowestBit;
using sonic_json::internal::arm_common::CountOnes;
using sonic_json::internal::arm_common::InlinedMemcmp;
using sonic_json::internal::arm_common::InlinedMemcmpEq;
using sonic_json::internal::arm_common::LeadingZeroes;
using sonic_json::internal::arm_common::PrefixXor;
using sonic_json::internal::arm_common::TrailingZeroes;
using sonic_json::internal::arm_common::Xmemcpy;

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

}  // namespace sve_256
}  // namespace internal
}  // namespace sonic_json
