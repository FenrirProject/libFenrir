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

#include "Fenrir/v1/auth/Auth.hpp"
#include "Fenrir/v1/common.hpp"
#include "Fenrir/v1/data/AS_list.hpp"
#include "Fenrir/v1/data/IP.hpp"
#include "Fenrir/v1/data/packet/Packet.hpp"
#include "Fenrir/v1/event/IO.hpp"
#include "Fenrir/v1/event/Events_Base.hpp"
#include "Fenrir/v1/net/Direction.hpp"
#include "Fenrir/v1/net/Handshake_ID.hpp"
#include "Fenrir/v1/resolve/Resolver.hpp"
#include "Fenrir/v1/net/Socket.hpp"
#include <memory>

namespace Fenrir__v1 {
namespace Impl {

class Connection;

namespace Event {


// Socket events
class FENRIR_LOCAL Keepalive final : public Timer
{
public:
    std::weak_ptr<Connection> _conn;
    Link_ID _link;
    Direction _incoming;

    Keepalive() = delete;
    Keepalive (const Keepalive&) = delete;
    Keepalive& operator= (const Keepalive&) = delete;
    Keepalive (Keepalive &&) = delete;
    Keepalive& operator= (Keepalive &&) = delete;
    Keepalive (Loop *const loop, const std::weak_ptr<Connection> &conn,
                                                    const Link_ID link,
                                                    const Direction incoming)
        : Timer (Type::KEEPALIVE, loop), _conn (conn), _link(link),
                                                            _incoming (incoming)
        {}
    ~Keepalive()
        {}
    static std::shared_ptr<Keepalive> mk_shared (
                                        Loop *const loop,
                                        const std::weak_ptr<Connection> &conn,
                                        const Link_ID link,
                                        const Direction incoming)
    {
        auto ret = std::make_shared<Keepalive> (loop, conn, link, incoming);
        ret->_ourselves = ret;
        return ret;
    }
};


class FENRIR_LOCAL Read final : public IO
{
public:
    std::weak_ptr<Socket> socket;

    Read (const Read&) = delete;
    Read& operator= (const Read&) = delete;
    Read (Read &&) = delete;
    Read& operator= (Read &&) = delete;
    Read (Loop *const loop, std::shared_ptr<Socket> sock)
        : IO (Type::READ, loop, sock->get_fd(), Impl::IO::READ), socket (sock)
        {}
    ~Read()
        {}
    static std::shared_ptr<Read> mk_shared (Loop *const loop,
                                                std::shared_ptr<Socket> sock)
    {
        auto ret = std::make_shared<Read> (loop, sock);
        ret->_ourselves = ret;
        return ret;
    }
};

class FENRIR_LOCAL Send final : public Timer
{
public:
    class Data {};
    std::unique_ptr<Data> _data;

    Send (const Send&) = delete;
    Send& operator= (const Send&) = delete;
    Send (Send &&) = delete;
    Send& operator= (Send &&) = delete;
    Send (Loop *const loop, std::unique_ptr<Data> data)
        : Timer (Type::SEND, loop), _data (std::move(data))
        {}
    ~Send()
        {}
    static std::shared_ptr<Send> mk_shared (Loop *const loop,
                                                    std::unique_ptr<Data> data)
    {
        auto ret = std::make_shared<Send> (loop, std::move(data));
        ret->_ourselves = ret;
        return ret;
    }
};

} // namespace Envent

namespace Resolve {
class Resolver;
} // namespace Resolve

namespace Event {

// FIXME: move to main types specified in Event_Base.hpp
// connection-related events
enum class Connect_Status : uint8_t {
                                    RESOLV = 0x00,
                                    INIT = 0x01,
                                    DROP = 0x02,
                                    SUCCESS = 0x03
                                    };

class FENRIR_LOCAL Connect : public Timer
{
public:
    Connect_Status status;

    Connect() = delete;
    Connect (const Connect&) = delete;
    Connect& operator= (const Connect&) = delete;
    Connect (Connect &&) = delete;
    Connect& operator= (Connect &&) = delete;
    Connect (Loop *const loop, Connect_Status stat)
        :Timer (Type::CONNECT, loop), status(stat)
        {}
    ~Connect()
        {}
};

class FENRIR_LOCAL Resolve final : public Connect
{
public:
    Impl::Resolve::AS_list _as;
    std::vector<uint8_t> _fqdn;
    Impl::Resolve::Resolver::ID _last_resolver;
    Error _err;

