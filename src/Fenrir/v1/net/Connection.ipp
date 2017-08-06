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
#include "Fenrir/v1/data/Storage_Raw.hpp"
#include "Fenrir/v1/data/control/Control.ipp"
#include "Fenrir/v1/event/Events_all.hpp"
#include "Fenrir/v1/event/Loop.hpp"
#include "Fenrir/v1/Handler.hpp"
#include "Fenrir/v1/net/Connection.hpp"
#include "Fenrir/v1/net/Link.hpp"
#include "Fenrir/v1/util/math.hpp"
#include <algorithm>
#include <limits>

namespace Fenrir__v1 {
namespace Impl {

FENRIR_INLINE Connection::Stream_Track_In::Stream_Track_In (const Stream_ID id,
                                                    const Storage_t s,
                                                    const Counter window_start,
                                                    std::shared_ptr<Storage>str)
{
    if (str == nullptr) {
        _received = std::make_unique<Storage_Raw> ();
    } else {
        _received = std::move (str);
    }
    _received->add_stream (id, s, window_start, Counter {15000},
                                                            Storage::IO::INPUT);
}

FENRIR_INLINE Connection::Stream_Track_Out::Stream_Track_Out(const Stream_ID id,
                                                    const Storage_t s,
                                                    Random *rnd,
                                                    std::shared_ptr<Storage>str)
    : _bytes_sent (0)
{
    const Counter window_start {rnd->uniform<uint32_t> (0,
                                        static_cast<uint32_t> (max_counter))};
    if (str == nullptr) {
        _sent = std::make_unique<Storage_Raw> ();
    } else {
        _sent = std::move (str);
    }
    _sent->add_stream (id, s, window_start, Counter {15000},
                                                        Storage::IO::OUTPUT);
}

FENRIR_INLINE Connection::Connection (const Role role, const User_ID user,
                                        Event::Loop *const loop,
                                        Handler *const handler,
                                        const Stream_ID read_control_stream,
                                        const Stream_ID write_control_stream,
                                        const Conn_ID read, const Conn_ID write,
                                        const Counter control_window_start,
                                        const Packet::Alignment_Byte read_al,
                                        const Packet::Alignment_Byte write_al,
                                        const uint8_t max_read_padding,
                                        const uint8_t max_write_padding)
    : _read_connection_id (read), _write_connection_id (write),
                            _user_id (user),
                            _rel_read_control_stream (read_control_stream),
                            _unrel_read_control_stream (
                                Stream_ID { // casting to avoid narrowing warn
                                static_cast<uint16_t> (1 +
                                static_cast<uint16_t> (read_control_stream))}),
                            _rel_write_control_stream (write_control_stream),
                            _unrel_write_control_stream (
                                Stream_ID { // casting to avoid narrowing warn
                                static_cast<uint16_t> (1 +
                                static_cast<uint16_t> (write_control_stream))}),
                            _read_al (read_al), _write_al (write_al),
                            _max_read_padding (max_read_padding),
                            _max_write_padding (max_write_padding),
                            _role (role), _loop (loop), _handler (handler)
{
    _max_write_padding = 8;
    auto rel_st_in = std::make_shared<Storage_Raw> ();
    auto unrel_st_in = std::make_shared<Storage_Raw> ();
    auto rel_st_out = std::make_shared<Storage_Raw> ();
    auto unrel_st_out = std::make_shared<Storage_Raw> ();
    rel_st_in->add_stream (_rel_read_control_stream,
                                                        Storage_t::RELIABLE |
                                                        Storage_t::ORDERED  |
                                                        Storage_t::COMPLETE,
                                                        control_window_start,
                                                        Counter {15000},
                                                        Storage::IO::INPUT);
    unrel_st_in->add_stream (_unrel_read_control_stream,
                                                        Storage_t::UNRELIABLE |
                                                        Storage_t::UNORDERED  |
                                                        Storage_t::COMPLETE,
                                                        control_window_start,
                                                        Counter {15000},
                                                        Storage::IO::INPUT);
    rel_st_out->add_stream (_rel_write_control_stream,
                                                        Storage_t::RELIABLE |
                                                        Storage_t::ORDERED  |
                                                        Storage_t::COMPLETE,
                                                        control_window_start,
                                                        Counter {15000},
                                                        Storage::IO::OUTPUT);
    unrel_st_out->add_stream (_unrel_write_control_stream,
                                                        Storage_t::UNRELIABLE |
                                                        Storage_t::UNORDERED  |
                                                        Storage_t::COMPLETE,
                                                        control_window_start,
                                                        Counter {15000},
                                                        Storage::IO::OUTPUT);
    // reliable control stream
    _streams_in.emplace_back (std::piecewise_construct,
                            std::forward_as_tuple  (_rel_read_control_stream),
                            std::forward_as_tuple  (_rel_read_control_stream,
                                                        Storage_t::RELIABLE |
                                                        Storage_t::ORDERED  |
                                                        Storage_t::COMPLETE,
                                                        control_window_start,
                                                        std::move (rel_st_in)));
    // unreliable control stream
    _streams_in.emplace_back (std::piecewise_construct,
                            std::forward_as_tuple  (_unrel_read_control_stream),
                            std::forward_as_tuple  (_unrel_read_control_stream,
                                                    Storage_t::UNRELIABLE |
                                                    Storage_t::UNORDERED  |
                                                    Storage_t::COMPLETE,
                                                    control_window_start,
                                                    std::move (unrel_st_in)));
    // reliable control stream
    _streams_out.emplace_back (std::piecewise_construct,
                            std::forward_as_tuple  (_rel_write_control_stream),
                            std::forward_as_tuple  (_rel_write_control_stream,
                                                    Storage_t::RELIABLE |
                                                    Storage_t::ORDERED  |
                                                    Storage_t::COMPLETE,
                                                    &_rnd,
                                                    std::move (rel_st_out)));
    // unreliable control stream
    _streams_out.emplace_back (std::piecewise_construct,
                            std::forward_as_tuple (_unrel_write_control_stream),
                            std::forward_as_tuple (_unrel_write_control_stream,
                                                    Storage_t::RELIABLE |
                                                    Storage_t::ORDERED  |
                                                    Storage_t::COMPLETE,
                                                    &_rnd,
                                                    std::move (unrel_st_out)));
}

FENRIR_INLINE std::pair<Impl::Error, Stream_ID> Connection::add_stream_out (
            const Storage_t s, const type_safe::optional<Stream_ID> linked_with)
{
    std::unique_lock<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);

