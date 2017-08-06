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

#include "Fenrir/v1/data/Storage_Raw.hpp"
#include <algorithm>
#include <cmath>

namespace Fenrir__v1 {
namespace Impl {

constexpr Counter Storage_Raw::initial_window;

Track operator| (const Track a, const Track b)
{
    return static_cast<Track> (static_cast<uint8_t> (a) |
                                                    static_cast<uint8_t> (b));
}

Track operator& (const Track a, const Track b)
{
    return static_cast<Track> (static_cast<uint8_t> (a) &
                                                    static_cast<uint8_t> (b));
}

bool track_has (const Track value, const Track test)
    { return (value & test) == test; }

Impl::Error Storage_Raw::add_stream (const Stream_ID stream,
                                const Storage_t storage,
                                const type_safe::optional<Counter> window_start,
                                const type_safe::optional<Counter> window_size,
                                const Storage::IO type)
{
    FENRIR_UNUSED (type);
    if (_streams.size() == 0) {
        if (!window_start.has_value() || !window_size.has_value())
            return Impl::Error::WRONG_INPUT;
        _output = stream;
        _window_start = window_start.value();
        _window_size = window_size.value();
        _raw_idx_start = 0;
    }
    if (window_start.has_value() || window_size.has_value())
        return Impl::Error::WRONG_INPUT; // makes sense only on the first stream

    std::lock_guard<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);
    auto tmp = std::lower_bound (_streams.begin(), _streams.end(), stream,
                        [] (const std::pair<Stream_ID, Storage_t> a,
                                                            const Stream_ID id)
                            { return id < std::get<Stream_ID> (a); });
    if (std::get<Stream_ID> (*tmp) == stream)
        return Impl::Error::ALREADY_PRESENT;
    _streams.emplace_back (stream, storage);
    std::sort (_streams.begin(), _streams.end(),
                            [] (const std::pair<Stream_ID, Storage_t> a,
                                const std::pair<Stream_ID, Storage_t> b)
                { return std::get<Stream_ID> (a) < std::get<Stream_ID> (b); });
    if (storage_t_has (storage, Storage_t::RELIABLE))
        _reliable = true;
    return Error::NONE;
}

Impl::Error Storage_Raw::del_stream (const Stream_ID stream,
                                                        const Storage::IO type)
{
    FENRIR_UNUSED (type);
    std::lock_guard<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);
    auto tmp = std::find_if (_streams.begin(), _streams.end(),
                        [stream] (const std::pair<Stream_ID, Storage_t> a)
                            { return stream < std::get<Stream_ID> (a); });
    if (tmp == _streams.end())
        return Impl::Error::WRONG_INPUT;
    auto next = tmp + 1;
    while (next != _streams.end()) {
        *tmp = std::move(*next);
        ++tmp;
        ++next;
    }
    if (_streams.size() == 0)
        _raw.resize (0);
    return Impl::Error::NONE;
}

Storage_t Storage_Raw::type (const Stream_ID stream)
{
    std::lock_guard<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);
    auto tmp = std::lower_bound (_streams.begin(), _streams.end(), stream,
                        [] (const std::pair<Stream_ID, Storage_t> a,
                                                            const Stream_ID id)
                            { return id < std::get<Stream_ID> (a); });
    if (std::get<Stream_ID> (*tmp) != stream)
        return Storage_t::NOT_SET;
    return std::get<Storage_t> (*tmp);
}


