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
#include "Fenrir/v1/plugin/Dynamic.hpp"
#include "Fenrir/v1/event/Loop.hpp"
#include "Fenrir/v1/Handler.hpp"
#include "Fenrir/v1/rate/Rate.hpp"
#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <utility>

namespace Fenrir__v1 {
namespace Impl {

class Connection;

namespace Rate {

// RR_RR: Round-Robin --- Round-Robin
// this simple implementation of rate check selects one connection in
// round-robin, and then selects a link in that connection as
// round-robin.
// Does NOT check link or connection priorities.


class FENRIR_LOCAL RR_RR final: public Rate
{
public:
    RR_RR (const std::shared_ptr<Lib> from, Event::Loop *const loop,
                                                        Loader *const loader,
                                                        Random *const rnd,
                                                        Handler *handler)
        : Rate (from, loop, loader, rnd, handler), _last_conn (0) {}

    RR_RR() = delete;
    RR_RR (const RR_RR&) = delete;
    RR_RR& operator= (const RR_RR&) = delete;
    RR_RR (RR_RR &&) = default;
    RR_RR& operator= (RR_RR &&) = default;
    ~RR_RR () {}
    void parse_event (std::shared_ptr<Event::Plugin_Timer> ev) override
        { FENRIR_UNUSED (ev); }

    Link_Params def_link_params() override
        { return Link_Params {{1500, 4500}}; }

    type_safe::optional<send_info> send (
                            std::unique_ptr<Event::Send::Data> data) override;

    void enqueue (const Link_ID from, const Link_ID to,
                std::unique_ptr<Packet> pkt,
                    const type_safe::optional<Conn0_Type> handshake) override;
private:
    enum class Send_Type : uint8_t { DATA = 0x01, HANDSHAKE = 0x00 };
    class FENRIR_LOCAL data_base : public Event::Send::Data
    {
    public:
        data_base (const Send_Type t) : _type (t) {}
        const Send_Type _type;
    };

    class FENRIR_LOCAL data_handshake final : public data_base
    {
    public:
        data_handshake() = delete;
        data_handshake (const Link_ID from, const Link_ID to,
                                                std::unique_ptr<Packet> pkt)
            : data_base (Send_Type::HANDSHAKE), _from (from), _to (to),
                _pkt (std::move(pkt)) {}
        Link_ID _from;
        Link_ID _to;
        std::unique_ptr<Packet> _pkt;
    };

    struct speed
    {
        uint64_t _max_bps;          // max user-set bitrate
        uint64_t _max_congestion;   // max connection-discovered bitrate
        uint64_t _last_amount;
        std::chrono::microseconds _last_time;
    };

    struct info
    {
        Conn_ID _conn_id;
        speed   _conn_speed;
        Link_ID _last_link_to;
        Link_ID _last_link_from;
        std::vector<std::pair<Link_ID, speed>> _links_to;
        std::vector<std::pair<Link_ID, speed>> _links_from;
    };

    Conn_ID _last_conn;
    // TODO: better if it is map? this will cause lots of moves when
    // the first connections die out :( deque, better?
    std::vector<info> _conninfo;
    std::mutex _mtx;

