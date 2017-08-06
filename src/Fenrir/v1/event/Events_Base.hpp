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
#include "Fenrir/v1/event/IO.hpp"
#include "Fenrir/v1/event/Loop_callbacks.hpp"
#include <chrono>
#include <ev.h>
#include <memory>

namespace Fenrir__v1 {
namespace Impl {
namespace Event {

class Loop;


enum class FENRIR_LOCAL Type : uint8_t {
                            READ         = 0x01,
                            SEND         = 0x02,
                            KEEPALIVE    = 0x03,
                            HANDSHAKE    = 0x04,
                            CONNECT      = 0X05,
                            PLUGIN_TIMER = 0x06,
                            PLUGIN_IO    = 0x07,
                            USER         = 0x08
                            };


// This will be used cuncurrently, so always use a shared_ptr for this.
class FENRIR_LOCAL Base
{
public:
    // loop, ourselves, ev, timeout are initialized in add_(timer/once/listener)
    std::shared_ptr<Base> _ourselves; // yes, this circular shared_ptr is wanted
    Loop *const _loop;
    const Type _type;

    enum class status : uint8_t {NON_INITIALIZED, INITIALIZED, STARTED};
    status _status;

    Base (Type t, Loop *const loop)
        : _loop (loop), _type (t), _status (status::NON_INITIALIZED)
        {}
    ~Base()
        {}

    Base() = delete;
    Base (const Base&) = delete;
    Base& operator= (const Base&) = delete;
    Base (Base &&) = default; // needed for static_pointer_cast
    Base& operator= (Base &&) = default;

    bool operator== (const Base& e) const
        { return _ourselves == e._ourselves; }
    bool operator!= (const Base& e) const
        { return _ourselves != e._ourselves; }
};

class FENRIR_LOCAL Timer : public Base
{
public:
    ev_timer _time_ev;

    Timer (Type t, Loop *const loop)
        : Base (t, loop)
    {
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wold-style-cast"
        ev_timer_init (&_time_ev, cb_timer, 0., 0.);
        #pragma clang diagnostic pop
        _time_ev.data = this;
        _status = Base::status::INITIALIZED;
    }

    ~Timer()
        {}

    Timer() = delete;
    Timer (const Timer&) = delete;
    Timer& operator= (const Timer&) = delete;
    Timer (Timer &&) = default; // needed for static_pointer_cast
    Timer& operator= (Timer &&) = default;
};

class FENRIR_LOCAL IO : public Base
{
public:
    ev_io _io_ev;

    IO (Type t, Loop *const loop, int fd, const Impl::IO type)
        : Base (t, loop)
    {
        int flag = 0;
        if (io_has (type, Impl::IO::READ))
            flag |= EV_READ;
        if (io_has (type, Impl::IO::WRITE))
            flag |= EV_WRITE;
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wold-style-cast"
        ev_io_init (&_io_ev, cb_io, fd, flag);
        _io_ev.data = this;
        #pragma clang diagnostic pop
        _status = Base::status::INITIALIZED;
    }

    ~IO()
        {}

    IO() = delete;
    IO (const IO&) = delete;
    IO& operator= (const IO&) = delete;
    IO (IO &&) = default; // needed for static_pointer_cast
    IO& operator= (IO &&) = default;
};

// TODO: file change

} // namespace Event
} // namespace Impl
} // namespace Fenrir__v1