Impl::Error Storage_Raw::recv_data (const Stream_ID stream,
                                                const Counter counter,
                                                gsl::span<const uint8_t> data,
                                                const Stream::Fragment type)
{
    // add data to _raw. check both the flags and existing data
    // before writing anything.

    FENRIR_UNUSED (stream); // all data goes to the same place.
    std::lock_guard<std::mutex> lock (_mtx);
    FENRIR_UNUSED(lock);

    if (data.size() == 0) // now safely assume we have data
        return Impl::Error::NONE;
    Counter shift_counter;
    Counter max_copied = Counter (static_cast<uint32_t> (data.size()));
    // check data boundaries
    if (counter >= _window_start) {
        shift_counter = counter - _window_start;
    } else {
        shift_counter = counter + (max_counter - _window_start);
    }
    if (shift_counter > _window_size)
        return Impl::Error::WRONG_INPUT; // over our window size
    if (data.size() > static_cast<uint32_t> (_window_size - shift_counter))
        max_copied = _window_size - shift_counter;


    uint32_t data_idx = 0;
    uint32_t raw_idx = static_cast<uint32_t> (shift_counter);
    // check for conflicting flags
    if (counter != _window_start) {
        // check conflicts with data bafore "counter"
        const Track before = _raw[raw_idx > 0 ?
                                        raw_idx - 1 : _raw.size() - 1].second;
        switch (before) {
        case Track::START:
            [[clang::fallthrough]];
        case Track::MIDDLE:
            if (fragment_has (type, Stream::Fragment::START))
                return Impl::Error::WRONG_INPUT;
            break;
        case Track::END:
            [[clang::fallthrough]];
        case Track::FULL:
            if (fragment_has (type, Stream::Fragment::START))
                return Impl::Error::WRONG_INPUT;
            break;
        case Track::ACKED: // can't happen
            assert (false && "Fenrir: Storage_Raw enum error\n");
            break;
        case Track::UNKNOWN:
            // not-received data.
            break;
        }
    }
    // check "data" head
    if (_raw[raw_idx].second != Track::UNKNOWN) {
        if (data.size() > 1) {
            if (std::get<Track> (_raw[raw_idx]) !=
                        static_cast<Track> (type & Stream::Fragment::START) ||
                                            _raw[raw_idx].first != data[0]) {
                return Impl::Error::WRONG_INPUT;
            }
        } else if (data.size() == 1) {
            // if (size == 1) then "type" can be == Track::FULL
            // 1 byte messages might seem nonsense, but it would not be the
            // first nonsense nor the worst.
            if (_raw[raw_idx].second != static_cast<Track> (type) ||
                                            _raw[raw_idx].first != data[0]) {
                return Impl::Error::WRONG_INPUT;
            }
        }
    }
    ++data_idx;
    ++raw_idx;
    raw_idx %= static_cast<uint32_t> (_window_size);
    while (data_idx < (static_cast<uint32_t> (max_copied) - 1)) {
        // check conflicts with existing data (begin/end of "data" exluded)
        if (_raw[raw_idx].second != Track::UNKNOWN) {
            if (_raw[raw_idx].second != Track::MIDDLE ||
                                        _raw[raw_idx].first != data[data_idx]) {
                return Impl::Error::WRONG_INPUT;
            }
        }
        ++data_idx;
        ++raw_idx;
        raw_idx %= static_cast<uint32_t> (_window_size);
    }
    // check tail of "data"
    if (data_idx < data.size()) {
        if (_raw[raw_idx].second != Track::UNKNOWN) {
            if (data.size() > 1) {
                if (std::get<Track> (_raw[raw_idx]) !=
                        static_cast<Track> (type & Stream::Fragment::END) ||
                                        _raw[raw_idx].first != data[data_idx]) {
                        return Impl::Error::WRONG_INPUT;
                }
            // } else if (data.size() == 1) {
            // can't happen. lready checked anyway
            }
        }
    }
    // check flag conflicts after "data", but only if we did not fill our
    // receive window.
    if (((shift_counter + max_copied) % _window_size) != _window_start) {
        raw_idx = static_cast<uint32_t> ((shift_counter + max_copied)
                                                                % _window_size);
        const Track after = _raw[raw_idx].second;
        switch (after) {
        case Track::START:
            [[clang::fallthrough]];
        case Track::MIDDLE:
            if (fragment_has (type, Stream::Fragment::END))
                return Impl::Error::WRONG_INPUT;
            break;
        case Track::END:
            [[clang::fallthrough]];
        case Track::FULL:
            if (fragment_has (type, Stream::Fragment::END))
                return Impl::Error::WRONG_INPUT;
            break;
        case Track::ACKED: // can't happen
            assert (false && "Fenrir: Storage_Raw enum error\n");
            break;
        case Track::UNKNOWN:
            // not-received data.
            break;
        }
    }



    // Conflict check done. Save the data
    data_idx = 0;
    raw_idx = static_cast<uint32_t> (shift_counter);
    // head
    _raw[raw_idx].first = data[data_idx];
    if (data.size() > 1) {
        _raw[raw_idx].second = static_cast<Track> (
                                                type & Stream::Fragment::START);
    } else {
        _raw[raw_idx].second = static_cast<Track> (type);
    }
    ++data_idx;
    ++raw_idx;
    raw_idx %= static_cast<uint32_t> (_window_size);
    while (data_idx < (static_cast<uint32_t> (max_copied) - 1)) {
        _raw[raw_idx].first = data[data_idx];
        _raw[raw_idx].second = Track::MIDDLE;
        ++data_idx;
        ++raw_idx;
        raw_idx %= static_cast<uint32_t> (_window_size);
    }
    // tail
    if (data_idx < static_cast<uint32_t> (max_copied)) {
        _raw[raw_idx].second = static_cast<Track> (
                                                type & Stream::Fragment::END);
    }
    return Impl::Error::NONE;
}