    Resolve() = delete;
    Resolve (const Resolve&) = delete;
    Resolve& operator= (const Resolve&) = delete;
    Resolve (Resolve &&) = delete;
    Resolve& operator= (Resolve &&) = delete;
    Resolve (Loop *const loop, std::vector<uint8_t> &&fqdn)
        : Connect (loop, Connect_Status::RESOLV),
            _fqdn (std::forward<std::vector<uint8_t>>(fqdn)),
            _err (Error::WORKING)
        {}
    ~Resolve()
        {}
    static std::shared_ptr<Resolve> mk_shared (Loop *const loop,
                                                    std::vector<uint8_t> &&fqdn)
    {
        auto ret = std::make_shared<Resolve> (loop,
                                    std::forward<std::vector<uint8_t>> (fqdn));
        ret->_ourselves = ret;
        return ret;
    }
};

// "Handshake" is used only to drop the handshake
class FENRIR_LOCAL Handshake final : public Timer
{
public:
    enum class TYPE : uint8_t {
        SEND = 0x01,
        TIMEOUT = 0x02
    };

    Handshake_ID _id;
    TYPE _type;

    Handshake() = delete;
    Handshake (const Handshake&) = delete;
    Handshake& operator= (const Handshake&) = delete;
    Handshake (Handshake &&) = delete;
    Handshake& operator= (Handshake &&) = delete;
    Handshake (Loop *const loop, const Handshake_ID id, const TYPE type)
        : Timer (Type::HANDSHAKE, loop), _id (id), _type (type)
        {}
    ~Handshake()
        {}
    static std::shared_ptr<Handshake> mk_shared (Loop *const loop,
                                                        const Handshake_ID id,
                                                        const TYPE type)
    {
        auto ret = std::make_shared<Handshake> (loop, id, type);
        ret->_ourselves = ret;
        return ret;
    }
};

class FENRIR_LOCAL Plugin_Timer : public Timer
{
public:
    std::weak_ptr<Dynamic> _plg;

    Plugin_Timer() = delete;
    Plugin_Timer (const Plugin_Timer&) = delete;
    Plugin_Timer& operator= (const Plugin_Timer&) = delete;
    Plugin_Timer (Plugin_Timer &&) = delete;
    Plugin_Timer& operator= (Plugin_Timer &&) = delete;
    Plugin_Timer (Loop *const loop, std::weak_ptr<Dynamic> plg)
        : Timer (Type::PLUGIN_TIMER, loop), _plg (std::move(plg))
        {}
    ~Plugin_Timer()
        {}
    static std::shared_ptr<Plugin_Timer> mk_shared (Loop *const loop,
                                                    std::weak_ptr<Dynamic> plg)
    {
        auto ret = std::make_shared<Plugin_Timer> (loop, plg);
        ret->_ourselves = ret;
        return ret;
    }
};

class FENRIR_LOCAL Plugin_IO : public IO
{
public:
    std::weak_ptr<Dynamic> _plg;

    Plugin_IO() = delete;
    Plugin_IO (const Plugin_IO&) = delete;
    Plugin_IO& operator= (const Plugin_IO&) = delete;
    Plugin_IO (Plugin_IO &&) = delete;
    Plugin_IO& operator= (Plugin_IO &&) = delete;
    Plugin_IO (Loop *const loop, std::weak_ptr<Dynamic> plg, int fd,
                                                                Impl::IO type)
        : IO (Type::PLUGIN_IO, loop, fd, type), _plg (std::move(plg))
        {}
    ~Plugin_IO()
        {}
};

class FENRIR_LOCAL User final : public Base
{
public:
    std::shared_ptr<void> user_data;

    User (const User&) = delete;
    User& operator= (const User&) = delete;
    User (User &&) = delete;
    User& operator= (User &&) = delete;
    User (Loop *const loop)
        :Base (Type::USER, loop)
        {}
    ~User()
        {}
};

} // namespace Event
} // namespace Impl
} // namespace Fenrir__v1
