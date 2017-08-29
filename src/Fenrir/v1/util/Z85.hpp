/*
 * Copyright (c) 2016-2017, Luca Fulchir<luca@fulchir.it>, All rights reserved.
 *
 * This file is part of libFenrir.
 *
 * libFenrir is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * libFenrir is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and a copy of the GNU Lesser General Public License
 * along with libFenrir.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Fenrir/v1/common.hpp"
#include "Fenrir/v1/util/math.hpp"
#include "Fenrir/v1/util/endian.hpp"
#include <array>
#include <gsl/span>
#include <string.h>
#include <type_traits>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {

// TODO: DEDUPLICATE this code
// TODO: see issue #1 (constant time)

FENRIR_LOCAL constexpr size_t z85_encoded_size (const size_t in);
FENRIR_LOCAL constexpr size_t z85_encoded_size_no_header (const size_t in);
FENRIR_LOCAL constexpr size_t z85_decoded_size_no_header (const size_t in);
FENRIR_LOCAL constexpr size_t z85_decoded_size (
                                            const gsl::span<const uint8_t> in);
FENRIR_LOCAL constexpr bool z85_encode (const gsl::span<const uint8_t> in,
                                                        gsl::span<uint8_t> out);
FENRIR_LOCAL constexpr bool z85_encode_no_header (
                                            const gsl::span<const uint8_t> in,
                                                        gsl::span<uint8_t> out);
FENRIR_LOCAL constexpr bool z85_decode (const gsl::span<const uint8_t> in,
                                                        gsl::span<uint8_t> out);
FENRIR_LOCAL constexpr bool z85_decode_no_header (
                                            const gsl::span<const uint8_t> in,
                                                        gsl::span<uint8_t> out);

namespace {
// extra '\0' at the end, but whatever.
static constexpr std::array<const uint8_t, 86> enc = {
    "0123456789"
    "abcdefghij"
    "klmnopqrst"
    "uvwxyzABCD"
    "EFGHIJKLMN"
    "OPQRSTUVWX"
    "YZ.-:+=^!/"
    "*?&<>()[]{"
    "}@%$#"
};

// how to generate "dec": take "enc" above,
//	for i=0..256
//		if (find i in enc[..])
//			dec[i] = enc[X] + 1
//		else
//			dec[i] = 0
// This way if the character is not valid we get '0x00', else
// we only have to do '-1' to the values on 'dec'
// note: uint8 goes to 256, we only go to 128 (the rest are 0x00)
static constexpr std::array<const uint8_t, 128> dec = {{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x45, 0x00, 0x55, 0x54, 0x53, 0x49, 0x00,
    0x4c, 0x4d, 0x47, 0x42, 0x00, 0x40, 0x3f, 0x46,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0a, 0x41, 0x00, 0x4a, 0x43, 0x4b, 0x48,
    0x52, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
    0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33,
    0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
    0x3c, 0x3d, 0x3e, 0x4e, 0x00, 0x4f, 0x44, 0x00,
    0x00, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21,
    0x22, 0x23, 0x24, 0x50, 0x00, 0x51, 0x00, 0x00
}};
} // empty namespace

FENRIR_LOCAL constexpr size_t z85_encoded_size (const size_t in)
{
    uint32_t add = in % 4 == 0 ? 0 : 1;
    return sizeof(uint8_t) + (in / 4 + add) * 5;
}

FENRIR_LOCAL constexpr size_t z85_encoded_size_no_header (const size_t in)
{
    uint32_t add = in % 4 == 0 ? 0 : 1;
    return (in / 4 + add) * 5;
}

FENRIR_LOCAL constexpr size_t z85_decoded_size_no_header (const size_t in)
{
    if (in % 5 != 0)
        return 0;

    return (in / 5 * 4);
}

FENRIR_LOCAL constexpr size_t z85_decoded_size (
                                            const gsl::span<const uint8_t> in)
{
    if (in.size() < 1 || in[0] < '0' || in[0] > '3' ||
                static_cast<size_t> (in.size()) <= sizeof(uint8_t) ||
                (static_cast<size_t> (in.size()) - sizeof(uint8_t)) % 5 != 0) {
        return 0;
    }
    uint8_t padding = in[0] - '0';
    if (in.size() - padding < 0)
        return 0;
    return ((static_cast<size_t> (in.size()) - sizeof(uint8_t)) / 5 * 4)
                                                                    - padding;
}

FENRIR_LOCAL constexpr bool z85_encode (const gsl::span<const uint8_t> in,
                                                        gsl::span<uint8_t> out)
{
    if (static_cast<size_t> (out.size()) !=
                        z85_encoded_size (static_cast<size_t> (in.size()))) {
        return false;
    }

    const uint8_t *p = in.data();
    out[0] = '0';
    if (in.size() % 4 != 0) {
        out[0] += 4 - in.size() % 4;
    }

    ssize_t out_idx = sizeof(uint8_t);
    ssize_t runs = in.size();
    for (; runs > 0; p += 4, runs -= 4) {
        uint32_t tmp = 0;
        const size_t len = runs >= static_cast<ssize_t> (sizeof(tmp)) ?
                                    sizeof(tmp) : static_cast<size_t> (runs);
        memcpy (&tmp, p, len);
        tmp = h_to_l<uint32_t> (tmp);
        auto idx = static_cast<uint32_t> ((tmp / pow (85, 4))) % 85;
        out[out_idx++] = (enc [static_cast<size_t> (idx)]);
        idx = static_cast<uint32_t> ((tmp / pow (85, 3))) % 85;
        out[out_idx++] = (enc [static_cast<size_t> (idx)]);
        idx = static_cast<uint32_t> ((tmp / pow (85, 2))) % 85;
        out[out_idx++] = (enc [static_cast<size_t> (idx)]);
        idx = static_cast<uint32_t> ((tmp / 85)) % 85;
        out[out_idx++] = (enc [static_cast<size_t> (idx)]);
        idx = static_cast<uint32_t> (tmp) % 85;
        out[out_idx++] = (enc [static_cast<size_t> (idx)]);
    }

    return true;
}

FENRIR_LOCAL constexpr bool z85_encode_no_header (
                                            const gsl::span<const uint8_t> in,
                                                        gsl::span<uint8_t> out)
{
    if (static_cast<size_t> (out.size()) !=
                z85_encoded_size_no_header (static_cast<size_t> (in.size()))) {
        return false;
    }

    const uint8_t *p = in.data();

    ssize_t out_idx = 0;
    ssize_t runs = in.size();
    for (; runs > 0; p += 4, runs -= 4) {
        uint32_t tmp = 0;
        const size_t len = runs >= static_cast<ssize_t> (sizeof(tmp)) ?
                                sizeof(tmp) : static_cast<size_t> (runs);
        memcpy (&tmp, p, len);
        tmp = h_to_l<uint32_t> (tmp);
        auto idx = static_cast<uint32_t> ((tmp / pow (85, 4))) % 85;
        out[out_idx++] = (enc [static_cast<size_t> (idx)]);
        idx = static_cast<uint32_t> ((tmp / pow (85, 3))) % 85;
        out[out_idx++] = (enc [static_cast<size_t> (idx)]);
        idx = static_cast<uint32_t> ((tmp / pow (85, 2))) % 85;
        out[out_idx++] = (enc [static_cast<size_t> (idx)]);
        idx = static_cast<uint32_t> ((tmp / 85)) % 85;
        out[out_idx++] = (enc [static_cast<size_t> (idx)]);
        idx = static_cast<uint32_t> (tmp) % 85;
        out[out_idx++] = (enc [static_cast<size_t> (idx)]);
    }

    return true;
}

FENRIR_LOCAL constexpr bool z85_decode (const gsl::span<const uint8_t> in,
                                                        gsl::span<uint8_t> out)
{
    if (in.size() < 1 || in[0] < '0' || in[0] > '3' ||
                (static_cast<size_t> (in.size()) - sizeof(uint8_t)) % 5 != 0 ||
                    static_cast<size_t> (out.size()) != z85_decoded_size (in)) {
        return false;
    }
    int32_t exp = 4;
    uint32_t val_dec = 0;

    uint8_t *p_out = out.data();
    ssize_t out_size = out.size();

    for (ssize_t i = sizeof(uint8_t); i < in.size(); ++i) {
        if (in[i] >= 128 || dec[in[i]] == 0)	// err: non valid char found
            return false;
        val_dec += (dec[in[i]] - 1) * pow (85, static_cast<uint32_t> (exp));
        --exp;
        if (exp < 0) {
            size_t copied_size = out_size >= 4 ? sizeof(val_dec) :
                                                static_cast<size_t> (out_size);
            val_dec = l_to_h<uint32_t> (val_dec);
            memcpy (p_out, &val_dec, copied_size);
            p_out += sizeof(val_dec);
            out_size -= copied_size;

            exp = 4;
            val_dec = 0;
        }
    }

    return true;
}

FENRIR_LOCAL constexpr bool z85_decode_no_header (
                                            const gsl::span<const uint8_t> in,
                                                        gsl::span<uint8_t> out)
{
    if (static_cast<size_t> (in.size()) % 5 != 0 ||
                                    static_cast<size_t> (out.size()) !=
                                        z85_decoded_size_no_header (
                                            static_cast<size_t> (in.size()))) {
        return false;
    }
    int32_t exp = 4;
    uint32_t val_dec = 0;

    uint8_t *p_out = out.data();
    ssize_t out_size = out.size();

    for (ssize_t i = 0; i < in.size(); ++i) {
        if (in[i] >= 128 || dec[in[i]] == 0)	// err: non valid char found
            return false;
        val_dec += (dec[in[i]] - 1) * pow (85, static_cast<uint32_t> (exp));
        --exp;
        if (exp < 0) {
            size_t copied_size = out.size() >= 4 ? sizeof(val_dec) :
                                            static_cast<size_t> (out.size());
            val_dec = l_to_h<uint32_t> (val_dec);
            memcpy (p_out, &val_dec, copied_size);
            p_out += sizeof(val_dec);
            out_size -= copied_size;

            exp = 4;
            val_dec = 0;
        }
    }

    return true;
}

} // namespace Impl
} // namespace Fenrir__v1
