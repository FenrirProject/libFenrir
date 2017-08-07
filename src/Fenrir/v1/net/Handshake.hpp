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
#include "Fenrir/v1/data/AS_list.hpp"
#include "Fenrir/v1/data/packet/Packet.hpp"
#include "Fenrir/v1/data/Conn0.hpp"
#include "Fenrir/v1/event/Loop.hpp"
#include "Fenrir/v1/event/Events_all.hpp"
#include "Fenrir/v1/net/Handshake_ID.hpp"
#include "Fenrir/v1/net/Link_defs.hpp"
#include "Fenrir/v1/plugin/Loader.ipp"
#include "Fenrir/v1/util/Shared_Lock.hpp"
#include <deque>

namespace Fenrir__v1 {
namespace Impl {

namespace Event {
class Loop;
}
class Loader;
class Random;
class Handler;

// handle packets with Connection_ID == 0
class FENRIR_LOCAL Handshake
{
public:
    Handshake (Event::Loop *const loop, Random *const rnd, Loader *const load,
                                        Handler *const handler, Db *const db)
        :_loop (loop), _rnd (rnd), _load (load), _handler (handler), _db (db)
    {
        std::array<uint8_t, 64> key;
        _rnd->uniform<uint8_t> (key);

        _srv_secret = _load->get_shared<Crypto::Encryption> (
                                                    Crypto::Encryption::ID {2});
        _srv_secret->set_key (key);
        assert (_srv_secret->is_authenticated() &&
                                    "Handshake: srv_secret not authenticated");
        // FIXME: cleanup. keys should be properly deleted
    }
    Handshake() = delete;
    Handshake (const Handshake&) = delete;
    Handshake& operator= (const Handshake&) = delete;
    Handshake (Handshake &&) = default;
    Handshake& operator= (Handshake &&) = default;
    ~Handshake() = default;

    bool add_auth (const Crypto::Auth::ID id);
    bool del_auth (const Crypto::Auth::ID id);
    std::vector<Crypto::Auth::ID> list_auths();
    bool setpref_auths (std::vector<Crypto::Auth::ID> pref);
    bool load_pubkey (const Crypto::Key::Serial serial,
                                          const Crypto::Key::ID type,
                                          const gsl::span<const uint8_t> priv,
                                          const gsl::span<const uint8_t> pub);
    bool del_pubkey (const Crypto::Key::Serial serial);

    void recv (const Link_ID from, const Link_ID to, Packet &pkt);
    void drop_handshake (std::shared_ptr<Event::Handshake> ev);

    std::tuple<std::unique_ptr<Packet>, Link_ID>
                                        connect (Resolve::AS_list &auth_servers)
            { return send_c_init (auth_servers); }

private:
    Shared_Lock _mtx_auths;
    Shared_Lock _mtx; // TODO: split
    std::mutex _srv_mtx;
    Event::Loop *const _loop;
    Random *const _rnd;
    Loader *const _load;
    Handler *const _handler;
    Db *const _db;
    std::vector<std::shared_ptr<Crypto::Auth>> auths;
    std::vector<std::pair<Crypto::Key::Serial, std::shared_ptr<Crypto::Key>>>
                                                                    _pubkeys;

    // track the handshakes we initialize
    using ID = Handshake_ID;

    struct state_client {
        Conn0_Type _type;   // last sent
        uint16_t _auth_server_idx;
        uint16_t _key_idx;
        Resolve::AS_list _auth_servers;
        std::unique_ptr<Packet> _pkt;   // last packet, used for signatures
        std::shared_ptr<Crypto::Key> _client_key;
        std::shared_ptr<Crypto::Encryption> _enc_read;
        std::shared_ptr<Crypto::Hmac> _hmac_read;
        std::shared_ptr<Recover::ECC> _ecc_read;
        std::shared_ptr<Crypto::Encryption> _enc_write;
        std::shared_ptr<Crypto::Hmac> _hmac_write;
        std::shared_ptr<Recover::ECC> _ecc_write;
        // TODO: provide KDF *and* deterministic rng for user
        //std::shared_ptr<Crypto::KDF> _kdf;

        state_client (Resolve::AS_list &list, uint16_t auth_srv_idx,
                                                    uint16_t key_idx,
                                                    std::unique_ptr<Packet> pkt)
            : _type (Conn0_Type::C_INIT),
              _auth_server_idx (auth_srv_idx), _key_idx (key_idx),
              _auth_servers (std::move(list)),
              _pkt (std::move(pkt)),
              _client_key (nullptr), _enc_read (nullptr), _hmac_read (nullptr),
              _ecc_read (nullptr), _enc_write (nullptr), _hmac_write (nullptr),
              _ecc_write (nullptr)
        {}
    };
    std::vector<std::pair<Handshake::ID, state_client>> _client_active;
    std::atomic<uint32_t> _srv_next_tracked;
    std::atomic<uint32_t> _client_next_tracked;
    std::shared_ptr<Crypto::Encryption> _srv_secret; // Authenticated enc.

    std::tuple<std::unique_ptr<Packet>, Link_ID>
                                send_c_init (Resolve::AS_list &auth_servers);
    void answer_c_init (const Link_ID from, const Link_ID to, const Packet &pkt,
                                                    const Conn0_C_INIT data);
    void answer_s_cookie (const Link_ID from, const Link_ID to,
                                const Packet &pkt, const Conn0_S_COOKIE data);
    void answer_c_cookie (const Link_ID from, const Link_ID to,
                                const Packet &pkt, const Conn0_C_COOKIE data);
    void answer_s_keys (const Link_ID from, const Link_ID to,
                                const Packet &pkt, const Conn0_S_KEYS data);
    void answer_c_auth (const Link_ID from, const Link_ID to,
                                const Packet &pkt, const Conn0_C_AUTH data);
    void parse_s_result (const Link_ID from, const Link_ID to,
                                const Packet &pkt, const Conn0_S_RESULT data);
};

} // namespace Impl
} // namespace Fenrir__v1

