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

#include "Fenrir/v1/net/Connection.hpp"
#include "Fenrir/v1/rate/Rate.hpp"
#include "Fenrir/v1/Handler.hpp"

namespace Fenrir__v1 {
namespace Impl {
namespace Rate {

// return pair<mtu, connection overhead>
std::pair<uint32_t, uint32_t> Rate::get_mtu (
                                        const std::shared_ptr<Connection> conn,
                                        const Link_ID link_from,
                                        const Link_ID link_to)
    { return {conn->mtu (link_from, link_to), conn->total_overhead()}; }

Impl::Error Rate::set_packet (std::shared_ptr<Connection> const conn,
                        Packet_NN pkt, const Link_ID dest, const uint32_t mtu)
{
    auto err = conn->add_data (pkt, mtu);
    if (err != Impl::Error::NONE)
        return err;
    conn->update_destination (dest);
    return conn->add_security (pkt);
}

std::shared_ptr<Socket> Rate::get_socket (const Link_ID id)
    { return _handler->get_socket (id); }

} // namespace Rate
} // namespace Impl
} // namespace Fenrir__v1
