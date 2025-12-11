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

#define STR2INT_LOWEST_VALUE_MUL_FACTOR 10
#define STR2INT_HUNDRED_MUL_FACTOR 100
#define STR2INT_THOUSAND_MUL_FACTOR 1000

namespace sonic_json {
namespace internal {
namespace sve_256 {

sonic_force_inline uint32_t low_half_simd_str2int(svuint16_t data, uint32_t *mulFactor)
{
    svuint32_t factor = svld1_u32(svptrue_pat_b32(SV_VL8), mulFactor);
    svuint32_t res = svmul_u32_x(svptrue_pat_b32(SV_VL8), svunpklo_u32(data), factor);
    return svaddv_u32(svptrue_pat_b32(SV_VL8), res);
}

sonic_force_inline uint64_t dot_simd_str2int(uint64_t *scale, uint16_t *mulFactor, svuint16_t data)
{
    svuint16_t factor = svld1_u16(svptrue_pat_b16(SV_VL16), mulFactor);
    svuint64_t res = svdot_u64(svdup_n_u64(0), data, factor);
    svuint64_t vecSca = svld1_u64(svptrue_pat_b64(SV_VL4), scale);
    return svaddv_u64(svptrue_pat_b64(SV_VL4), svmul_u64_x(svptrue_pat_b64(SV_VL4), res, vecSca));
}

sonic_force_inline uint64_t simd_str2int(const char *c, int &man_nd)
{
    svbool_t pgAll = svptrue_pat_b16(SV_VL16);
    svuint16_t data = svld1sb_u16(pgAll, reinterpret_cast<const signed char *>(&c[0]));
    data = svsub_n_u16_x(pgAll, data, '0');
    svbool_t gt_nine = svcmpgt_n_u16(pgAll, data, 9);
    int num_end_idx = 16;
    if (svptest_any(pgAll, gt_nine)) {
        num_end_idx = svcntp_b16(pgAll, svbrkb_z(pgAll, gt_nine));
    }
    man_nd = man_nd < num_end_idx ? man_nd : num_end_idx;

    switch (man_nd) {
        case 1:
            return svlasta_u16(svpfalse_b(), data);
        case 0X2:
            return svlasta_u16(svpfalse_b(), data) * STR2INT_LOWEST_VALUE_MUL_FACTOR +
            svlasta_u16(svptrue_pat_b16(SV_VL1), data);
        case 0X3:
            return svlasta_u16(svpfalse_b(), data) * STR2INT_HUNDRED_MUL_FACTOR +
            svlasta_u16(svptrue_pat_b16(SV_VL1), data) *
            STR2INT_LOWEST_VALUE_MUL_FACTOR + svlasta_u16(svptrue_pat_b16(SV_VL2), data);
        case 0x4: {
            static uint16_t mulFactor[4] = { 1000, 100, 10, 1 };
            svuint16_t factor = svld1_u16(svptrue_pat_b16(SV_VL4), mulFactor);
            svuint64_t res = svdot_u64(svdup_n_u64(0), data, factor);
            return svlasta_u64(svpfalse_b(), res);
        }
        case 0x5: {
            static uint32_t mulFactor[8] = { 10000, 1000, 100, 10, 1, 0, 0, 0 };
            return low_half_simd_str2int(data, mulFactor);
        }
        case 0x6: {
            static uint32_t mulFactor[8] = { 100000, 10000, 1000, 100, 10, 1, 0, 0 };
            return low_half_simd_str2int(data, mulFactor);
        }
        case 0x7: {
            static uint32_t mulFactor[8] = { 1000000, 100000, 10000, 1000, 100, 10, 1, 0 };
            return low_half_simd_str2int(data, mulFactor);
        }
        case 0x8: {
            static uint32_t mulFactor[8] = { 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1 };
            return low_half_simd_str2int(data, mulFactor);
        }
        case 0x9: {
            static uint32_t mulFactor[8] = { 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1 };
            uint64_t low_value = low_half_simd_str2int(data, mulFactor);
            return (uint64_t)svlastb_u16(svwhilelt_b16_u32(0, 0x9), data) +
                   (uint64_t)low_value * STR2INT_LOWEST_VALUE_MUL_FACTOR;
        }
        case 0xa: {
            static uint32_t mulFactor[8] = { 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1 };
            uint64_t low_value = low_half_simd_str2int(data, mulFactor);
            return (uint64_t)svlasta_u16(svwhilelt_b16_u32(0, 0x9), data) +
                   (uint64_t)svlastb_u16(svwhilelt_b16_u32(0, 0x9), data) * STR2INT_LOWEST_VALUE_MUL_FACTOR +
                   low_value * STR2INT_HUNDRED_MUL_FACTOR;
        }
        case 0xb: {
            static uint32_t mulFactor[8] = { 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1 };
            uint64_t low_value = low_half_simd_str2int(data, mulFactor);
            return (uint64_t)svlasta_u16(svwhilelt_b16_u32(0, 0xa), data) +
                   (uint64_t)svlasta_u16(svwhilelt_b16_u32(0, 0x9), data) * STR2INT_LOWEST_VALUE_MUL_FACTOR +
                   (uint64_t)svlastb_u16(svwhilelt_b16_u32(0, 0x9), data) * STR2INT_HUNDRED_MUL_FACTOR +
                   low_value * STR2INT_THOUSAND_MUL_FACTOR;
        }
        case 0xc: {
            static uint16_t mulFactor[16] = { 1000, 100, 10, 1, 1000, 100, 10, 1, 1000, 100, 10, 1, 0, 0, 0, 0 };
            static uint64_t scale[4] = { 100000000, 10000, 1, 0 };
            return dot_simd_str2int(scale, mulFactor, data);
        }

        case 0xd: {
            static uint16_t mulFactor[16] = { 1000, 100, 10, 1, 1000, 100, 10, 1, 1000, 100, 10, 1, 1, 0, 0, 0 };
            static uint64_t scale[4] = { 1000000000, 100000, 10, 1 };
            return dot_simd_str2int(scale, mulFactor, data);
        }

        case 0xe: {
            static uint16_t mulFactor[16] = { 1000, 100, 10, 1, 1000, 100, 10, 1, 1000, 100, 10, 1, 10, 1, 0, 0 };
            static uint64_t scale[4] = { 10000000000, 1000000, 100, 1 };
            return dot_simd_str2int(scale, mulFactor, data);
        }
        case 0xf: {
            static uint16_t mulFactor[16] = { 1000, 100, 10, 1, 1000, 100, 10, 1, 1000, 100, 10, 1, 100, 10, 1, 0 };
            static uint64_t scale[4] = { 100000000000, 10000000, 1000, 1 };
            return dot_simd_str2int(scale, mulFactor, data);
        }
        case 0x10: {
            static uint16_t mulFactor[16] = { 1000, 100, 10, 1, 1000, 100, 10, 1, 1000, 100, 10, 1, 1000, 100, 10, 1 };
            static uint64_t scale[4] = { 1000000000000, 100000000, 10000, 1 };
            return dot_simd_str2int(scale, mulFactor, data);
        }
        default:
            return 0;
    }
    return 1;
}

}  // namespace sve_256
}  // namespace internal
}  // namespace sonic_json