    type_safe::optional<send_info> send_data();
    type_safe::optional<send_info> send_handshake (
                                    const std::unique_ptr<data_handshake> data);
};

FENRIR_INLINE type_safe::optional<Rate::send_info> RR_RR::send (
                                        std::unique_ptr<Event::Send::Data> data)
{
    const auto test = reinterpret_cast<data_base*> (data.get());
    if (test->_type == Send_Type::DATA)
        return send_data();
    const auto hshake = reinterpret_cast<class data_handshake*>(data.release());
    return send_handshake (std::unique_ptr<data_handshake> (hshake));
}

FENRIR_INLINE type_safe::optional<Rate::send_info> RR_RR::send_handshake (
                                    const std::unique_ptr<data_handshake> data)
{
    const auto sock_from = get_socket (data->_from);
    return type_safe::make_optional (
                    send_info {sock_from, data->_to, std::move(data->_pkt)});
}

FENRIR_INLINE type_safe::optional<Rate::send_info> RR_RR::send_data()
{
    std::unique_lock<std::mutex> lock (_mtx);

    // select the connections via round-robin:
    // keep the selected ID in "last_conn"

    auto conn_it = std::upper_bound (_conninfo.begin(), _conninfo.end(),
                    _last_conn,
                        // operator<, => true on exit
                        [] (const Conn_ID id, const auto &el)
                            { return static_cast<bool> (id < el._conn_id); });
    if (conn_it == _conninfo.end())
        conn_it = _conninfo.begin(); // go back to beginning;
    if (conn_it == _conninfo.end()) {
        // no such connection id? don't sent anything anywhere.
        return type_safe::nullopt;
    }
    auto next_conn_id = conn_it->_conn_id;
    auto conn = _handler->get_connection (next_conn_id);

    // make sure we are not over the connection bandwidth
    auto time = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto usec = std::chrono::duration_cast<std::chrono::microseconds> (time);

    const uint64_t bps =  conn_it->_conn_speed._max_congestion /
        static_cast<uint64_t>((usec - conn_it->_conn_speed._last_time).count());
    if (bps > conn_it->_conn_speed._max_congestion) {
        _last_conn = next_conn_id;
        return send_data();
    }

    // get a Link FROM which we send
    // also, check that we do not go over the bandwidth limit.
    // in that case, next link, or next connection after a full round.
    auto next_from_it = std::upper_bound (conn_it->_links_from.begin(),
                        conn_it->_links_from.end(), conn_it->_last_link_from,
                [] (const Link_ID id, const auto &lnk) {
                    return static_cast<bool> (id < std::get<Link_ID> (lnk));
                });
    if (next_from_it == conn_it->_links_from.end()) {
        next_from_it = conn_it->_links_from.begin();
        if (next_from_it == conn_it->_links_from.end()) {
            _last_conn = next_conn_id;
            return send_data();
        }
    }
    const Link_ID next_link_from_id = std::get<Link_ID> (*next_from_it);
    const auto next_link_from = get_socket (next_link_from_id);

    // get a Link TO which we send
    uint32_t link_count = static_cast<uint32_t> (conn_it->_links_to.size());
    auto next_to_it = std::upper_bound (conn_it->_links_to.begin(),
                            conn_it->_links_to.end(), conn_it->_last_link_to,
                [] (const Link_ID id, const auto &lnk) {
                    return static_cast<bool> (id < std::get<Link_ID> (lnk));
                });
    if (next_to_it == conn_it->_links_to.end()) {
        next_to_it = conn_it->_links_to.begin();
        if (next_to_it == conn_it->_links_to.end()) {
            _last_conn = next_conn_id;
            return send_data();
        }
    }
    Link_ID next_link_to;
    while (link_count > 0) {
        next_link_to = std::get<Link_ID> (*next_to_it);

        const uint64_t link_bps =  conn_it->_conn_speed._max_congestion /
                        static_cast<uint64_t> (
                            (usec - conn_it->_conn_speed._last_time).count());
        if (link_bps <= std::get<speed> (*next_to_it)._max_congestion)
            break;
        ++next_to_it;
        if (next_to_it == conn_it->_links_to.end())
            next_to_it = conn_it->_links_to.begin();
        --link_count;
    }
    if (link_count == 0) {
        // all links over congestion.
        _last_conn = next_conn_id;
        return send_data();
    }
    lock.unlock();

    // now get a packet with the data for one stream:
    uint32_t data_mtu;
    uint32_t overhead;
    std::tie (data_mtu, overhead) = get_mtu (conn, next_link_from_id,
                                                                next_link_to);
    auto pkt = std::make_unique<Packet> (std::vector<uint8_t> (data_mtu, 0));
    if (pkt == nullptr)
        return type_safe::nullopt;

    data_mtu -= overhead;

    // add streams 'til full
    if (set_packet (conn, Packet_NN {pkt.get()}, next_link_to, data_mtu) !=
                                                            Impl::Error::NONE) {
        return type_safe::nullopt;
    }


    conn_it->_last_link_from = next_link_from_id;
    conn_it->_last_link_to   = next_link_to;
    conn_it->_conn_speed._last_time = usec;
    conn_it->_conn_speed._last_amount = pkt->raw.size() - overhead;
    next_from_it->second._last_time = usec;
    next_from_it->second._last_amount = pkt->raw.size() - overhead;
    next_to_it->second._last_time = usec;
    next_to_it->second._last_amount = pkt->raw.size() - overhead;

    // send
    return type_safe::make_optional (
                    send_info {next_link_from, next_link_to, std::move(pkt)});
}

FENRIR_INLINE void RR_RR::enqueue (const Link_ID from, const Link_ID to,
                            std::unique_ptr<Packet> pkt,
                                const type_safe::optional<Conn0_Type> handshake)
{
    std::unique_lock<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);
    FENRIR_UNUSED (handshake); // just for tacking purposes if you want

    auto ev = Event::Send::mk_shared (_loop,
                    std::make_unique<data_handshake>(from, to, std::move(pkt)));
    _loop->add_work (std::move(ev));
}

} // namespace Rate
} // namespace Impl
} // namespace Fenrir__v1