    if (_streams_out.size() == (pow (2, 16) - 1))
        return {Impl::Error::FULL, Stream_ID{0}};

    // search strea to link with
    std::shared_ptr<Storage> str;
    if (linked_with.has_value()) {
        const Stream_ID link = linked_with.value();
        auto res = std::lower_bound (_streams_out.begin(), _streams_out.end(),
                            link,
                            [] (const auto &it, const Stream_ID id)
                                { return std::get<Stream_ID> (it) < id; });
        if (res == _streams_out.end())
            return {Impl::Error::WRONG_INPUT, Stream_ID {0}};
        str = std::get<Stream_Track_Out> (*res)._sent;
    }

    // add stream. keep them ordered.
    Stream_ID id;
    while (true) {
        id = static_cast<Stream_ID> (_rnd.uniform<uint16_t>());
        auto res = std::lower_bound (_streams_out.begin(), _streams_out.end(),
                        id, [] (const auto &it, const Stream_ID tmp_id)
                                { return std::get<Stream_ID> (it) < tmp_id; });
        if (res != _streams_out.end() && std::get<Stream_ID> (*res) == id)
            continue;
        _streams_out.emplace (res, std::piecewise_construct,
                                        std::forward_as_tuple (id),
                                        std::forward_as_tuple (id, s, &_rnd,
                                                            std::move (str)));
        break;
    }
    return {Impl::Error::NONE, id};
}

FENRIR_INLINE Impl::Error Connection::add_stream_in (const Stream_ID id,
                            const Storage_t s, const Counter window_start,
                            const type_safe::optional<Stream_ID> linked_with)
{
    std::unique_lock<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);
    if (_streams_in.size() == (pow (2, 16) - 1))
        return Impl::Error::FULL;

