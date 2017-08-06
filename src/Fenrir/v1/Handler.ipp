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

#include "Fenrir/v1/API/Report.hpp"
#include "Fenrir/v1/db/Db.hpp"
#include "Fenrir/v1/event/Events_all.hpp"
#include "Fenrir/v1/Handler.hpp"
#include "Fenrir/v1/net/Connection.hpp"
#include "Fenrir/v1/net/Socket.hpp"
#include "Fenrir/v1/rate/RR-RR.hpp"
#include <type_safe/optional.hpp>

namespace Fenrir__v1 {
namespace Impl {

FENRIR_INLINE Handler::Handler()
    : _load (&_loop, &_rnd),
      _db (_load.get_shared<Db> (Db::ID{1})),
      _handshakes (&_loop, &_rnd, &_load, this, _db.get()),
      _keepalive_fail_before_drop (4)
{
    if (!_loop)
        return;
    _loop.loop();
     // FIXME: Db
#pragma clang push //A copy of a 32 bit value is more efficient than a reference
#pragma clang diagnostic ignored "-Wrange-loop-analysis"
    for (const auto id : _load.list<Resolve::Resolver>()) {
        _resolvers.emplace_back (_load.get_shared<Resolve::Resolver> (id));
        if (!_resolvers.rbegin()->get()->initialize())
            _resolvers.pop_back();
    }
#pragma clang pop
    _rate = std::make_shared<Rate::RR_RR> (nullptr, &_loop, &_load, &_rnd,this);
    _handshakes.add_auth (Crypto::Auth::ID{1}); // token
}

FENRIR_INLINE Handler::operator bool() const
    { return static_cast<bool> (_loop); }


FENRIR_INLINE bool Handler::load_pubkey (const Crypto::Key::Serial serial,
                                          const Crypto::Key::ID type,
                                          const gsl::span<const uint8_t> priv,
                                          const gsl::span<const uint8_t> pub)
    { return _handshakes.load_pubkey (serial, type, priv, pub); }

FENRIR_INLINE bool Handler::del_pubkey (const Crypto::Key::Serial serial)
    { return _handshakes.del_pubkey (serial); }


FENRIR_INLINE bool Handler::listen (const Link_ID id)
{
    auto sk = std::make_shared<Socket> (id);
    if (sk == nullptr || !*sk)
        return false;
    std::shared_ptr<Event::Read> sk_ev = Event::Read::mk_shared (&_loop, sk);
    sk->sock_ev = sk_ev;
    Shared_Lock_Guard<Shared_Lock_Write> lock (Shared_Lock_NN{&_sock_lock});
    _sockets.emplace_back (id, std::move(sk));
    lock.early_unlock();

    _loop.start(sk_ev);

    // TODO: add to existing connections. (but what if local net only?)
    return true;
}

FENRIR_INLINE void Handler::connect (const std::vector<uint8_t> &dest,
                                                    const Service_ID &service)
{
    std::vector<uint8_t> fqdn = dest;
    //fqdn.reserve (dest.size() + 1);
    //std::copy (dest.begin(), dest.end(), fqdn.begin());
    //fqdn.push_back ('\0');
    auto res_ev = Event::Resolve::mk_shared (&_loop, std::move(fqdn));

    Shared_Lock_Guard<Shared_Lock_Read> rlock (Shared_Lock_NN {&_res_lock});
    FENRIR_UNUSED (rlock);
    if (_resolvers.size() == 0)
        return;
    _resolvers[0]->resolv_async (std::move (res_ev));
}

FENRIR_INLINE std::unique_ptr<Report::Base> Handler::get_report()
{
    // FIXME: exit condition on destruction
    while (1) {
        auto wrk = _loop.wait();
        if (wrk != nullptr)
            do_work (std::move(wrk));
        std::unique_lock<std::mutex> lock (_rep_lock);
        FENRIR_UNUSED (lock);
        if (_user_reports.size() == 0) {
            lock.unlock();
            continue;
        }
        auto ret = std::move(_user_reports.front());
        _user_reports.pop_front();
        return ret;
    }
}


FENRIR_INLINE std::shared_ptr<Socket> Handler::get_socket (const Link_ID id)
{

    Shared_Lock_Guard<Shared_Lock_Read> rlock (Shared_Lock_NN{&_sock_lock});
    FENRIR_UNUSED (rlock);
    auto it = std::lower_bound (_sockets.begin(), _sockets.end(), id,
                                [] (const auto &sk, const Link_ID link)
                                    { return std::get<Link_ID> (sk) < link; });
    if (it == _sockets.end())
        return nullptr;
    return std::get<std::shared_ptr<Socket>> (*it);
}

FENRIR_INLINE void Handler::proxy_enqueue (const Link_ID from, const Link_ID to,
                        std::unique_ptr<Packet> pkt, const Conn0_Type handshake)
    { return _rate->enqueue (from, to, std::move(pkt), handshake); }
FENRIR_INLINE Link_Params Handler::proxy_def_link_params()
    { return _rate->def_link_params(); }

FENRIR_INLINE void Handler::do_work (std::shared_ptr<Event::Base> ev)
{
    switch (ev->_type) {
    case Event::Type::READ:
        return read_pkt (std::static_pointer_cast<Event::Read> (ev));
    case Event::Type::SEND:
        return send_pkt (std::static_pointer_cast<Event::Send> (ev));
    case Event::Type::KEEPALIVE:
        return ev_keepalive (std::static_pointer_cast<Event::Keepalive>(ev));
    case Event::Type::HANDSHAKE:
        return _handshakes.drop_handshake (
                            std::static_pointer_cast<Event::Handshake> (ev));
    case Event::Type::CONNECT:
        return connect (std::static_pointer_cast<Event::Connect> (ev));
    case Event::Type::PLUGIN_TIMER:
        return plg_ev (std::static_pointer_cast<Event::Plugin_Timer> (ev));
    case Event::Type::USER:
        // TODO: report ev to user
        return;
    }
    assert (false && "Fenrir: do_work: nonexaustive switch?");
    return;
}

FENRIR_INLINE Error Handler::add_connection (std::shared_ptr<Connection> conn)
{
    const Conn_ID id = conn->_read_connection_id;

    {
        Shared_Lock_Guard<Shared_Lock_Write> rlock(Shared_Lock_NN{&_sock_lock});
        for (const auto &sock : _sockets)
            conn->add_Link_out (std::get<Link_ID> (sock));
    }

    Shared_Lock_Guard<Shared_Lock_Write> wlock (Shared_Lock_NN{&_conn_lock});
    FENRIR_UNUSED (wlock);
    auto res_pair = _connections.insert ({id, std::move(conn)});
    if (std::get<bool> (res_pair))
        return Error::NONE;
    if (res_pair.first == _connections.end())
        return Error::FULL; // probably memory problems?
    return Error::ALREADY_PRESENT;
}

FENRIR_INLINE std::shared_ptr<Connection> Handler::get_connection (
                                                            const Conn_ID id)
{
    Shared_Lock_Guard<Shared_Lock_Read> rlock (Shared_Lock_NN{&_conn_lock});
    FENRIR_UNUSED (rlock);
    auto it = _connections.find (id);
    if (it == _connections.end())
        return nullptr;
    return std::get<std::shared_ptr<Connection>> (*it);
}

FENRIR_INLINE Conn_ID Handler::get_next_free (const Conn_ID id)
{
    auto next_conn = Conn_ID {static_cast<uint32_t> (id) + 1};
    Shared_Lock_Guard<Shared_Lock_Read> rlock (Shared_Lock_NN{&_sock_lock});
    FENRIR_UNUSED (rlock);
    auto it = _connections.find (next_conn);
    if (it == _connections.end())
        return next_conn;
    constexpr uint32_t max_conn = ~static_cast<uint32_t> (0);

    while (true) {
        if (std::get<const Conn_ID> (*it) == Conn_ID{max_conn})
            return get_next_free (Conn_ID{Conn_Reserved}); // overflow
        next_conn = Conn_ID {1 + static_cast<uint32_t> (next_conn)};
        ++it;
        if (it == _connections.end())
            return next_conn;
        if (std::get<const Conn_ID> (*it) != next_conn)
            return next_conn;
    }
}


enum class Received_Type : uint8_t {
                                    IGNORE = 0x00,
                                    DATA = 0x01,
                                    RESOLV = 0x02,
                                    CONNECT = 0x03
                                    };

FENRIR_INLINE void Handler::read_pkt (std::shared_ptr<Event::Read> ev)
{
    // read data from socket:
    auto sock = ev->socket.lock();
    if (sock == nullptr)
        return;

    auto read = sock->read();

    Packet pkt {std::move(std::get<std::vector<uint8_t>> (read))};

    if (!pkt)
        return;

    if (pkt.connection_id() == Conn_ID {0}) {
        if (pkt.parse (pkt.data_no_id(), Packet::Alignment_Byte::UINT8) !=
                                                                Error::NONE) {
            return;
        }
        return _handshakes.recv (std::get<Link_ID> (read), sock->id(), pkt);
    }

    if (pkt.connection_id() == Conn_ID {1} ||
                                        pkt.connection_id() == Conn_ID {2}) {
        // TODO: placeholder for proxy and multicast.
        return;
    }

    auto shared_conn = get_connection (pkt.connection_id());
    if (shared_conn == nullptr)
        return;

    auto activation_pkt = shared_conn->update_source (std::get<Link_ID> (read));
    if (activation_pkt!= nullptr) {
        // we need to send the activation link
        auto dest = std::get<Link_ID> (read);
        sock->write (activation_pkt->raw, dest);
        if (activation_pkt != nullptr &&
                                activation_pkt->raw.size() <= pkt.raw.size()) {
            return _rate->enqueue (sock->id(), std::get<Link_ID> (read),
                                std::move(activation_pkt), type_safe::nullopt);
        }
    }

    shared_conn->recv (pkt);
}

FENRIR_INLINE void Handler::send_pkt (std::shared_ptr<Event::Send> ev)
{
    auto out = _rate->send (std::move (ev->_data));

    if (!out.has_value())
        return;

    auto sock = std::move (out.value()._from);

    sock->write (out.value()._pkt->raw, out.value()._to);
}

FENRIR_INLINE void Handler::ev_keepalive (std::shared_ptr<Event::Keepalive> ev)
{
    auto conn = ev->_conn.lock();
    if (conn == nullptr)
        return;

    type_safe::optional<std::tuple<Link_ID, std::unique_ptr<Packet>>> keepalive;
    std::tuple<Link_ID, std::unique_ptr<Packet>> val;
    switch (ev->_incoming) {
    case Direction::INCOMING:
        return conn->missed_keepalive_in (ev->_link,
                                                _keepalive_fail_before_drop);
    case Direction::OUTGOING:
        keepalive = conn->missed_keepalive_out (ev->_link);
        if (!keepalive.has_value())
            return;
        val = std::move (keepalive.value());
        return _rate->enqueue (ev->_link, std::get<Link_ID> (val),
                            std::move(std::get<std::unique_ptr<Packet>> (val)),
                                                            type_safe::nullopt);
    }
}

FENRIR_INLINE void Handler::connect (std::shared_ptr<Event::Connect> ev)
{
    switch (ev->status) {
    case Event::Connect_Status::RESOLV:
        return connect_resolve (std::static_pointer_cast<Event::Resolve> (ev));
    }

    return;
}

FENRIR_INLINE void Handler::connect_resolve (std::shared_ptr<Event::Resolve> ev)
{
    if (ev->_err == Error::RESOLVE_NOT_FOUND) {
        Shared_Lock_Guard<Shared_Lock_Read> lock (Shared_Lock_NN {&_res_lock});
        FENRIR_UNUSED (lock);
        uint32_t idx;
        for (idx = 0; idx < _resolvers.size(); ++idx) {
            if (_resolvers[idx]->id() == ev->_last_resolver) {
                ++idx;
                break;
            }
        }
        if (idx < _resolvers.size()) {
            _resolvers[idx]->resolv_async (std::move(ev));
        } else {
            std::unique_lock<std::mutex> rep_lock (_rep_lock);
            FENRIR_UNUSED (rep_lock);
            // no more resolvers to try.
            _user_reports.emplace_back (
                    new Report::Resolve (Fenrir__v1::Error::NO_SUCH_DOMAIN));
        }
        return;
    }
    if (ev->_err == Error::RESOLVE_NOT_FENRIR) {
        std::unique_lock<std::mutex> rep_lock (_rep_lock);
        FENRIR_UNUSED (rep_lock);
        _user_reports.emplace_back (
                    new Report::Resolve (Fenrir__v1::Error::DOMAIN_NOT_FENRIR));
        return;
    }

    // done resolving, try to connect
    std::unique_ptr<Packet> pkt;
    Link_ID dest;
    std::tie (pkt, dest) = _handshakes.connect (ev->_as);
    if (dest.ip() == IP())
        return;

    Shared_Lock_Guard<Shared_Lock_Read> rlock (Shared_Lock_NN{&_sock_lock});
    const auto from = std::get<Link_ID> (_sockets[_rnd.uniform<uint32_t> (0,
                                    static_cast<uint32_t> (_sockets.size()))]);
    rlock.early_unlock();

    _rate->enqueue (from, dest, std::move(pkt), Conn0_Type::C_INIT);
}

FENRIR_INLINE void Handler::plg_ev (std::shared_ptr<Event::Plugin_Timer> ev)
{
    auto plugin = ev->_plg.lock();
    if (plugin == nullptr) {
        _loop.del (std::move(ev));
        return;
    }
    return plugin->parse_event (std::move(ev));
}

} // namespace Impl
} // namespace Fenrir__v1