//////////////
// RECEIVE ACK
//////////////

Impl::Error Storage_Raw::recv_ack (const Stream_ID stream,
                                               const Counter full_received_til,
                                               const Pairs chunk)
{
    // TODO: delete ACKed data
    std::lock_guard<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);

    Counter shift_to;
    if (full_received_til >= _window_start) {
        shift_to = full_received_til - _window_start;
    } else {
        shift_to = full_received_til + (max_counter - _window_start);
    }
    if (shift_to > _window_size)
        return Impl::Error::WRONG_INPUT;

    // check if we have such a stream
    const std::pair<Stream_ID, Storage_t> search_id = { stream,
                                                            Storage_t::NOT_SET};
    if (!std::binary_search (_streams.begin(), _streams.end(), search_id,
                                    [] (const auto &a, const auto &b) // a < b ?
                                                { return a.first < b.first; })){
        return Impl::Error::WRONG_INPUT;
    }
    del_data (stream, _window_start, full_received_til);
    for (const auto &p : chunk)
        del_data (stream, p.first, p.second);
    return Impl::Error::NONE;
}

Impl::Error Storage_Raw::del_data (const Stream_ID stream, const Counter from,
                                                            const Counter to)
{
    FENRIR_UNUSED (stream);
    // lock *before* calling this method!

    // we want to search and delete a chunk of data in _map

    Counter shift_from, shift_to;
    // check data boundaries
    if (from >= _window_start) {
        shift_from = from - _window_start;
    } else {
        shift_from = from + (max_counter - _window_start);
    }
    if (to >= _window_start) {
        shift_from = from - _window_start;
    } else {
        shift_from = from + (max_counter - _window_start);
    }
    if (shift_to > shift_from || shift_from > _window_size ||
                                                    shift_to > _window_size) {
        return Impl::Error::WRONG_INPUT; // over our window size
    }

    // mark data as  acknowledged
    uint32_t idx = static_cast<uint32_t> (shift_from);
    while (idx <= static_cast<uint32_t> (shift_to)) {
        _raw[idx].second = Track::ACKED;
        _raw[idx].first = 0x00;
        ++idx;
        idx %= static_cast<uint32_t> (_window_size);
    }
    // shift receiving window
    if (shift_from == _window_start) {
        idx = _raw_idx_start;
        while (_raw[idx].second == Track::ACKED) {
            _raw[idx].second = Track::UNKNOWN;
            ++idx;
            idx %= static_cast<uint32_t> (_window_size);
            ++_window_start;
        }
    }
    return Impl::Error::NONE;
}

///////////////
// RECEIVE NACK
///////////////

Impl::Error Storage_Raw::recv_nack (const Stream_ID stream,
                                                    const Counter last_received,
                                                    const Pairs chunk)
{
    FENRIR_UNUSED (stream);
    // TODO: reschedule data
    // NOTE: malicious/broken clients as well as network problems might mean
    // that we can receive NACK for data that has already been ACKed.
    // Although not a security risk, only non-ACKed data can be NACK.

}


