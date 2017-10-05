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
#include "Fenrir/v1/data/packet/Packet.hpp"
#include "Fenrir/v1/event/Events_all.hpp"
#include "Fenrir/v1/event/Loop.hpp"
#include "Fenrir/v1/net/Handshake.hpp"
#include "Fenrir/v1/plugin/Loader.hpp"
#include "Fenrir/v1/service/Service_ID.hpp"
#include "Fenrir/v1/service/Service_Info.hpp"
#include "Fenrir/v1/util/Shared_Lock.hpp"
#include <map>
#include <memory>
#include <vector>
#include <utility>

namespace Fenrir__v1 {
namespace Report {
class Base;
} // namespace Report

namespace Impl {

class Connection;

namespace Rate {
class Rate;
} // namespace Rate

class Socket;

class FENRIR_LOCAL Handler {
public:
    Handler();
    Handler (const Handler&) = default;
    Handler& operator= (const Handler&) = default;
    Handler (Handler &&) = default;
    Handler& operator= (Handler &&) = default;
    ~Handler() = default;
    explicit operator bool() const;

    bool load_pubkey (const Crypto::Key::Serial serial,
                                          const Crypto::Key::ID type,
                                          const gsl::span<const uint8_t> priv,
                                          const gsl::span<const uint8_t> pub);
    bool del_pubkey (const Crypto::Key::Serial serial);
    bool listen (const Link_ID id);


    std::shared_ptr<Socket> get_socket (const Link_ID id);
    void proxy_enqueue (const Link_ID from, const Link_ID to,
                    std::unique_ptr<Packet> pkt, const Conn0_Type handshake);
    Link_Params proxy_def_link_params ();
    Error add_connection (std::shared_ptr<Connection> conn);
    std::shared_ptr<Connection> get_connection (const Conn_ID id);
    Conn_ID get_next_free (const Conn_ID id);
    void connect (const std::vector<uint8_t> &dest, const Service_ID &service);
    std::unique_ptr<Report::Base> get_report();
private:
    struct FENRIR_LOCAL Vhost_Service :
                type_safe::strong_typedef<Vhost_Service,
                                std::pair<Service_ID, std::vector<uint8_t>>>,
                type_safe::strong_typedef_op::equality_comparison<Vhost_Service>
    {
        using strong_typedef::strong_typedef;
        bool operator< (const Vhost_Service &rhs) const
        {
            // see issue #1
            // try to make this parse the whole pair instead of returning early
            // TODO: this is clumsy. better ides to avoid branching?
            const auto *a = reinterpret_cast<const std::pair<
                                    Service_ID, std::vector<uint8_t>>*> (this);
            const auto *b = reinterpret_cast<const std::pair<
                                    Service_ID, std::vector<uint8_t>>*> (&rhs);
            int32_t x = 0;
            if (a->first < b->first) {
                x -= 10;
            } else {
                x += 10;
            }
            // FIXME: issue #1: not constant time, and try to avoid branching
            // we don't reall car wht the ordering is, as long as there
            // actually is one
            if (a->second < b->second) {
                x -= 5;
            } else {
                x += 5;
            }
            return x < 0;
        }
    };

    Shared_Lock _conn_lock, _sock_lock, _srv_lock, _res_lock;
    std::mutex _rep_lock;
    Event::Loop _loop;
    Random _rnd;
    Loader _load;
    std::shared_ptr<Db> _db;
    std::shared_ptr<Rate::Rate> _rate;
    std::vector<std::shared_ptr<Resolve::Resolver>> _resolvers;
    std::map<Conn_ID, std::shared_ptr<Connection>> _connections;
    std::vector<std::pair<Vhost_Service, Service_Info>> _service_info;
    std::vector<std::pair<Vhost_Service, std::shared_ptr<Connection>>>_services;
    std::vector<std::pair<Link_ID, std::shared_ptr<Socket>>> _sockets;
    std::deque<std::unique_ptr<Report::Base>> _user_reports;
    Handshake _handshakes;
    uint8_t _keepalive_fail_before_drop;

    void do_work (std::shared_ptr<Event::Base> ev);

    void read_pkt (std::shared_ptr<Event::Read> ev);
    void send_pkt (std::shared_ptr<Event::Send> ev);
    void ev_keepalive (std::shared_ptr<Event::Keepalive> ev);
    void connect (std::shared_ptr<Event::Connect> ev);
    void connect_resolve (std::shared_ptr<Event::Resolve> ev);
    void plg_ev (std::shared_ptr<Event::Plugin_Timer> ev);
};

} // namespace Impl
} // namespace Fenrir__v1

#ifdef FENRIR_HEADER_ONLY
#include "Fenrir/v1/Handler.ipp"
#endif
