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
#include "base.h"

namespace sonic_json {
namespace internal {
namespace sve2_256 {

sonic_force_inline uint64_t to_bitmask(uint8x16_t v)
{
    return vget_lane_u64(vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(v), 0x4)), 0);
}

template <typename T>
struct simd8;

//
// Base class of simd8<uint8_t> and simd8<bool>, both of which use uint8x16_t
// internally.
//
template <typename T, typename Mask = simd8<bool>>
struct base_u8 {
    uint8x16_t value;
    static const int SIZE = sizeof(value);

    // Conversion from/to SIMD register
    sonic_force_inline base_u8(const uint8x16_t _value) : value(_value) {}
    sonic_force_inline operator const uint8x16_t &() const
    {
        return this->value;
    }
    sonic_force_inline operator uint8x16_t &()
    {
        return this->value;
    }

    sonic_force_inline simd8<T> operator&(const simd8<T> other) const
    {
        return vandq_u8(*this, other);
    }

    friend sonic_force_inline Mask operator==(const simd8<T> lhs, const simd8<T> rhs)
    {
        return vceqq_u8(lhs, rhs);
    }
};

// SIMD byte mask type (returned by things like eq and gt)
template <>
struct simd8<bool> : base_u8<bool> {
    typedef uint16_t bitmask_t;
    typedef uint32_t bitmask2_t;

    static sonic_force_inline simd8<bool> splat(bool _value)
    {
        return vmovq_n_u8(uint8_t(-(!!_value)));
    }

    sonic_force_inline simd8(const uint8x16_t _value) : base_u8<bool>(_value) {}
    // False constructor
    sonic_force_inline simd8() : simd8(vdupq_n_u8(0)) {}
    // Splat constructor
    sonic_force_inline simd8(bool _value) : simd8(splat(_value)) {}
};

// Unsigned bytes
template <>
struct simd8<uint8_t> : base_u8<uint8_t> {
    static sonic_force_inline uint8x16_t splat(uint8_t _value)
    {
        return vmovq_n_u8(_value);
    }
    static sonic_force_inline uint8x16_t zero()
    {
        return vdupq_n_u8(0);
    }
    static sonic_force_inline uint8x16_t load(const uint8_t *values)
    {
        return vld1q_u8(values);
    }

    sonic_force_inline simd8(const uint8x16_t _value) : base_u8<uint8_t>(_value) {}
    // Zero constructor
    sonic_force_inline simd8() : simd8(zero()) {}
    // Array constructor
    sonic_force_inline simd8(const uint8_t values[16]) : simd8(load(values)) {}
    // Splat constructor
    sonic_force_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Member-by-member initialization
    sonic_force_inline simd8(uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4, uint8_t v5, uint8_t v6,
                             uint8_t v7, uint8_t v8, uint8_t v9, uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13,
                             uint8_t v14, uint8_t v15)
        : simd8(uint8x16_t{ v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15 })
    {
    }
};

template <typename T>
struct simd8x64 {
    static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T>);
    static_assert(NUM_CHUNKS == 4, "ARM kernel should use four registers per 64-byte block.");
    const simd8<T> chunks[NUM_CHUNKS];

    simd8x64(const simd8x64<T> &o) = delete;                 // no copy allowed
    simd8x64<T> &operator=(const simd8<T> &other) = delete;  // no assignment allowed
    simd8x64() = delete;                                     // no default constructor allowed

    sonic_force_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1, const simd8<T> chunk2,
                                const simd8<T> chunk3)
        : chunks{ chunk0, chunk1, chunk2, chunk3 }
    {
    }
    sonic_force_inline simd8x64(const T ptr[64])
        : chunks{ simd8<T>::load(ptr), simd8<T>::load(ptr + 16), simd8<T>::load(ptr + 32), simd8<T>::load(ptr + 48) }
    {
    }

    sonic_force_inline uint64_t to_bitmask() const
    {
        const uint8x16_t bit_mask = { 0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                                      0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80 };
        // Add each of the elements next to each other, successively, to stuff each
        // 8 byte mask into one.
        uint8x16_t sum0 = vpaddq_u8(this->chunks[0] & bit_mask, this->chunks[1] & bit_mask);
        uint8x16_t sum1 = vpaddq_u8(this->chunks[2] & bit_mask, this->chunks[3] & bit_mask);
        sum0 = vpaddq_u8(sum0, sum1);
        sum0 = vpaddq_u8(sum0, sum0);
        return vgetq_lane_u64(vreinterpretq_u64_u8(sum0), 0);
    }

    sonic_force_inline uint64_t eq(const T m) const
    {
        const simd8<T> mask = simd8<T>::splat(m);
        return simd8x64<bool>(this->chunks[0] == mask, this->chunks[1] == mask, this->chunks[0x2] == mask,
                              this->chunks[0x3] == mask)
            .to_bitmask();
    }
};  // struct simd8x64<T>

}  // namespace sve2_256
}  // namespace internal
}  // namespace sonic_json
