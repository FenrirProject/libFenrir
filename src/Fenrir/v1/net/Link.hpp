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
#include "Fenrir/v1/event/Loop.hpp"
#include "Fenrir/v1/event/Events_all.hpp"
#include "Fenrir/v1/net/Link_defs.hpp"
#include "Fenrir/v1/util/Random.hpp"
#include <chrono>
#include <limits>
#include <memory>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {

// easy way to represent a source/destination of data
class FENRIR_LOCAL Link
{
public:
    static constexpr std::chrono::microseconds keepalive_timeout =
                                                    std::chrono::seconds { 15 };
    using ID = Link_ID;
    using smallmicro = std::chrono::duration<uint32_t, std::micro>;

    std::vector<uint8_t> _activation;   // random 256 bit: used for confirmation
                                    // of 1-RTT. if (size() == 0) then confirmed
    uint64_t _max_rate;  // bps
    // _keepalive:
    //   if _activation.size() > 0, then a timeout will drop
    //     this link from the queue.
    //   else
    //     only drop after 4 keepalive failed.
    std::shared_ptr<Event::Keepalive> _keepalive;
    smallmicro _rtt;    // Microseconds. 32 bits should be enough to maintain a
                        // connection towards Mars: Shoot for the stars,
                        // if you miss you'll only end up drifting endlessly.
    Link_ID _link;
    uint16_t _mtu;
    uint8_t _keepalive_failed;


    Link() = delete;
    Link (const Link&) = default;
    Link& operator= (const Link&) = delete;
    Link (Link&&) = default;
    Link& operator= (Link&&) = default;
    ~Link() = default;

    Link (const Link_ID link, std::shared_ptr<Event::Keepalive> keepalive,
                                const uint16_t mtu, const uint64_t max_rate)
        : _max_rate (max_rate), _keepalive (std::move(keepalive)),
                                _rtt (std::numeric_limits<smallmicro>::max()),
                                _link (link), _mtu (mtu)
    {}

    Link (const Link_ID link, Random *const rnd,
                                    std::shared_ptr<Event::Keepalive> keepalive)
        : _max_rate (0), _keepalive (std::move(keepalive)),
                                _rtt (std::numeric_limits<smallmicro>::max()),
                                _link (link), _mtu (0), _keepalive_failed (0)
    {
        constexpr size_t size = 32;
        _activation.reserve (size);
        rnd->uniform<uint8_t> (_activation);
    }

    bool operator== (const Link &s) const
        { return static_cast<bool> (_link == s._link); }

    explicit operator bool() const
        { return _activation.size() == 0; }

    void keepalive (Event::Loop *loop)
        { loop->start (_keepalive, keepalive_timeout, Event::Repeat::YES); }

    void deactivate (Event::Loop *loop)
        { loop->deactivate (_keepalive); }
};


} // namespace Impl
} // namespace Fenrir__v1