Error Storage_Raw::add_data (const Stream_ID stream,
                                            const gsl::span<const uint8_t> data)
{
    FENRIR_UNUSED (stream);
    // FIXME: redo storage
    // problem: we want a storage class that can be used with multiple streams,
    // both as sender and receiver.
    // now we don't know how to add data without having the counter.
    //  * when adding user data we need to report the error and counter after
    //    the data, so that the user can add data in chunks
    //  * we can't/should not buffer too much sending data. what is the limit?
    //    do we use the sending window as a limit? seems too little, what
    //    happens to the control stream if there is no space?
    return Impl::Error::WRONG_INPUT;
}

///////////////
// RESERVE DATA
///////////////

Counter Storage_Raw::reserve_data (const Stream_ID stream, const uint32_t size)
{
    // kind of a hack. pretend to add "size" bytes, and reserve the
    // full-message counter for it.
    // This is used to generate and send the link-activation packets.
    // could not find a better method :(
    // basically we want to add a hole that we will not actually send, and
    // regard it as already-sent
    if (_reliable || _output != stream)
        assert (false && "Fenrir: This hack should not be used this way!");

    std::lock_guard<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);

    if (_raw.size() == 0 || track_has(std::get<Track> (_raw[0]), Track::START)){
        // Rationale:
        // 2 posibilities:
        //   - queue was empty. So it was fine to just advance the window
        //   - queue had something. that something had not been sent yet,
        //     since we are in a unreliable-unordered mode. Which means that we
        //     can pretend that this message had a lower counter and just send
        //     it. We can do that because the unreliable streams do not have
        //     ACKs and the window is advanced as soon as send_data(..)
        auto saved = _window_start;
        _window_start += Counter{size};
        return saved;
    } else {
        // the previous message was not delivered completely. we can not
        // do the previous trick with the counter. we need to enqueue this :(
        // do not enqueue the data, just the "hole" the data will form.
        uint32_t idx;
        for (idx = 0; idx < _raw.size(); ++idx) {
            if (track_has (std::get<Track> (_raw[idx]), Track::END))
                break;
        }
        assert (track_has (std::get<Track> (_raw[idx]), Track::END) &&
                                        "Fenrir: reserve_data has no end :(");
        ++idx;

        const Counter hole_start = _window_size + Counter{idx} + Counter{_hole};
        _hole += size;
        return hole_start;
    }
}

////////////
// SEND_DATA
////////////

std::tuple<Stream::Fragment, Counter, std::vector<uint8_t>>
                                                        Storage_Raw::send_data (
                                                        const Stream_ID stream,
                                                        const uint32_t max_data)
{
    std::lock_guard<std::mutex> lock(_mtx);
    FENRIR_UNUSED (lock);
    FENRIR_UNUSED (stream);

    uint32_t length = 0;
    auto idx = _raw_idx_start;
    Stream::Fragment flag = Stream::Fragment::MIDDLE;
    if (track_has (std::get<Track> (_raw[idx]), Track::START))
        flag = Stream::Fragment::START;
    while (_raw[idx].second != Track::UNKNOWN && length < max_data) {
        ++length;
        ++idx;
        idx %= static_cast<uint32_t> (_window_size);
        if (track_has (std::get<Track> (_raw[idx]), Track::END)) {
            flag |= Stream::Fragment::END;
            break;
        }
    }
    if (length == 0)
        return std::make_tuple (flag, _window_start, std::vector<uint8_t> ());

    std::vector<uint8_t> ret (length, 0);
    for (uint32_t ret_idx = 0; ret_idx < length; ++ret_idx) {
        const uint32_t raw_idx = (_raw_idx_start + ret_idx) %
                                        static_cast<uint32_t> (_window_size);
        ret[ret_idx] = _raw[raw_idx].first;
    }
    const Counter msg_start = _window_start;
    if (!_reliable) {
        _window_start += Counter{length};
        if (fragment_has (flag, Stream::Fragment::END)) {
            _window_start += Counter{_hole};
            _hole = 0; // see reserve_data()
        }
    }
    return std::make_tuple (flag, msg_start, std::move(ret));
}

std::pair<Counter, Storage::Pairs> Storage_Raw::send_ack (
                                                        const Stream_ID stream)
{

}