    // search stream to link with
    std::shared_ptr<Storage> str;
    if (linked_with.has_value()) {
        const Stream_ID link = linked_with.value();
        auto res = std::lower_bound (_streams_out.begin(), _streams_out.end(),
                            link,
                            [] (const auto &it, const Stream_ID lnk)
                                { return std::get<Stream_ID> (it) < lnk; });
        if (res == _streams_out.end())
            return Impl::Error::WRONG_INPUT;
        str = std::get<Stream_Track_Out> (*res)._sent;
    }

    // add stream, keep them ordered
    auto res = std::lower_bound (_streams_in.begin(), _streams_in.end(),
                        id, [] (const auto &it, const Stream_ID tmp_id)
                                { return std::get<Stream_ID> (it) == tmp_id; });
    if (res != _streams_in.end())
        return Impl::Error::ALREADY_PRESENT;
    _streams_in.emplace (res, std::piecewise_construct,
                                        std::forward_as_tuple (id),
                                        std::forward_as_tuple (id, s,
                                                window_start, std::move (str)));
    return Impl::Error::NONE;
}

FENRIR_INLINE Impl::Error Connection::del_stream_out (const Stream_ID id)
{
    std::unique_lock<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);

    auto res = std::lower_bound (_streams_out.begin(), _streams_out.end(),
                            id, [] (const auto &it, const Stream_ID tmp_id)
                                { return std::get<Stream_ID> (it) == tmp_id; });
    if (res == _streams_out.end() || std::get<Stream_ID> (*res) != id)
        return Impl::Error::WRONG_INPUT;
    std::get<Stream_Track_Out> (*res)._sent->del_stream (id,
                                                        Storage::IO::OUTPUT);
    _streams_out.erase (res);
    return Error::NONE;
}

FENRIR_INLINE Impl::Error Connection::del_stream_in (const Stream_ID id)
{
    std::unique_lock<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);

    auto res = std::lower_bound (_streams_in.begin(), _streams_in.end(),
                            id, [] (const auto &it, const Stream_ID tmp_id)
                                { return std::get<Stream_ID> (it) == tmp_id; });
    if (res == _streams_in.end() || std::get<Stream_ID> (*res) != id)
        return Impl::Error::WRONG_INPUT;
    std::get<Stream_Track_In> (*res)._received->del_stream (id,
                                                        Storage::IO::INPUT);
    _streams_in.erase (res);
    return Error::NONE;
}

FENRIR_INLINE void Connection::recv (Packet &pkt)
{
    gsl::span<uint8_t> raw_pkt;
    if (_ecc->correct (pkt.data_no_id(), raw_pkt) == Recover::ECC::Result::ERR){
        return;
    }
    if (!_hmac_recv->is_valid (raw_pkt, raw_pkt))
        return;
    if (_enc_recv->decrypt (raw_pkt, raw_pkt) != Error::NONE)
        return;
    if (pkt.parse (raw_pkt, _read_al) != Error::NONE) {
        // Error::WRONG_INPUT;
        return;
    }
    std::unique_lock<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);

    for (const auto &stream : pkt.stream) {
        auto in = std::lower_bound (_streams_in.begin(), _streams_in.end(),
                        stream.id(), [] (const auto &it, const Stream_ID tmp_id)
                                { return std::get<Stream_ID> (it) == tmp_id; });
        if (in == _streams_in.end() ||
                                    std::get<Stream_ID> (*in) != stream.id()) {
            continue; // ignore unknown streams
        }
        std::get<Stream_Track_In> (*in)._received->recv_data (stream.id(),
                                                            stream.counter(),
                                                            stream.data(),
                                                            stream.type());
        if (stream.id() == _rel_read_control_stream) {
            parse_rel_control();
        } else if (stream.id() == _rel_read_control_stream) {
            parse_unrel_control();
        }
    }

}

