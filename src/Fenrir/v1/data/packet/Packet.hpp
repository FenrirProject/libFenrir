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
#include "Fenrir/v1/crypto/Crypto.hpp"
#include "Fenrir/v1/data/packet/Stream.hpp"
#include "Fenrir/v1/util/endian.hpp"
#include "Fenrir/v1/util/Random.hpp"
#include <gsl/span>
#include <type_safe/constrained_type.hpp>
#include <type_safe/strong_typedef.hpp>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {

class Random;

// Strong typedef for Connection id,to avoid up/down casting, and other
// operations which do not make sense for an identifier.
struct FENRIR_LOCAL  Conn_ID : type_safe::strong_typedef<Conn_ID, uint32_t>,
                type_safe::strong_typedef_op::equality_comparison<Conn_ID>,
                type_safe::strong_typedef_op::relational_comparison<Conn_ID>
    { using strong_typedef::strong_typedef; };

constexpr Conn_ID Conn_Reserved {3}; // 0, 1, 2 are reserved

constexpr uint32_t PKT_HEADER = sizeof(Conn_ID) + 1;
constexpr uint32_t PKT_MINLEN = PKT_HEADER + STREAM_MINLEN;

class FENRIR_LOCAL Packet
{
public:
    enum class FENRIR_LOCAL Alignment_Flag : uint8_t {
        UINT8  = 0x00,
        UINT16 = 0x01,
        UINT32 = 0x02,
        UINT64 = 0x03
    };
    enum class FENRIR_LOCAL Alignment_Byte : uint8_t {
        UINT8  = 1,
        UINT16 = 2,
        UINT32 = 4,
        UINT64 = 8
    };

    // all is stored in "raw". "stream is a data structure with pointer for
    // easy access, but does not really contain the data.
    std::vector<uint8_t> raw;
    std::vector<Stream> stream;

    Packet (std::vector<uint8_t> &&data)
        : raw (std::move(data)) {}
    Packet (const size_t size)
        : raw (size, 0) {}

    Packet() = delete;
    Packet (const Packet&) = delete;
    Packet& operator= (const Packet&) = delete;
    Packet (Packet &&) = default;
    Packet& operator= (Packet &&) = default;
    ~Packet() = default;

    explicit operator bool() const
        { return raw.size() >= PKT_MINLEN; }

    void set_header (const Conn_ID conn, const uint8_t padding,
                                                            Random *const rnd)
    {
        if (raw.size() <= sizeof(Conn_ID) + 1 + padding)
            return; // just to be safe

        uint8_t *bytes = raw.data();
        *reinterpret_cast<uint32_t*> (bytes) =
                                h_to_l<uint32_t> (static_cast<uint32_t> (conn));
        bytes += sizeof(Conn_ID);
        *bytes = padding;
        ++bytes;
        rnd->uniform<uint8_t> (gsl::span<uint8_t> {bytes, padding});
    }

    Conn_ID connection_id() const
    {
        const uint32_t *id = reinterpret_cast<const uint32_t*> (raw.data());
        return Conn_ID (l_to_h<uint32_t>(*id));
    }

    uint8_t padding() const
        { return raw[sizeof(Conn_ID)]; }

    // add a stream and return a pointer to it.
    Stream *add_stream (const Stream_ID stream_id, const Stream::Fragment type,
                                                        const Counter counter,
                                                        const uint16_t size)
    {
        if (!*this)
            return nullptr;
        uint8_t *p8;
        const uint8_t *raw_end = raw.data() + raw.size();
        auto last_stream = stream.rbegin();
        if (last_stream == stream.rend()) {
            p8 = raw.data() + sizeof(Conn_ID) + 1 + padding();
        } else {
            p8 = last_stream->raw().data() +last_stream->raw().size();
        }
        if (p8 >= raw_end || ((raw_end - p8) - STREAM_MINLEN) < size)
            return nullptr;
        stream.emplace_back (stream_id, type, counter,
                                gsl::span<uint8_t> (p8, STREAM_MINLEN +size));
        return &*stream.rbegin();
    }

    Impl::Error parse (const gsl::span<uint8_t> data, const Alignment_Byte _al)
    {
        if (data.size() < PKT_MINLEN)
            return Impl::Error::WRONG_INPUT;
        auto bytes = 0;
        // padding could be misaligned (?)
        const uint8_t padding = 1 + data[0] +
                                        (data[0] % static_cast<uint8_t> (_al));
        if (padding > data.size())
            return Impl::Error::WRONG_INPUT;
        bytes += padding;
        uint16_t streams_found = 0;
        while (bytes < data.size()) {
            const Stream s (data.subspan (bytes, data.size() - bytes));
            if (!s)
                break;
            const uint8_t align_skip = static_cast<uint8_t> (s.size() %
                                                static_cast<uint32_t> (_al));
            bytes += s.size() + align_skip;
            ++streams_found;
        }
        if ((data.size() - bytes) > STREAM_MINLEN)
            return Impl::Error::WRONG_INPUT;
        stream.reserve (streams_found);
        // now actually append the streams to the vector
        bytes = padding;
        while (bytes != data.size()) {
            stream.emplace_back (data.subspan (bytes, data.size() - bytes));
            const auto s = stream.crbegin();
            const uint8_t align_skip = static_cast<uint8_t> (s->size() %
                                                static_cast<uint32_t> (_al));
            bytes += s->size() + align_skip;
        }

        return Impl::Error::NONE;
    }
    gsl::span<uint8_t> data_no_id()
    {
        return gsl::span<uint8_t> (raw.data() + sizeof (Conn_ID),
                        static_cast<ssize_t> (raw.size() - sizeof (Conn_ID)));
    }
    static Packet::Alignment_Byte Flag_To_Byte (
                                            const Packet::Alignment_Flag flag)
    {
        switch (flag) {
        case Packet::Alignment_Flag::UINT8:
            return Packet::Alignment_Byte::UINT8;
        case Packet::Alignment_Flag::UINT16:
            return Packet::Alignment_Byte::UINT16;
        case Packet::Alignment_Flag::UINT32:
            return Packet::Alignment_Byte::UINT32;
        case Packet::Alignment_Flag::UINT64:
            return Packet::Alignment_Byte::UINT64;
        }
        assert (false && "Fenrir: wrong alignment?");
        return Packet::Alignment_Byte::UINT8;
    }
};

using Packet_NN = typename type_safe::constrained_type<Packet*,
                                            type_safe::constraints::non_null>;

} // namespace Impl
} // namespace Fenrir__v1
