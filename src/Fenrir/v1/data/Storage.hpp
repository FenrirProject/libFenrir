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
#include "Fenrir/v1/data/packet/Stream.hpp"
#include "Fenrir/v1/data/Storage_t.hpp"
#include <gsl/span>
#include <type_safe/optional.hpp>
#include <utility>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {

// Use this class to handle stream.
// It will be put in a shared_ptr, which can be shared by multiple streams.
// This will let you efficiently handle data between multiple streams
// so you can have -for example- a reliable data stream and an unreliable
// FEC stream without duplicating data.
// Handle any ack/nack as you want.
// The sky is the limit, go crazy.(actaul limit: maybe the RAM, don't go crazy).

// One of the reasons why this will be handled with shared_ptr is so that we
// can have an unreliable multicast stream and a reliable unicast stream for
// the same user data. This will let us support things like reliable multicast.

// TODO: make a plugin
class FENRIR_LOCAL Storage
{
    // NOTE: you need to handle multithreading yourself when subclassing.
public:
    enum class FENRIR_LOCAL IO : uint8_t { INPUT = 0x01, OUTPUT= 0x02 };

    Storage() = default;
    Storage (const Storage&) = default;
    Storage& operator= (const Storage&) = default;
    Storage (Storage &&) = default;
    Storage& operator= (Storage &&) = default;
    virtual ~Storage() {}

    virtual Impl::Error add_stream (const Stream_ID stream,
                                const Storage_t storage,
                                const type_safe::optional<Counter> window_start,
                                const type_safe::optional<Counter> window_size,
                                const Storage::IO type) = 0;
    virtual Impl::Error del_stream (const Stream_ID stream,
                                                    const Storage::IO type) = 0;
    virtual Storage_t type (const Stream_ID stream) = 0;

    virtual Impl::Error recv_data (const Stream_ID stream,
                                            const Counter counter,
                                            gsl::span<const uint8_t> data,
                                            const Stream::Fragment type) = 0;
    using Pairs = std::vector<std::pair<Counter, Counter>>;
    virtual Impl::Error recv_ack (const Stream_ID stream,
                                            const Counter full_received_til,
                                            const Pairs chunk) = 0;
    virtual Impl::Error recv_nack (const Stream_ID stream,
                                            const Counter last_received,
                                            const Pairs chunk) = 0;

    virtual Error add_data (const Stream_ID stream,
                                    const gsl::span<const uint8_t> data) = 0;
    virtual Counter reserve_data (const Stream_ID stream,
                                                    const uint32_t size) = 0;

    // FIXME: input parameter: output Link_ID.
    //   Reason: to tell rate-limiting where the packet drop occurred
    //   Problem: one packet fails multiple streams!
    // FIXME: instead of returning vector, get Vector_view as input, write there
    virtual std::tuple<Stream::Fragment, Counter, std::vector<uint8_t>>
                                                                    send_data (
                                                const Stream_ID stream,
                                                const uint32_t max_data) = 0;
    // pair: <full_received_till, subsequent_chunk_received.>
    virtual std::pair<Counter, Pairs> send_ack (const Stream_ID stream) = 0;
    // pair: <last received byte, missing chunks>
    virtual std::pair<Counter, Pairs> send_nack (const Stream_ID stream) = 0;
    // pair: <data started at "Counter", (full/start/end/middle message), data>
    virtual std::tuple<Counter, Stream::Fragment, std::vector<uint8_t>>
                                    get_user_data (const Stream_ID stream) = 0;
};

} // namespace Impl
} // namespace Fenrir__v1