std::pair<Counter, Storage::Pairs> Storage_Raw::send_nack (
                                                        const Stream_ID stream)
{

}

std::tuple<Counter, Stream::Fragment, std::vector<uint8_t>>
                            Storage_Raw::get_user_data (const Stream_ID stream)
{
    std::lock_guard<std::mutex> lock(_mtx);
    FENRIR_UNUSED (lock);

    // make sure we are not requested some other stream
    const auto stream_it = std::lower_bound (_streams.begin(), _streams.end(),
            _output, [] (const auto el, const Stream_ID st)
                { return static_cast<bool> (std::get<Stream_ID> (el) < st);});

    if (stream_it == _streams.end()) {
        assert (false && "Fenrir: Storage_Raw should have output stream");
        return {Counter {0}, Stream::Fragment::FULL, std::vector<uint8_t>()};
    }
    if (_output != stream) {
        // with this implementation only one stream gets to report data to the
        // user
        return {Counter {0}, Stream::Fragment::FULL, std::vector<uint8_t>()};
    }

    const auto stream_type = std::get<Storage_t> (*stream_it);

    if (storage_t_has (stream_type, Storage_t::ORDERED)) {
        // both complete and incomplete
        // FIXME: only "reliable" for now?

        if (!track_has (std::get<Track> (_raw[_raw_idx_start]), Track::START)) {
            return {Counter(0), Stream::Fragment::FULL, std::vector<uint8_t>()};
        }
        if (std::get<Track> (_raw[_raw_idx_start]) == Track::FULL) {
            // only one byte? skip some stuff, return early.
            std::get<Track> (_raw[_raw_idx_start]) = Track::UNKNOWN;
            const auto ret_idx = _raw_idx_start;
            ++_window_start;
            _window_start %= Counter (pow (2, 30));
            ++_raw_idx_start;
            _raw_idx_start %= static_cast<uint32_t> (_window_start);
            return {_window_start, Stream::Fragment::FULL, std::vector<uint8_t>(
                                            std::get<uint8_t> (_raw[ret_idx]))};
        }

        Stream::Fragment return_flag {Stream::Fragment::START};
        Counter length {1};
        auto idx = _raw_idx_start;
        idx %= static_cast<uint32_t> (_window_size);
        while (length < _window_size) {
            if (std::get<Track> (_raw[_raw_idx_start]) == Track::END) {
                return_flag = Stream::Fragment::FULL;
                break;
            }
            if (std::get<Track> (_raw[_raw_idx_start]) != Track::MIDDLE) {
                if (storage_t_has (stream_type, Storage_t::INCOMPLETE))
                    break;
                // else Storage_t::COMPLETE
                return {Counter{0}, Stream::Fragment::FULL,
                                                        std::vector<uint8_t>()};
            }
            ++idx;
            idx %= static_cast<uint32_t> (_window_size);
            ++length;
        }
        // if Storaget::Complete, length == _window_size,
        //     but without Fragment::END
        // we still report it, since we need to flush buffers in order to get
        // more data.

        auto ret = std::make_tuple (_window_start, return_flag,
                    std::vector<uint8_t> (static_cast<uint32_t>(length), 0x00));
        _window_start += length;
        _window_start %= Counter (pow (2, 30));
        while (idx != _raw_idx_start) {
            --length;
            std::get<std::vector<uint8_t>>
                                        (ret)[static_cast<uint32_t> (length)] =
                                                std::get<uint8_t> (_raw[idx]);
            _raw[idx] = {0x00, Track::UNKNOWN};
            --idx;
            if (idx > static_cast<uint32_t> (max_counter))
                idx = static_cast<uint32_t> (_window_size) - 1;
        }
        return ret;
    }


    //
    // At this point: storage_t_has (stream_type, Storage_t::UNRDERED)
    //

    // scan the whole receiving window. slow, but whatever.
    Stream::Fragment returned_flag {Stream::Fragment::MIDDLE};
    Counter window_used {1};
    uint32_t msg_start = _raw_idx_start;
    uint32_t msg_end;
    uint32_t length;
    if (storage_t_has (stream_type, Storage_t::COMPLETE)) {
        // search for a complete message.
        // FIXME: goto. Yes, goto are bad. But it was quicker.
        // refactor and take the goto away
    start_search:
        while (!track_has (std::get<Track>(_raw[msg_start]), Track::START)) {
            ++msg_start;
            msg_start %= static_cast<uint32_t> (_window_size);
            ++window_used;
            if (window_used == _window_size) {
                return {Counter{0}, Stream::Fragment::FULL,
                                                    std::vector<uint8_t> ()};
            }
        }
        returned_flag = Stream::Fragment::START;
        msg_end = msg_start;
        length = 1;
        // Get message end
        while (!track_has (std::get<Track>(_raw[msg_end]), Track::END)) {
            if (std::get<Track>(_raw[msg_end]) != Track::MIDDLE) {
                // !END, !MIDDLE and !START (or we would have not commited it
                // in the first place) => no complete message. Go for the next.
                // Start searching again.
                msg_start = msg_end;
                goto start_search;
            }
            ++msg_end;
            msg_start %= static_cast<uint32_t> (_window_size);
            ++length;
            ++window_used;
            if (window_used == _window_size) {
                if (msg_start == _raw_idx_start) {
                    // also: length == _window_size
                    // we filled our window, but we didn't find the end of
                    // the massage. Report to the user anyway.
                    break;
                }
                return {Counter{0}, Stream::Fragment::FULL,
                                                    std::vector<uint8_t> ()};
            }
        }
    } else {
        // storage_t_has (stream_type, Storage_t::INCOMPLETE)
        // Get message beginning
        while (!((std::get<Track>(_raw[msg_start]) == Track::START) ||
                   (std::get<Track>(_raw[msg_start]) == Track::MIDDLE) ||
                   (std::get<Track>(_raw[msg_start]) == Track::END))) {
            ++msg_start;
            msg_start %= static_cast<uint32_t> (_window_size);
            ++window_used;
            if (window_used == _window_size) {
                return {Counter{0}, Stream::Fragment::FULL,
                                                    std::vector<uint8_t> ()};
            }
        }
        returned_flag = static_cast<Stream::Fragment> (
                                            std::get<Track>(_raw[msg_start]));
        msg_end = msg_start;
        length = 1;
        // Get message end
        while (!track_has (std::get<Track>(_raw[msg_end]), Track::END)) {
            if (!track_has (std::get<Track>(_raw[msg_end]), Track::START) &&
                            // aready !has( END)
                            std::get<Track>(_raw[msg_end]) != Track::MIDDLE) {
                break;
            }
            ++msg_end;
            msg_start %= static_cast<uint32_t> (_window_size);
            ++length;
            ++window_used;
            if (window_used == _window_size) {
                if (msg_start == _raw_idx_start) {
                    // also: length == _window_size
                    // we filled our window, but we didn't find the end of
                    // the massage. Report to the user anyway.
                    break;
                }
                return {Counter{0}, Stream::Fragment::FULL,
                                                    std::vector<uint8_t> ()};
            }
        }
    }
    if (track_has (std::get<Track>(_raw[msg_end]), Track::END))
        returned_flag &= Stream::Fragment::END;
    auto ret = std::make_tuple (_window_start, returned_flag,
                std::vector<uint8_t> (static_cast<uint32_t>(length), 0x00));
    while (length > 0) {
        --length;
        std::get<std::vector<uint8_t>>
                                    (ret)[static_cast<uint32_t> (length)] =
                                            std::get<uint8_t> (_raw[msg_end]);
        _raw[msg_end] = {0x00, Track::ACKED};
        --msg_end;
        if (msg_end > static_cast<uint32_t> (max_counter))
            msg_end = static_cast<uint32_t> (_window_size) -1;
    }
    // unordered delivery_ this while can span multiple messages
    if (msg_start == _raw_idx_start) {
        while (std::get<Track> (_raw[_raw_idx_start]) == Track::ACKED) {
            ++_raw_idx_start;
            _raw_idx_start %= static_cast<uint32_t> (_window_size);
            ++_window_start;
            _window_start %= Counter (pow (2, 30));
        }
    }
    return ret;

}

} // namespace Impl
} // namespace Fenrir__v1