FENRIR_INLINE std::vector<user_data> Connection::get_data()
{
    std::unique_lock<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);

    // get all possible data from all possible streams.
    std::vector<user_data> ret;
    for (auto &stream_it : _streams_in) {
        user_data::data data;
        std::vector<user_data::data> ret_stream;
        do {
            auto &stream = std::get<Stream_Track_In> (stream_it);
            data = stream._received->get_user_data (
                                                std::get<Stream_ID>(stream_it));
            if (std::get<std::vector<uint8_t>> (data).size() > 0)
                ret_stream.emplace_back (std::move(data));
        } while (std::get<std::vector<uint8_t>> (data).size() != 0);
        if (ret_stream.size() > 0)
            ret.emplace_back (std::get<Stream_ID> (stream_it),
                                                        std::move (ret_stream));
    }
    return ret;
}

FENRIR_INLINE std::unique_ptr<Packet> Connection::update_source (
                                                            const Link_ID from)
{
    std::lock_guard<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);

    // last link should have more proability of being used. scan from last.
    auto link = std::find_if (_incoming.rbegin(), _incoming.rend(),
                                            [from] (const Link &l)
                                                { return from == l._link; });
    if (link != _incoming.rend()) {
        link->keepalive (_loop);
        link->_keepalive_failed = 0;
        // FIXME: check if this link still has to be activated. then re-send
        // activation pkt. But not at every packet. ... WHEN?
        // right now if the activation pkt is missing
        return nullptr;
    }
    auto keepal = Event::Keepalive::mk_shared (_loop, _ourselves, from,
                                                        Direction::INCOMING);
    _loop->start (keepal, Link::keepalive_timeout, Event::Repeat::YES);

    _incoming.emplace_back (from, &_rnd, keepal);

    // build the activation packet.
    const uint16_t msg_size = static_cast<uint16_t> (
                                    Control::Link_Activation_Srv<>::min_size() +
                                        _incoming.rbegin()->_activation.size());
    const size_t total_size = PKT_MINLEN + msg_size + total_overhead();
    auto activation_pkt = std::make_unique<Packet> (
                                        std::vector<uint8_t> (total_size, 0));
    activation_pkt->set_header (_write_connection_id,
                            static_cast<uint8_t> (_enc_send->bytes_header() +
                                                  _enc_send->bytes_header()),
                                                                        &_rnd);
    // use reserve_data (size) on the unrel_control_stream
    // TODO: we should be able to do this stateless. easy enough:
    // LINK_ID + timestamp encrypted/authenticated. (over the existing
    // connection)
    // TODO: also: choose between "manual" and automatic confirmation
    //   meaning: automatic sends an activation for each non-active link,
    //   and manual requires you to request the activaton pkt.
    const Stream_ID unrel_str = _unrel_write_control_stream;
    auto unrel_stream = std::find_if (_streams_out.begin(), _streams_out.end(),
                        [unrel_str] (const auto &st)
                            { return unrel_str == std::get<Stream_ID> (st); });

    const Counter ctr = std::get<Stream_Track_Out>(*unrel_stream)._sent->
                                            reserve_data (unrel_str, msg_size);
    auto msg = activation_pkt->add_stream (unrel_str, Stream::Fragment::FULL,
                                                                ctr, msg_size);
    Control::Link_Activation_Srv<Control::Access::READ_WRITE> activate_msg (
                                                            msg->data(), from);
    std::copy (activate_msg._activation.begin(),
                                    activate_msg._activation.end(),
                                    _incoming.rbegin()->_activation.begin());

    activation_pkt->set_header (_write_connection_id, 0, &_rnd);
    if (_enc_send->encrypt (activation_pkt->raw) != Error::NONE)
        return nullptr;
    if (_hmac_send->add_hmac (activation_pkt->raw) != Error::NONE)
        return nullptr;
    if (_ecc->add_ecc (activation_pkt->raw) != Error::NONE)
        return nullptr;
    return activation_pkt;
}

FENRIR_INLINE void Connection::update_destination (const Link_ID to)
{
    std::lock_guard<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);

    // last link should have more proability of being used. scan from last.
    auto link = std::find_if (_outgoing.rbegin(), _outgoing.rend(),
                                            [to] (const Link &l)
                                                { return to == l._link; });
    if (link != _incoming.rend()) {
        link->keepalive (_loop);
        link->_keepalive_failed = 0;
        return;
    }
    assert (false && "Fenrir: send to non-registered out link");
}

