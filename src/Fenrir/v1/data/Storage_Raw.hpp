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
#include "Fenrir/v1/data/Storage.hpp"
#include "Fenrir/v1/util/math.hpp"
#include <deque>
#include <mutex>
#include <utility>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {


// Track enum to help track flags and state of sent/received data
enum class FENRIR_LOCAL Track : uint8_t {
    START = static_cast<uint8_t> (Stream::Fragment::START),
    END = static_cast<uint8_t> (Stream::Fragment::END),
    MIDDLE = static_cast<uint8_t> (Stream::Fragment::MIDDLE),
    FULL = static_cast<uint8_t> (Stream::Fragment::FULL),
    ACKED = static_cast<uint8_t> (pow (2, 6)),
    UNKNOWN = static_cast<uint8_t> (pow (2, 7)),
};

FENRIR_LOCAL Track operator| (const Track a, const Track b);
FENRIR_LOCAL Track operator& (const Track a, const Track b);
FENRIR_LOCAL bool track_has (const Track value, const Track test);




class FENRIR_LOCAL Storage_Raw final : public Storage
{
    static constexpr Counter initial_window {15000}; // 15Kb
public:
    Storage_Raw()
        : _window_start (0), _window_size (initial_window), _reliable (false)
    {
        _raw.resize (static_cast<uint32_t> (initial_window),
                                                        {0x00, Track::UNKNOWN});
    }
    ~Storage_Raw() {}

    Impl::Error add_stream (const Stream_ID stream,
                                const Storage_t storage,
                                const type_safe::optional<Counter> window_start,
                                const type_safe::optional<Counter> window_size,
                                const Storage::IO type) override;
    Impl::Error del_stream (const Stream_ID stream,
                                            const Storage::IO type) override;
    Storage_t type (const Stream_ID stream) override;

    Impl::Error recv_data (const Stream_ID stream, const Counter counter,
                                        gsl::span<const uint8_t> data,
                                        const Stream::Fragment type) override;

    Impl::Error recv_ack (const Stream_ID stream,
                                                const Counter full_received_til,
                                                const Pairs chunk) override;
    Impl::Error recv_nack (const Stream_ID stream,
                                                const Counter last_received,
                                                const Pairs chunk) override;

    Error add_data (const Stream_ID stream,
                                const gsl::span<const uint8_t> data) override;
    // hack: nither receive, nor send. actually both.
    // rationale: when we send the link-activation data, we do not actually
    // want to keep track of it. But we ned to send it with a valid
    // stream/counter. if we just add it to the stram we can not control
    // on which link it will be sent. NOT used any other way.
    //   => we do not add data here. we only register a "hole".
    //   => only valid for the UNRELIABLE UNORDERED COMPLETE comtrol stream.
    Counter reserve_data (const Stream_ID stream, const uint32_t size) override;

    // TODO: move to GSL::SPAN
    std::tuple<Stream::Fragment, Counter, std::vector<uint8_t>> send_data (
                                            const Stream_ID stream,
                                            const uint32_t max_data) override;
    std::pair<Counter, Pairs> send_ack (const Stream_ID stream) override;
    std::pair<Counter, Pairs> send_nack (const Stream_ID stream) override;

    std::tuple<Counter, Stream::Fragment, std::vector<uint8_t>> get_user_data (
                                            const Stream_ID stream) override;
private:

    std::mutex _mtx;
    std::vector<std::pair<Stream_ID, Storage_t>> _streams;
    // use deque for quick append
    // yes, we trck each byte individually. Rewrite for better performance?
    std::deque<std::pair<uint8_t, Track>> _raw;
    // quick hack to support "reserve_data()"
    uint32_t _hole;
    Counter _window_start, _window_size;
    uint32_t _raw_idx_start;
    Stream_ID _output; // only 1 output for user data makes sense.
    bool _reliable; // activate acks.

    Impl::Error del_data (const Stream_ID stream, const Counter from,
                                                            const Counter to);
};

} // namespace Impl
} // namespace Fenrir__v1

