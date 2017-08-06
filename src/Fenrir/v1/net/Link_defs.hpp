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
#include "Fenrir/v1/data/IP.hpp"

namespace Fenrir__v1 {
namespace Impl {


// Strong typedef for User id,to avoid up/down casting, and other
// operations which do not make sense for an identifier.
// FIXME: split into Link_ID_From / Link_ID_To
struct FENRIR_LOCAL Link_ID :
                    type_safe::strong_typedef<Link_ID, std::pair<IP, UDP_Port>>,
                    type_safe::strong_typedef_op::equality_comparison<Link_ID>
{
    using strong_typedef::strong_typedef;

    bool operator< (const Link_ID id) const
    {
        using ip_port = std::pair<IP, UDP_Port>;

        if (std::get<IP> (static_cast<ip_port> (*this)) ==
                                    std::get<IP> (static_cast<ip_port> (id))) {
            return static_cast<bool> (
                            std::get<UDP_Port> (static_cast<ip_port> (*this)) <
                            std::get<UDP_Port> (static_cast<ip_port> (id)));
        }
        return std::get<IP> (static_cast<ip_port> (*this)) <
                                    std::get<IP> (static_cast<ip_port> (id));
    }
    explicit operator bool() const {
        using ip_port = std::pair<IP, UDP_Port>;
        // paranoia mode on.
        return static_cast<bool> (std::get<IP> (static_cast<ip_port> (*this)));
    }
    IP ip() const
    {
        return std::get<IP> (static_cast<std::pair<IP, UDP_Port>> (*this));
    }
    UDP_Port udp_port() const
    {
        return std::get<UDP_Port> (static_cast<std::pair<IP, UDP_Port>>(*this));
    }
    IP& ip()
    {
        auto *p = reinterpret_cast<std::pair<IP, UDP_Port>*> (this);
        return std::get<IP> (*p);
    }
    UDP_Port& udp_port()
    {
        auto *p = reinterpret_cast<std::pair<IP, UDP_Port>*> (this);
        return std::get<UDP_Port> (*p);
    }
};

struct FENRIR_LOCAL Link_Params :
            type_safe::strong_typedef<Link_ID, std::pair<uint16_t, uint16_t>>
{
    using strong_typedef::strong_typedef;

    uint16_t mtu() const
    {
        return static_cast<const std::pair<uint16_t, uint16_t>> (*this).first;
    }
    uint16_t init_window() const
    {
        return static_cast<const std::pair<uint16_t, uint16_t>> (*this).second;
    }
};


} // namespace Impl
} // namespace Fenrir__v1