FENRIR_INLINE Error Connection::add_Link_in (const Link_ID id)
{
    auto keepal = Event::Keepalive::mk_shared (_loop, _ourselves, id,
                                                        Direction::INCOMING);
    auto def_param = _handler->proxy_def_link_params();
    std::lock_guard<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);

    _loop->start (keepal, Link::keepalive_timeout, Event::Repeat::YES);

    _incoming.emplace_back (id, keepal, def_param.mtu(),
                                                    def_param.init_window());

    return Error::NONE;
}

FENRIR_INLINE Error Connection::add_Link_out (const Link_ID id)
{
    auto keepal = Event::Keepalive::mk_shared (_loop, _ourselves, id,
                                                        Direction::INCOMING);
    auto def_param = _handler->proxy_def_link_params();
    std::lock_guard<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);

    _loop->start (keepal, Link::keepalive_timeout, Event::Repeat::YES);

    _outgoing.emplace_back (id, keepal, def_param.mtu(),
                                                    def_param.init_window());
    return Error::NONE;

}

FENRIR_INLINE void Connection::missed_keepalive_in (const Link_ID id,
                                                        const uint8_t max_fail)
{
    std::lock_guard<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);

    auto link = std::find_if (_incoming.begin(), _incoming.end(),
                                            [id] (const Link &l)
                                                { return id == l._link; });
    if (link == _incoming.end())
        return;

    link->keepalive (_loop);
    ++link->_keepalive_failed;
    // if this link still has to be re-activated, do NOT re-send the activation
    // sending the activation multiple times without receiving will mean
    // breaking the "no amplification" rule
    if (link->_keepalive_failed < max_fail)
        return;
    // we got over the max keepalive failed. drop this Link, it is not active
    // anymore.
    _incoming.erase (link);

    // FIXME : when do we delete the connection?
    // should we put a timeout on activating the links?
    // ...we need somewhere to send our data!
    // timeout for no links? <<- nice
}


FENRIR_INLINE type_safe::optional<std::tuple<Link_ID,
                    std::unique_ptr<Packet>>> Connection::missed_keepalive_out (
                                                            const Link_ID to)
{
    Link_ID dest;
    {
        std::lock_guard<std::mutex> lock (_mtx);
        FENRIR_UNUSED (lock);
        if (_incoming.size() == 0)
            return type_safe::nullopt; // nowhere to send the keepalive

        // last link should have more proability of being used. scan from last.
        auto link = std::find_if (_outgoing.rbegin(), _outgoing.rend(),
                                                [to] (const Link &l)
                                                    { return to == l._link; });
        if (link == _outgoing.rend())
            return type_safe::nullopt;

        link->keepalive (_loop);

        const uint16_t incoming_size = static_cast<const uint16_t> (
                                                            _incoming.size());
        dest = _incoming[_rnd.uniform<uint16_t> (0, incoming_size)]._link;
    }
    // send a keepalive from the specified link.
    uint8_t padding = _rnd.uniform<uint8_t>();
    const size_t total_size = PKT_MINLEN + padding + total_overhead();
    auto keepalive_pkt = std::make_unique<Packet> (
                                        std::vector<uint8_t> (total_size, 0));
    keepalive_pkt->set_header (_write_connection_id, padding, &_rnd);
    if (_enc_send->encrypt (keepalive_pkt->raw) != Error::NONE)
        return type_safe::nullopt;
    if (_hmac_send->add_hmac (keepalive_pkt->raw) != Error::NONE)
        return type_safe::nullopt;
    //return {dest, keepalive_pkt};
    return {std::make_tuple (dest, std::move(keepalive_pkt))};
}

FENRIR_INLINE uint32_t Connection::total_overhead() const
{
    return _enc_send->bytes_overhead() + _hmac_send->bytes_overhead() +
                                                        _ecc->get_overhead();
}

FENRIR_INLINE uint32_t Connection::mtu (const Link_ID from, const Link_ID to)
                                                                        const
{
    auto from_it = std::lower_bound (_incoming.begin(), _incoming.end(), from,
                                        [] (const auto &link, const Link_ID id)
                                            { return link._link < id; });
    if (from_it->_link != from)
        return 0;
    auto to_it = std::lower_bound (_outgoing.begin(), _outgoing.end(), to,
                                        [] (const auto &link, const Link_ID id)
                                            { return link._link < id; });
    if (to_it->_link != from)
        return 0;
    return std::min (from_it->_mtu, to_it->_mtu);
}

