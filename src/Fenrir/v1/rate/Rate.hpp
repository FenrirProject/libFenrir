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
#include "Fenrir/v1/data/Conn0_Type.hpp"
#include "Fenrir/v1/event/Events_all.hpp"
#include "Fenrir/v1/plugin/Dynamic.hpp"
#include <type_safe/strong_typedef.hpp>
#include <type_safe/optional.hpp>
#include <map>
#include <memory>
#include <utility>

namespace Fenrir__v1 {
namespace Impl {

class Handler;
class Connection;
class Link;
class Loader;
class Packet;
class Socket;
namespace Event {
class Base;
} // namespace Event

class Handler;

namespace Rate {

// When subclassing please name the class in a way that tells people
// how connections and streams are chosen
// the example is RR-RR: choose a connection in Round-Robin, and then
// choose a link in round-robin.

class FENRIR_LOCAL Rate : public Dynamic
{
public:
    struct FENRIR_LOCAL ID :
                        type_safe::strong_typedef<ID, uint16_t>,
                        type_safe::strong_typedef_op::equality_comparison<ID>,
                        type_safe::strong_typedef_op::relational_comparison<ID>
        { using strong_typedef::strong_typedef; };

    Rate (const std::shared_ptr<Lib> from, Event::Loop *const loop,
                                                        Loader *const loader,
                                                        Random *const rnd,
                                                        Handler *const handler)
        : Dynamic (from, loop, loader, rnd), _handler (handler) {}
    Rate() = delete;
    Rate (const Rate&) = delete;
    Rate& operator= (const Rate&) = delete;
    Rate (Rate &&) = default;
    Rate& operator= (Rate &&) = default;
    virtual ~Rate () {}

    struct send_info {
        std::shared_ptr<Socket> _from;
        Link_ID _to;
        std::unique_ptr<Packet> _pkt;
    };

    virtual Link_Params def_link_params() = 0;

    virtual type_safe::optional<send_info> send (
                                    std::unique_ptr<Event::Send::Data> data)= 0;

    // decide when to send handshake answers
    virtual void enqueue (const Link_ID from, const Link_ID to,
                        std::unique_ptr<Packet> pkt,
                          const type_safe::optional<Conn0_Type> handshake) = 0;

    // FIXME: add the following functions (can change name):
    //   * add socket
    //   * set max socket speed
    //   * del socket
    //   * add connection
    //   * set max conn speed
    //   * del connection
    //   * add link
    //   * set max link speed
    //   * del link
    //   * rate + (??)
    //   * rate - (??)
protected:
    Handler *const _handler;
    // pair<mtu, connection overhead>
    std::pair<uint32_t, uint32_t> get_mtu (
                                        const std::shared_ptr<Connection> conn,
                                        const Link_ID link_from,
                                        const Link_ID link_to);
    Impl::Error set_packet (std::shared_ptr<Connection> const conn,
                        Packet_NN pkt, const Link_ID dest, const uint32_t mtu);
    std::shared_ptr<Socket> get_socket (const Link_ID id);
};

} // namespace Rate
} // namespace Impl
} // namespace Fenrir__v1

