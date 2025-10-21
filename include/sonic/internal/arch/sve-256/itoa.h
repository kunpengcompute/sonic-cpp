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

#include <arm_sve.h>

namespace sonic_json {
namespace internal {
namespace sve_256 {

#define UTO8_FACTOR 10000
#define UTO16_FACTOR 100000000

sonic_force_inline svuint16_t utoa_8_helper(uint16_t num1, uint16_t num2)
{
  svbool_t ptrue_32 = svptrue_b32();
  svuint32_t v1 = svdup_n_u32(num1);
  svuint32_t v2 = svdup_n_u32(num2);
  svuint32_t kVecDiv = svunpklo_u32(svreinterpret_u16_u64(svdup_n_u64(0x80003334147b20c5)));
  svuint32_t v00 = svsplice_u32(svptrue_pat_b32(SV_VL4), v1, v2);
  svuint32_t v01 = svmul_u32_x(ptrue_32, v00, kVecDiv);
  svuint32_t v02 = svlsr_n_u32_x(ptrue_32, v01, 14);
  svuint32_t kVecShift = svunpklo_u32(svreinterpret_u16_u64(svdup_n_u64(0x8000200008000080)));
  svuint32_t v03 = svmul_u32_x(ptrue_32, v02, kVecShift);
  return svreinterpret_u16_u32(v03);
}

sonic_force_inline svuint16_t utoa_sve(uint32_t num)
{
  svbool_t ptrue_16 = svptrue_b16();
  svuint16_t help_vec = utoa_8_helper(num / UTO8_FACTOR, num % UTO8_FACTOR);
  svuint16_t v10 = svuzp2_u16(help_vec, svdup_n_u16(0));
  svuint16_t v11 = svmul_n_u16_x(ptrue_16, v10, 0xa);
  svuint16_t v12 = svreinterpret_u16_u64(svlsl_n_u64_x(svptrue_b64(), svreinterpret_u64_u16(v11), 0x10));
  svuint16_t v13 = svsub_u16_x(ptrue_16, v10, v12);
  return v13;
}

sonic_force_inline svuint16_t utoa_helper16(uint16_t num1, uint16_t num2, uint16_t num3, uint16_t num4)
{
    svbool_t ptrue_32 = svptrue_b32();
    svuint32_t v1 = svdup_n_u32(num1);
    svuint32_t v2 = svdup_n_u32(num2);
    svuint32_t v3 = svdup_n_u32(num3);
    svuint32_t v4 = svdup_n_u32(num4);

    svuint32_t kVecDiv = svunpklo_u32(svreinterpret_u16_u64(svdup_n_u64(0x80003334147b20c5)));
    svuint32_t v00 = svuzp1_u32(v1, v2);
    svuint32_t v10 = svuzp1_u32(v3, v4);
    svuint32_t v01 = svmul_u32_x(ptrue_32, v00, kVecDiv);
    svuint32_t v11 = svmul_u32_x(ptrue_32, v10, kVecDiv);
    svuint32_t v02 = svlsr_n_u32_x(ptrue_32, v01, 0xe);
    svuint32_t v12 = svlsr_n_u32_x(ptrue_32, v11, 0xe);
    svuint32_t kVecShift = svunpklo_u32(svreinterpret_u16_u64(svdup_n_u64(0x8000200008000080)));
    svuint32_t v03 = svmul_u32_x(ptrue_32, v02, kVecShift);
    svuint32_t v13 = svmul_u32_x(ptrue_32, v12, kVecShift);
    svuint16_t res = svuzp2_u16(svreinterpret_u16_u32(v03), svreinterpret_u16_u32(v13));
    return res;
}

sonic_force_inline svuint16_t utoa_sve16(uint32_t num1, uint32_t num2)
{
    svbool_t ptrue_16 = svptrue_b16();
    svuint16_t help_vec = utoa_helper16(num1 / UTO8_FACTOR, num1 % UTO8_FACTOR, num2 / UTO8_FACTOR, num2 % UTO8_FACTOR);
    svuint16_t v11 = svmul_n_u16_x(ptrue_16, help_vec, 0xa);
    svuint16_t v12 = svreinterpret_u16_u64(svlsl_n_u64_x(svptrue_b64(), svreinterpret_u64_u16(v11), 0x10));
    svuint16_t v13 = svsub_u16_x(ptrue_16, help_vec, v12);
    return v13;
}

static sonic_force_inline char *Utoa_8(uint32_t val, char *out) {
  /* convert to digits */
  svuint16_t v0 = utoa_sve(val);
  svuint16_t v3 = svadd_u16_x(svptrue_b16(), v0, svdup_n_u16((uint16_t)('0')));
  svuint8_t v4 = svuzp1_u8(svreinterpret_u8_u16(v3), svdup_n_u8(0));
  // /* store high 64 bits */
  svst1_u8(svptrue_pat_b8(SV_VL8), (uint8_t *)out, v4);
  return out + 0x8;
}

static sonic_force_inline char *Utoa_16(uint64_t val, char *out)
{
    /* remaining digits */
    svuint16_t v0 = utoa_sve16((uint32_t)(val / UTO16_FACTOR), (uint32_t)(val % UTO16_FACTOR));
    svuint16_t v3 = svadd_u16_x(svptrue_b16(), v0, svdup_n_u16((uint16_t)('0')));
    svuint8_t v4 = svuzp1_u8(svreinterpret_u8_u16(v3), svdup_n_u8(0));
    svst1_u8(svptrue_pat_b8(SV_VL16), (uint8_t *)out, v4);
    return out + 0x10;
}

}  // namespace sve_256
}  // namespace internal
}  // namespace sonic_json