FENRIR_INLINE Error Connection::add_data (Packet_NN pkt, const uint32_t mtu)
{
    // Add data to packet. Select streams in Round Robin.
    // TODO: make this a plugin for easier experimentation.

    // set the packet data to start after padding + encryption overhead
    // it's a small hack so that the encryption plugin can
    // work inline it if can.
    // we will reset the correct padding before returning.
    const uint8_t enc_overhead = static_cast<uint8_t> (
                        _enc_send->bytes_header() + _hmac_send->bytes_header());
    const uint8_t pad = _rnd.uniform<uint8_t> (0, _max_write_padding);
    pkt.modify().get()->set_header (_write_connection_id, enc_overhead + pad,
                                                                        &_rnd);

    std::unique_lock<std::mutex> lock (_mtx);
    auto bytes_left = mtu - (_enc_send->bytes_overhead() +
                                                _hmac_send->bytes_overhead() +
                                                        _ecc->get_overhead());
    // get first stream after "_last_out"
    auto it = std::lower_bound (_streams_out.begin(), _streams_out.end(),
                            _last_out,
                            [] (const auto &test, const Stream_ID id)
                                { return std::get<Stream_ID> (test) < id; });
    if (it == _streams_out.end()) {
        it = _streams_out.begin();
        if (it == _streams_out.end())
            return Impl::Error::EMPTY;
    } else if (std::get<Stream_ID> (*it) == _last_out) {
        ++it;
        if (it == _streams_out.end())
            it = _streams_out.begin();
    }

    // send always at least 8 bytes. Arbitrary, but we should try not to
    // fragment too much.
    while (bytes_left > (STREAM_MINLEN + 8) &&
                                    std::get<Stream_ID> (*it) != _last_out) {
        bytes_left -= STREAM_MINLEN;
        // TODO: move this to gsl::span
        auto to_add = std::get<Stream_Track_Out> (*it)._sent->send_data (
                                        std::get<Stream_ID> (*it), bytes_left);
        const uint16_t size = static_cast<const uint16_t> (
                                std::get<std::vector<uint8_t>> (to_add).size());
        if (size > 0) {
            auto out_stream = pkt.modify().get()->add_stream (
                                            std::get<Stream_ID> (*it),
                                            std::get<Stream::Fragment> (to_add),
                                            std::get<Counter> (to_add),
                                            size);
            // TODO: avoid copy, make "send_data" write directly in packet.
            std::copy (out_stream->data().begin(), out_stream->data().end(),
                            std::get<std::vector<uint8_t>> (to_add).begin());
            bytes_left -= size;
            _last_out = std::get<Stream_ID> (*it);
        } else {
            bytes_left += STREAM_MINLEN;
        }
        ++it;
        if (it == _streams_out.end())
            it = _streams_out.begin();
    }
    lock.unlock();
    // set the correct padding
    pkt.modify().get()->set_header (_write_connection_id, pad, &_rnd);
    return Impl::Error::NONE;
}

FENRIR_INLINE Error Connection::add_security (Packet_NN pkt)
{
    uint8_t *start = pkt.modify().get()->raw.data() + sizeof(Conn_ID);
    uint8_t *end = start + pkt.modify().get()->raw.size();
    auto enc_span = gsl::span<uint8_t> (start, end);
    auto err = _enc_send->encrypt (enc_span);
    if (err != Impl::Error::NONE)
        return err;
    gsl::span<uint8_t> span (pkt.modify().get()->raw.data(),
                        static_cast<ssize_t> (pkt.modify().get()->raw.size()));
    err = _hmac_send->add_hmac (span);
    if (err != Impl::Error::NONE)
        return err;
    err = _ecc->add_ecc (pkt.modify().get()->raw);
    if (err != Impl::Error::NONE)
        return err;
    return Error::WRONG_INPUT;
}

} // namespace Impl
} // namespace Fenrir__v1
