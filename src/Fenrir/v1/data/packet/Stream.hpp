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
#include <cassert>
#include <type_safe/strong_typedef.hpp>
#include <gsl/span>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {

static constexpr uint32_t STREAM_MINLEN {8}; // header bytes

// Strong typedef for Stream id,to avoid up/down casting, and other
// operations which do not make sense for an identifier.
struct FENRIR_LOCAL Stream_ID : type_safe::strong_typedef<Stream_ID, uint16_t>,
                type_safe::strong_typedef_op::equality_comparison<Stream_ID>,
                type_safe::strong_typedef_op::relational_comparison<Stream_ID>
    { using strong_typedef::strong_typedef; };

struct FENRIR_LOCAL Stream_PRIO : type_safe::strong_typedef<Stream_ID, uint8_t>,
                    type_safe::strong_typedef_op::equality_comparison<Stream_ID>
    { using strong_typedef::strong_typedef; };

struct FENRIR_LOCAL Counter : type_safe::strong_typedef<Counter, uint32_t>,
                    type_safe::strong_typedef_op::equality_comparison<Counter>,
                    type_safe::strong_typedef_op::integer_arithmetic<Counter>,
                    type_safe::strong_typedef_op::relational_comparison<Counter>
    { using strong_typedef::strong_typedef; };

constexpr Counter max_counter {pow (2, 30) - 1};

class FENRIR_LOCAL Stream
{
public:
    enum class FENRIR_LOCAL Fragment : uint8_t {
        START = 0x02,
        END = 0x01,
        MIDDLE = 0x00,
        FULL = 0x03
    };

    Stream() = delete;
    Stream (const Stream&) = delete;
    Stream& operator= (const Stream&) = delete;
    Stream (Stream &&) = default;
    Stream& operator= (Stream &&) = default;
    ~Stream() = default;
    // note: this constructor can receive more data than a single stream
    Stream (const gsl::span<uint8_t> data_in)
        : raw_stream (test_span (data_in))
    {}

    Stream (const Stream_ID stream_id, const Fragment type,
                    const Counter counter, gsl::span<uint8_t> view)
        : raw_stream (view)
    {
        assert (raw_stream.size() >= STREAM_MINLEN &&
                raw_stream.size() < (pow (2, 16) + STREAM_MINLEN));
        set_header (stream_id, type, counter);
    }

    explicit operator bool() const
        { return raw_stream.size() >= STREAM_MINLEN; }

    void set_header (const Stream_ID stream_id, const Fragment type,
                                                        const Counter counter)
    {
        uint16_t *raw_16 = reinterpret_cast<uint16_t *> (raw_stream.data());
        *raw_16 = h_to_l<uint16_t> (static_cast<uint16_t> (stream_id));
        ++raw_16;
        *raw_16 = h_to_l<uint16_t> (static_cast<uint16_t> (raw_stream.size()) -
                                                                STREAM_MINLEN);
        ++raw_16;
        uint32_t *raw_32 = reinterpret_cast<uint32_t *> (raw_16);
        uint32_t flags;
        switch (type) {
        case Fragment::START:
            flags = static_cast<uint8_t> (Flags::FRAGSTART);
            break;
        case Fragment::END:
            flags = static_cast<uint8_t> (Flags::FRAGEND);
            break;
        case Fragment::MIDDLE:
            flags = static_cast<uint8_t> (Flags::EMPTY);
            break;
        case Fragment::FULL:
            flags = static_cast<uint8_t> (Flags::FRAGSTART) |
                    static_cast<uint8_t> (Flags::FRAGEND);
            break;
        }
        flags <<= 30;
        *raw_32 = h_to_l<uint32_t> (flags + static_cast<uint32_t> (counter));
    }

    uint32_t size() const
        { return static_cast<uint32_t> (raw_stream.size()); }
    uint32_t data_size() const
        { return static_cast<uint32_t> (raw_stream.size()) - STREAM_MINLEN; }
    Counter counter() const
    {
        constexpr uint32_t mask = ~(static_cast<uint32_t> (Flags::FULL) << 30);
        const uint32_t *counter = reinterpret_cast<const uint32_t*> (
                                                            raw_stream.data());
        ++counter;
        return Counter(l_to_h<uint32_t> (*counter) & mask);
    }
    Stream_ID id() const
    {
        const uint16_t *id = reinterpret_cast<const uint16_t*> (
                                                            raw_stream.data());
        return Stream_ID (l_to_h<uint16_t> (*id));
    }
    Fragment type() const
    {
        const uint8_t *raw_flags = raw_stream.data() + 4;
        Flags flag = static_cast<Flags> (*raw_flags &
                                            static_cast<uint8_t> (Flags::FULL));
        switch (flag) {
        case Flags::EMPTY:
            return Fragment::MIDDLE;
        case Flags::FULL:
            return Fragment::FULL;
        case Flags::FRAGSTART:
            return Fragment::START;
        case Flags::FRAGEND:
            return Fragment::END;
        case Flags::NOSTART:
        case Flags::NOEND:
            break;
        }
        // we never really get here, but this way we avoid warnings
        assert (false && "Fenrir: Stream::type default");
        return Fragment::END;
    }

    gsl::span<uint8_t> raw()
        { return raw_stream; }
    gsl::span<const uint8_t> raw() const
        { return raw_stream; }
    gsl::span<uint8_t> data()
        { return raw_stream.subspan (8); }
    gsl::span<const uint8_t> data() const
        { return raw_stream.subspan (8); }


private:
    gsl::span<uint8_t> raw_stream;
    enum class FENRIR_LOCAL Flags : uint8_t { EMPTY = 0x00, FULL = 0xC0,
                                            FRAGSTART = 0x80, FRAGEND = 0x40,
                                            NOSTART = 0x7F, NOEND = 0xBF};

    static gsl::span<uint8_t> test_span (const gsl::span<uint8_t> data_in)
    {
        if (data_in.size() < STREAM_MINLEN)
            return 0;
        uint16_t *len = reinterpret_cast<uint16_t*> (data_in.data());
        len++;
        int32_t total_len = STREAM_MINLEN + l_to_h<uint16_t> (*len);
        if (data_in.size() < total_len)
            return 0;
        return data_in.subspan (0, total_len);
    }
};

FENRIR_LOCAL Stream::Fragment operator| (const Stream::Fragment a,
                                                    const Stream::Fragment b);
FENRIR_LOCAL Stream::Fragment operator|= (const Stream::Fragment a,
                                                    const Stream::Fragment b);
FENRIR_LOCAL Stream::Fragment operator& (const Stream::Fragment a,
                                                    const Stream::Fragment b);
FENRIR_LOCAL Stream::Fragment operator&= (const Stream::Fragment a,
                                                    const Stream::Fragment b);
FENRIR_LOCAL Stream::Fragment operator! (const Stream::Fragment a);
FENRIR_LOCAL bool fragment_has (const Stream::Fragment value,
                                                const Stream::Fragment test);

} // namespace Impl
} // namespace Fenrir__v1

