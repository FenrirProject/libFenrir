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

#include "Fenrir/v1/auth/Lattice.hpp"
#include "Fenrir/v1/common.hpp"
#include "Fenrir/v1/util/Shared_Lock.ipp"
#include "Fenrir/v1/plugin/Loader.ipp"
#include "Fenrir/v1/Handler.hpp"
#include "Fenrir/v1/net/Handshake.hpp"
#include "Fenrir/v1/net/Connection.hpp"
#include "Fenrir/v1/data/Conn0.hpp"
#include "Fenrir/v1/data/Username.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <limits>

namespace Fenrir__v1 {
namespace Impl {

namespace {
constexpr auto pkt_timeout = std::chrono::duration_cast<
                        std::chrono::milliseconds> (std::chrono::seconds (3));
// minimum packet length received by server during connection init
// client can not send less than this to avoid amplification.
constexpr uint16_t pkt_init_minlen = 1280 - 8;
constexpr uint16_t conn_init_minlen = pkt_init_minlen - PKT_MINLEN;

} // empty namespace


// TODO: check all against issue #1 (constant time)

FENRIR_INLINE bool Handshake::add_auth (const Crypto::Auth::ID id)
{
    Shared_Lock_Guard<Shared_Lock_Write> lock {Shared_Lock_NN{&_mtx_auths}};
    FENRIR_UNUSED (lock);
    for (auto auth : auths) {
        if (auth->id() == id)
            return false;
    }
    auto lib = _load->get_shared<Crypto::Auth> (id);
    if (lib == nullptr)
        return false;
    auths.push_back (std::move(lib));
    return true;
}

FENRIR_INLINE bool Handshake::del_auth (const Crypto::Auth::ID id)
{
    Shared_Lock_Guard<Shared_Lock_Write> lock {Shared_Lock_NN{&_mtx_auths}};
    FENRIR_UNUSED (lock);
    for (auto auth = auths.begin(); auth < auths.end(); ++auth) {
        if ((*auth)->id() == id) {
            auths.erase (auth);
            return true;
        }
    }
    return false;
}

FENRIR_INLINE std::vector<Crypto::Auth::ID> Handshake::list_auths()
{
    Shared_Lock_Guard<Shared_Lock_Write> lock {Shared_Lock_NN{&_mtx_auths}};
    FENRIR_UNUSED (lock);
    std::vector<Crypto::Auth::ID> ret (auths.size());
    for (auto auth : auths)
        ret.push_back (auth->id());
    return ret;
}

FENRIR_INLINE bool Handshake::setpref_auths (std::vector<Crypto::Auth::ID> pref)
{
    Shared_Lock_Guard<Shared_Lock_Write> lock {Shared_Lock_NN{&_mtx_auths}};
    FENRIR_UNUSED (lock);
    if (pref.size() != auths.size())
        return false;
    for (auto auth : auths) {
        auto it = auths.begin();
        for (; it != auths.end(); ++it) {
            if ((*it)->id() == auth->id())
                break;
        }
        if (it == auths.end()) {
            return false; // missing preference
        }
    }
    std::vector<std::shared_ptr<Crypto::Auth>> _new_auths (auths.size());
    for (auto p : pref) {
        for (auto auth : auths) {
            if (auth->id() == p) {
                _new_auths.push_back (auth);
                break;
            }
        }
    }
    auths = std::move(_new_auths);
    return false;
}


FENRIR_INLINE bool Handshake::del_pubkey (const Crypto::Key::Serial serial)
{
    Shared_Lock_Guard<Shared_Lock_Write> lock {Shared_Lock_NN{&_mtx}};
    FENRIR_UNUSED (lock);
    auto it = std::find_if (_pubkeys.begin(), _pubkeys.end(),
                [serial] (const auto tmp)
                    { return std::get<Crypto::Key::Serial> (tmp) == serial; });
    if (it == _pubkeys.end())
        return false;
    if (_pubkeys.size() > 2 && it != _pubkeys.rbegin().base()) {
        std::swap (*it, *_pubkeys.rbegin());
        _pubkeys.pop_back();
    } else {
        _pubkeys.erase (it);
    }
    return true;
}

FENRIR_INLINE bool Handshake::load_pubkey (const Crypto::Key::Serial serial,
                                            const Crypto::Key::ID type,
                                            const gsl::span<const uint8_t> priv,
                                            const gsl::span<const uint8_t> pub)
{
    auto tmp_key = _load->get_shared<Crypto::Key> (type);
    if (tmp_key == nullptr)
        return false;

    if (!tmp_key->init (priv, pub))
        return false;

    Shared_Lock_Guard<Shared_Lock_Write> lock {Shared_Lock_NN{&_mtx}};
    FENRIR_UNUSED (lock);
    auto it = std::find_if (_pubkeys.begin(), _pubkeys.end(),
                [serial] (const auto tmp)
                    { return std::get<Crypto::Key::Serial> (tmp) == serial; });
    if (it != _pubkeys.end())
        return false;

    _pubkeys.emplace_back (serial, std::move(tmp_key));

    return true;
}

FENRIR_INLINE void Handshake::drop_handshake (
                                        std::shared_ptr<Event::Handshake> ev)
{
    Shared_Lock_Guard<Shared_Lock_Write> w_lock {Shared_Lock_NN(&_mtx)};
    FENRIR_UNUSED (w_lock);
    // search previous packet in _cative handshakes
    auto it = std::lower_bound (_client_active.begin(), _client_active.end(),
                        ev->_id,
                            [] (const auto &el, const Handshake::ID id)
                                { return std::get<Handshake::ID> (el) < id; });

    if (it == _client_active.end() || std::get<Handshake::ID> (*it) != ev->_id){
        return;     // we got triggered, but luckily the hanshake finished
                    // just a little before we deleted it.
    }
    _client_active.erase (it);
}



FENRIR_INLINE void Handshake::recv (const Link_ID from, const Link_ID to,
                                                                    Packet &pkt)
{
    if (pkt.stream.size() != 1 ||
                    static_cast<size_t> (pkt.stream.begin()->data().size()) <=
                                                            Conn0::min_size()) {
        return;
    }
    const auto conn_t = Conn0::get_type (pkt.stream.begin()->data());
    if (!conn_t.has_value())
        return;

    switch (conn_t.value()) {
    case Conn0_Type::C_INIT:
        return answer_c_init (from, to, pkt,
                                    Conn0_C_INIT (pkt.stream.begin()->data()));
    case Conn0_Type::S_COOKIE:
        return answer_s_cookie (from, to, pkt,
                                    Conn0_S_COOKIE(pkt.stream.begin()->data()));
    case Conn0_Type::C_COOKIE:
        return answer_c_cookie (from, to, pkt,
                                    Conn0_C_COOKIE(pkt.stream.begin()->data()));
    case Conn0_Type::S_KEYS:
        return answer_s_keys (from, to, pkt,
                                    Conn0_S_KEYS (pkt.stream.begin()->data()));
    case Conn0_Type::C_AUTH:
        return answer_c_auth (from, to, pkt,
                                    Conn0_C_AUTH (pkt.stream.begin()->data()));
    case Conn0_Type::S_RESULT:
        return parse_s_result (from, to, pkt,
                                    Conn0_S_RESULT (pkt.stream.begin()->data()));
    }
}

// FIXME: craete events (pkt_timeout) for handshake timeouts

FENRIR_INLINE std::tuple<std::unique_ptr<Packet>, Link_ID>
                        Handshake::send_c_init (Resolve::AS_list &auth_servers)
{
    if (auth_servers._err != Error::NONE || auth_servers._key.size() == 0)
        return {nullptr, Link_ID {{IP(), UDP_Port{0}}}};

    // select the auth server we eill contact
    const auto srv_idx = auth_servers.get_idx (0, _rnd);
    if (!srv_idx.has_value())
        return {nullptr, Link_ID {{IP(), UDP_Port{0}}}};
    const Link_ID srv_dest {{auth_servers._servers[srv_idx.value()].ip,
                                auth_servers._servers[srv_idx.value()].port}};


    const auto all_enc  = _load->list<Crypto::Encryption>();
    const auto all_hmac = _load->list<Crypto::Hmac>();
    const auto all_ecc  = _load->list<Recover::ECC>();
    const auto all_key  = _load->list<Crypto::Key>();
    const auto all_kdf  = _load->list<Crypto::KDF>();

    uint16_t msg_size = static_cast<uint16_t> (Conn0_C_INIT::min_size() +
                        (all_enc.size()  + all_hmac.size()  +
                             all_ecc.size() + all_key.size() + all_kdf.size()) *
                                                sizeof(Crypto::Encryption::ID));
    if (msg_size < conn_init_minlen)
        msg_size = conn_init_minlen;
    const uint16_t total_size = static_cast<uint16_t> (PKT_MINLEN + msg_size);
    auto pkt = std::make_unique<Packet> (total_size);
    // pkt->set_header() not needed: data initialized at 0 and connid=0/pad=0
    auto stream = pkt->add_stream (Stream_ID {0}, Stream::Fragment::FULL,
                                                        Counter {0}, msg_size);
    std::vector<Crypto::Key::ID> available_keys;
    available_keys.reserve (auth_servers._key.size());
    for (auto &key : auth_servers._key) {
        available_keys.push_back (
                            std::get<std::shared_ptr<Crypto::Key>> (key)->id());
    }
    auto k_type = _load->choose<Crypto::Key> (available_keys);
    if (k_type == Crypto::Key::ID {0}) {
        // FIXME: report no common keytype
        return {nullptr, Link_ID {{IP(), UDP_Port{0}}}};
    }
    uint32_t key_idx;
    for (key_idx = 0; key_idx < auth_servers._key.size(); ++key_idx) {
        if (std::get<std::shared_ptr<Crypto::Key>> (auth_servers._key[key_idx])
                                                            ->id() == k_type) {
            break;
        }
    }
    if (key_idx >= auth_servers._key.size() ||
                            key_idx > std::numeric_limits<uint16_t>::max()) {
        return {nullptr, Link_ID {{IP(), UDP_Port{0}}}}; // FIXME: report error
    }
    Conn0_C_INIT msg (stream->data(), _rnd,
                    std::get<Crypto::Key::Serial> (auth_servers._key[key_idx]),
                                all_enc, all_hmac, all_ecc, all_key, all_kdf);
    if (!msg)
        return {nullptr, Link_ID {{IP(), UDP_Port{0}}}}; // FIXME: report error
    auto copy_data = pkt->raw;
    auto copy_pkt = std::make_unique<Packet> (std::move(copy_data));
    // add stream faster than copy_pkt->parse() again
    copy_pkt->add_stream (Stream_ID {0}, Stream::Fragment::FULL,
                                                        Counter {0}, msg_size);
    Shared_Lock_Guard<Shared_Lock_Write> lock {Shared_Lock_NN{&_mtx}};
    FENRIR_UNUSED (lock);
    // search for a new, usable Handshake::ID
    Handshake::ID h_id;
    {
        auto it = _client_active.end();
        do {
            h_id = Handshake::ID {{Counter {_rnd->uniform<uint32_t> (0,
                                        static_cast<uint32_t> (max_counter))},
                                    Stream_ID{_rnd->uniform<uint16_t>()}}};
            it = std::lower_bound(_client_active.begin(), _client_active.end(),
                        h_id, [] (const auto &data, Handshake::ID id)
                            { return std::get<Handshake::ID> (data) < id; });
        } while (it != _client_active.end() &&
                                        std::get<Handshake::ID> (*it) == h_id);
    }

    auto raw_id = static_cast<std::pair<Counter, Stream_ID>> (h_id);
    pkt->stream[0].set_header (std::get<Stream_ID> (raw_id),
                                                   Stream::Fragment::FULL,
                                                   std::get<Counter> (raw_id));
    copy_pkt->stream[0].set_header (std::get<Stream_ID> (raw_id),
                                                   Stream::Fragment::FULL,
                                                   std::get<Counter> (raw_id));
    _client_active.emplace_back (h_id, state_client (auth_servers,
                                                srv_idx.value(),
                                                static_cast<uint16_t> (key_idx),
                                                std::move(copy_pkt)));
    std::sort (_client_active.begin(), _client_active.end(), []
                (const auto &state_a, const auto &state_b)
                    {
                        return std::get<Handshake::ID> (state_a) <
                                            std::get<Handshake::ID> (state_b);
                    });
    return {std::move(pkt), srv_dest};
}

FENRIR_INLINE void Handshake::answer_c_init (const Link_ID recv_from,
                                                        const Link_ID recv_to,
                                                        const Packet &pkt,
                                                        const Conn0_C_INIT data)
{
    if (pkt.raw.size() < pkt_init_minlen || !data ||
                                            data._supported_crypt.size() == 0 ||
                                            data._supported_hmac.size()  == 0 ||
                                            data._supported_key.size()   == 0) {
        return;
    }

    Shared_Lock_Guard<Shared_Lock_Read> lock {Shared_Lock_NN(&_mtx)};
    // check key_id
    auto key_it = std::lower_bound (_pubkeys.begin(), _pubkeys.end(),
                                                                data.r->_key_id,
                [] (const auto &test, const Crypto::Key::Serial id)
                    {
                        return static_cast<bool> (std::get<Crypto::Key::Serial>(
                                                                    test) < id);
                    });
    if (key_it == _pubkeys.end() ||
                    std::get<Crypto::Key::Serial> (*key_it) != data.r->_key_id){
        return;
    }
    auto priv = std::get<std::shared_ptr<Crypto::Key>> (*key_it);
    lock.early_unlock();


    // FIXME: choose enc/hmac depending on enc->is_authenticated()
    //  also reject non-authenticated connections (hmac = 1 && !sel_enc->is_auth()
    // select crypt/hmac/key
    const auto sel_enc = _load->choose<Crypto::Encryption> (
                                                        data._supported_crypt);
    const auto sel_hmac = _load->choose<Crypto::Hmac> (data._supported_hmac);
    const auto sel_ecc = _load->choose<Recover::ECC> (data._supported_ecc);
    const auto sel_key = _load->choose<Crypto::Key> (data._supported_key);
    const auto sel_kdf = _load->choose<Crypto::KDF> (data._supported_kdf);

    // sel_ecc *CAN* be zero => no additional ECC on the connection
    if (sel_enc == Crypto::Encryption::ID {0} ||
            sel_hmac == Crypto::Hmac::ID {0} || sel_key == Crypto::Key::ID {0}||
                                                sel_kdf == Crypto::KDF::ID {0}){
        return;
    }

    int64_t timestamp =
            std::chrono::duration_cast<std::chrono::milliseconds> (
                std::chrono::steady_clock::now().time_since_epoch()).count();

    const auto supported_auth = _load->list<Crypto::Auth>();

    const uint16_t sign_length = priv->signature_length();
    const uint16_t total_length = static_cast<uint16_t> (
                            Conn0_S_COOKIE::min_size() +
                            static_cast<uint32_t> (sizeof(Crypto::Auth::ID) *
                                                        supported_auth.size() +
                                                            2 * sign_length));
    auto answer = std::make_unique<Packet> (total_length + PKT_MINLEN);
    answer->set_header (Conn_ID{0}, 0, _rnd);
    Stream *str = answer->add_stream (pkt.stream[0].id(),Stream::Fragment::FULL,
                                        pkt.stream[0].counter(), total_length);
    auto cookie = Conn0_S_COOKIE (str->data(), sel_enc, sel_hmac, sel_ecc,
                                        sel_key, sel_kdf, timestamp,
                                        std::move(supported_auth), sign_length);
    if (!cookie)
        return;
    std::vector<uint8_t> cs_tosign = cookie.client_server_data_tosign (data);


    if (!priv->sign (cs_tosign, cookie._cs_data_signature))
        return; // can't sign for some reason
    auto s_tosign = cookie.server_data_tosign();
    if (!priv->sign (s_tosign, cookie._s_data_signature))
        return; // can't sign for some reason
    return _handler->proxy_enqueue (recv_to, recv_from,
                                    std::move(answer), Conn0_Type::S_COOKIE);
}

FENRIR_INLINE void Handshake::answer_s_cookie (const Link_ID recv_from,
                                                    const Link_ID recv_to,
                                                    const Packet &pkt,
                                                    const Conn0_S_COOKIE data)
{
    if (!data || data._supported_auth.size() == 0 ||
                                        data._cs_data_signature.size() == 0 ||
                                        data._s_data_signature.size() == 0) {
        return;
    }

    const Handshake::ID hshake_id ({pkt.stream[0].counter(),
                                                        pkt.stream[0].id()});
    Shared_Lock_Guard<Shared_Lock_Read> lock {Shared_Lock_NN{&_mtx}};
    // search previous packet in _cative handshakes
    auto it = std::lower_bound (_client_active.begin(), _client_active.end(),
                        hshake_id,
                            [] (const auto &el, const Handshake::ID id)
                                { return std::get<Handshake::ID> (el) < id; });

    if (it == _client_active.end() ||
                    std::get<Handshake::ID> (*it) != hshake_id ||
                    std::get<state_client> (*it)._type != Conn0_Type::C_INIT){
        return; // random packet. Don't answer.
    }

    // test the received data correctness based on our sent data:
    Conn0_C_INIT prev_data (std::get<state_client>(*it)._pkt->stream[0].data());
    if (!prev_data)
        return;

    if (std::find (prev_data._supported_crypt.begin(),
               prev_data._supported_crypt.end(),
               data.r->_selected_crypt) == prev_data._supported_crypt.end() ||
        std::find (prev_data._supported_hmac.begin(),
               prev_data._supported_hmac.end(),
               data.r->_selected_hmac) == prev_data._supported_hmac.end() ||
        std::find (prev_data._supported_ecc.begin(),
               prev_data._supported_ecc.end(),
               data.r->_selected_ecc) == prev_data._supported_ecc.end() ||
        std::find (prev_data._supported_key.begin(),
               prev_data._supported_key.end(),
               data.r->_selected_key) == prev_data._supported_key.end() ||
        std::find (prev_data._supported_kdf.begin(),
               prev_data._supported_kdf.end(),
               data.r->_selected_kdf) == prev_data._supported_kdf.end()) {
        return;
    }

    const auto v_cs_to_test = data.client_server_data_tosign (prev_data);
    const gsl::span<const uint8_t> span_cs_to_test (v_cs_to_test.data(),
                                    static_cast<ssize_t>(v_cs_to_test.size()));
    const auto  s_to_test = data.server_data_tosign();

    const auto key_idx = std::get<state_client> (*it)._key_idx;
    Crypto::Key *const srv_key_ptr =
                    std::get<std::shared_ptr<Crypto::Key>>
                    (std::get<state_client> (*it)._auth_servers._key[key_idx])
                                                                        .get();

    if (!srv_key_ptr->verify (span_cs_to_test, data._cs_data_signature) ||
            !srv_key_ptr->verify (      s_to_test, data._s_data_signature)) {
         return; // malicious/malformed packet. Don't answer.
    }

    // packet is verified. build answer.

    // generate priv/pubkey for key-exchange
    auto our_key = _load->get_shared<Crypto::Key> (data.r->_selected_key);
    if (our_key == nullptr || !our_key->init())
        return; // TODO: report error somehow

    const auto pub_len = our_key->get_publen();
    const auto exchange_len = our_key->get_key_data_length();

    uint16_t msg_length = static_cast<uint16_t> (
                                        Conn0_C_COOKIE::min_size() +
                                        data._s_data_signature.size_bytes() +
                                        pub_len + exchange_len);
    if (msg_length < conn_init_minlen)
        msg_length = conn_init_minlen;
    auto answer = std::make_unique<Packet> (msg_length + PKT_MINLEN);
    answer->set_header (Conn_ID{0}, 0, _rnd);

    Stream *str = answer->add_stream (pkt.stream[0].id(),Stream::Fragment::FULL,
                                        pkt.stream[0].counter(), msg_length);

    auto c_cookie = Conn0_C_COOKIE (str->data(), prev_data.r->_key_id,
                                            data.r->_selected_crypt,
                                            data.r->_selected_hmac,
                                            data.r->_selected_ecc,
                                            data.r->_selected_key,
                                            data.r->_selected_kdf,
                                            data.r->_timestamp,
                                            _rnd,
                                            static_cast<uint16_t> (
                                            data._s_data_signature.length()),
                                            pub_len, exchange_len);
    if (!c_cookie)
        return; // TODO: report error somehow
    std::copy (data._s_data_signature.begin(),
                        data._s_data_signature.end(), c_cookie._cookie.begin());
    if (!our_key->get_pubkey (c_cookie._client_pubkey) ||
                        !our_key->get_key_data (c_cookie._client_key_data)) {
        return; // TODO: report error somehow
    }

    auto raw_copy = answer->raw;
    auto copy_pkt = std::make_unique<Packet> (std::move(raw_copy));
    copy_pkt->add_stream (pkt.stream[0].id(),Stream::Fragment::FULL,
                                        pkt.stream[0].counter(), msg_length);
    auto write_lock = lock.try_upgrade();
    if (std::get<Shared_Lock_STAT> (write_lock) ==
                                    Shared_Lock_STAT::UNLOCKED_AND_RELOCKED) {
        // could not upgrade the lock
        // it was unlocked and relocked. This means that the iterator might
        // have been invalidated. get it again.
        it = std::lower_bound (_client_active.begin(), _client_active.end(),
                        hshake_id,
                            [] (const auto &el, const Handshake::ID id)
                                { return std::get<Handshake::ID> (el) < id; });

        if (it == _client_active.end() ||
                                std::get<Handshake::ID> (*it) != hshake_id ||
                    std::get<state_client> (*it)._type != Conn0_Type::C_INIT) {
            return;
        }
    }
    std::get<state_client> (*it)._type = Conn0_Type::C_COOKIE;
    std::get<state_client> (*it)._client_key = std::move (our_key);
    std::get<state_client> (*it)._pkt = std::move(copy_pkt);
    std::get<Shared_Lock_Guard<Shared_Lock_Write>> (write_lock).early_unlock();

    return _handler->proxy_enqueue (recv_to, recv_from,
                                    std::move(answer), Conn0_Type::C_COOKIE);
}

namespace {
struct FENRIR_LOCAL srv_key_format {
    std::array<uint8_t, 64> _key;
    int64_t _time;
    Conn_ID _reserved_conn;
    Crypto::Encryption::ID _enc;
    Crypto::Hmac::ID _hmac;
    Recover::ECC::ID _ecc;
    Crypto::KDF::ID _kdf;
};
} // empty namespace

FENRIR_INLINE void Handshake::answer_c_cookie (const Link_ID recv_from,
                                                    const Link_ID recv_to,
                                                    const Packet &pkt,
                                                    const Conn0_C_COOKIE data)
{
    if (pkt.raw.size() < pkt_init_minlen || !data ||
                                            data._cookie.size() == 0 ||
                                            data._client_pubkey.size() == 0) {
        return;
    }

    // check that the handshake happened in the last 5secs
    const int64_t time_now =
            std::chrono::duration_cast<std::chrono::milliseconds> (
                std::chrono::steady_clock::now().time_since_epoch()).count();
    if (data.r->_timestamp > time_now ||
                        (data.r->_timestamp + pkt_timeout.count()) < time_now) {
        // we don't do just one "if" clause to avoid overflows.
        return; // timeout or garbage
    }

    Shared_Lock_Guard<Shared_Lock_Read> lock {Shared_Lock_NN{&_mtx}};
    // time ok, check our signature
    // check key_id
    auto key_it = std::lower_bound (_pubkeys.begin(), _pubkeys.end(),
                                                            data.r->_key_id,
        [] (const auto &test, const Crypto::Key::Serial id)
            {
                return static_cast<bool> (std::get<Crypto::Key::Serial> (
                                                                    test) < id);
            });
    if (key_it == _pubkeys.end() ||
                    std::get<Crypto::Key::Serial> (*key_it) != data.r->_key_id){
        return; // no such key_id.
    }
    auto priv = std::get<std::shared_ptr<Crypto::Key>> (*key_it);

    lock.early_unlock();

    std::vector<uint8_t> test_data (Conn0_S_COOKIE::min_size(), 0);
    const Conn0_S_COOKIE test_cookie (test_data, data.r->_selected_crypt,
                                            data.r->_selected_hmac,
                                            data.r->_selected_ecc,
                                            data.r->_selected_key,
                                            data.r->_selected_kdf,
                                            data.r->_timestamp,
                                            std::vector<Crypto::Auth::ID>(), 0);

    if (!priv->verify (test_cookie.server_data_tosign(), data._cookie))
        return;

    // signature OK, build answer
    const uint16_t sign_length = priv->signature_length();

    const auto client_ephemeral =_load->get_shared<Crypto::Key> (
                                                        data.r->_selected_key);
    const auto server_ephemeral =_load->get_shared<Crypto::Key> (
                                                        data.r->_selected_key);

    if (!client_ephemeral->init (data._client_pubkey))
        return;
    if (!server_ephemeral->init())
        return;


    const uint16_t pub_len = server_ephemeral->get_publen();
    const uint16_t key_exchange_len = server_ephemeral->get_key_data_length();
    const uint16_t srv_enc_length = _srv_secret->bytes_overhead() +
                                                sizeof(struct srv_key_format);
    const uint16_t total_length = Conn0_S_KEYS::min_size() +
                                                pub_len + key_exchange_len +
                                                srv_enc_length + sign_length;

    auto answer = std::make_unique<Packet> (total_length + PKT_MINLEN);
    answer->set_header (Conn_ID{0}, 0, _rnd);

    Stream *str = answer->add_stream (pkt.stream[0].id(),Stream::Fragment::FULL,
                                        pkt.stream[0].counter(), total_length);

    auto s_keys = Conn0_S_KEYS (str->data(), data.r->_nonce, pub_len,
                                                            key_exchange_len,
                                                            srv_enc_length,
                                                            sign_length);
    struct srv_key_format *srv = reinterpret_cast<srv_key_format*> (
                        s_keys._srv_enc.data() + _srv_secret->bytes_header());

    server_ephemeral->get_pubkey (s_keys._pubkey);
    server_ephemeral->get_key_data (s_keys._key_data);
    if (!server_ephemeral->exchange_key (client_ephemeral.get(),
                                                        data._client_key_data,
                                                        s_keys._key_data,
                                                        srv->_key,
                                                        Role::Server)) {
        return;
    }

    // reserve a connection id
    uint32_t orig_id = _srv_next_tracked.load();
    auto next_free = _handler->get_next_free (Conn_ID {orig_id});
    uint32_t old_val = orig_id;
    uint32_t new_val = static_cast<uint32_t> (next_free) + 1;
    // now try to reserve "new_val - 1"
    while (!_srv_next_tracked.compare_exchange_weak (old_val, new_val)) {
        // could not exchange.
        if (old_val >= new_val) {
            if (orig_id <= new_val) {
                // other threads already reserved new_val. Try next.
                new_val = 1 + static_cast<uint32_t> (_handler->get_next_free (
                                                            Conn_ID {old_val}));
            } // else get_next(old_val) had overlowed. all ok.
        } else {
            // new_val < old_val
            if (old_val < orig_id) {
                // overflow. we can't reserve new_val. Try next.
                new_val = 1 + static_cast<uint32_t> (_handler->get_next_free (
                                                            Conn_ID {old_val}));
                orig_id = old_val;
            } // else all ok. we can still reserve new_val - 1
        }
    }
    // we have reserved "new_val - 1"
    next_free = Conn_ID {new_val - 1};


    srv->_time = time_now;
    srv->_reserved_conn = next_free;
    srv->_enc  = data.r->_selected_crypt;
    srv->_hmac = data.r->_selected_hmac;
    srv->_ecc  = data.r->_selected_ecc;
    srv->_kdf  = data.r->_selected_kdf;

    _srv_secret->encrypt (s_keys._srv_enc);

    if (!priv->sign (s_keys.data_tosign(), s_keys._sign))
        return;

    return _handler->proxy_enqueue (recv_to, recv_from,
                                        std::move(answer), Conn0_Type::S_KEYS);
}

FENRIR_INLINE void Handshake::answer_s_keys (const Link_ID recv_from,
                                                        const Link_ID recv_to,
                                                        const Packet &pkt,
                                                        const Conn0_S_KEYS data)
{
    if (!data || data._pubkey.size() == 0 || data._srv_enc.size() == 0 ||
                                                    data._sign.size() == 0) {
            return;
    }

    const Handshake::ID hshake_id ({pkt.stream[0].counter(),
                                                        pkt.stream[0].id()});
    Shared_Lock_Guard<Shared_Lock_Read> lock {Shared_Lock_NN{&_mtx}};
    // search previous packet in _client_ative handshakes
    auto it = std::lower_bound (_client_active.begin(), _client_active.end(),
                        hshake_id,
                            [] (const auto &el, const Handshake::ID id)
                                { return std::get<Handshake::ID> (el) < id; });

    if (it == _client_active.end() ||
                    std::get<Handshake::ID> (*it) != hshake_id ||
                    std::get<state_client> (*it)._type != Conn0_Type::C_COOKIE){
        return; // random packet. Don't answer.
    }

    Conn0_C_COOKIE prev_data (std::get<state_client> (*it)._pkt->stream[0].
                                                                        data());
    if (prev_data.r->_nonce != data.r->_client_nonce)
        return;

    // check signature
    auto key_idx = std::get<state_client> (*it)._key_idx;
    if (!std::get<std::shared_ptr<Crypto::Key>> (
            std::get<state_client> (*it)._auth_servers._key[key_idx])->verify (
                                                            data.data_tosign(),
                                                                data._sign)) {
        return;
    }


    Crypto::Key *const our_key = std::get<state_client> (*it)._client_key.get();
    std::array<uint8_t, 64> key;
    auto srv_ephemeral = _load->get_shared<Crypto::Key> (our_key->id());
    if (!srv_ephemeral->init (data._pubkey))
        return;
    if (!std::get<state_client> (*it)._client_key->exchange_key (
                                            srv_ephemeral.get(), data._key_data,
                                            prev_data._client_key_data, key,
                                                                Role::Client)) {
        return;
    }
    srv_ephemeral = nullptr;

    lock.early_unlock();

    auto kdf = _load->get_shared<Crypto::KDF> (prev_data.r->_selected_kdf);
    auto user_kdf = _load->get_shared<Crypto::KDF> (prev_data.r->_selected_kdf);
    if (kdf == nullptr || user_kdf == nullptr || !kdf->init (key))
        return;

    // all ok, build answer.
    constexpr std::array<char, 8> context {{ "FENRIR_" }};
    std::array<uint8_t, 64> tmp_key;

    auto enc_write = _load->get_shared<Crypto::Encryption> (
                                                prev_data.r->_selected_crypt);
    auto hmac_write = _load->get_shared<Crypto::Hmac> (
                                                prev_data.r->_selected_hmac);
    auto ecc_write = _load->get_shared<Recover::ECC> (
                                                prev_data.r->_selected_ecc);
    if (enc_write == nullptr || hmac_write == nullptr || ecc_write == nullptr)
        return;
    if (hmac_write->id() == Crypto::Hmac::ID{1} &&
                                            !enc_write->is_authenticated()) {
        return; // TODO: report eror;
    }
    kdf->get (1, context, tmp_key);
    enc_write->set_key (tmp_key);
    kdf->get (2, context, tmp_key);
    hmac_write->set_key (tmp_key);
    kdf->get (3, context, tmp_key);
    if (!ecc_write->init (tmp_key))
        return;
    auto enc_read = _load->get_shared<Crypto::Encryption> (
                                                prev_data.r->_selected_crypt);
    auto hmac_read = _load->get_shared<Crypto::Hmac> (
                                                prev_data.r->_selected_hmac);
    auto ecc_read = _load->get_shared<Recover::ECC> (
                                                prev_data.r->_selected_ecc);
    if (enc_read == nullptr || hmac_read == nullptr || ecc_read == nullptr)
        return;
    kdf->get (4, context, tmp_key);
    enc_read->set_key (tmp_key);
    kdf->get (5, context, tmp_key);
    hmac_read->set_key (tmp_key);
    kdf->get (6, context, tmp_key);
    if (!ecc_read->init (tmp_key))
        return;
    kdf->get (7, context, tmp_key);
    if (!user_kdf->init (tmp_key))
        return;

    // TODO: GET STREAMS DATA
    const uint16_t streams_num = 1;

    // FIXME: GET AUTH DATA
    std::array<uint8_t, 16> test_user {{ 'u','s','e','r','@',
                                'e','x','a','m','p','l','e','.','c','o','m' }};
    std::array<uint8_t, 32> test_token {{
                                        32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
                                        32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
                                        32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
                                        32, 32}};
    uint16_t auth_data_len = Conn0_Auth_Data::min_size() +
                            2 * test_user.size() +
                            test_token.size() +
                            Conn0_Auth_Data::stream_info_size (streams_num) +
                            enc_write->bytes_overhead() +
                            hmac_write->bytes_overhead() +
                            ecc_write->bytes_overhead();
    uint16_t total_len = static_cast<uint16_t> (Conn0_C_AUTH::min_size() +
                                                    data._srv_enc.size() +
                                                    auth_data_len);
    if (total_len < conn_init_minlen)
        total_len = conn_init_minlen;
    auto answer = std::make_unique<Packet> (total_len + PKT_MINLEN);
    answer->set_header (Conn_ID{0}, 0, _rnd);

    Stream *str = answer->add_stream (pkt.stream[0].id(),Stream::Fragment::FULL,
                                            pkt.stream[0].counter(), total_len);

    auto c_auth = Conn0_C_AUTH (str->data(), data._srv_enc, auth_data_len);

    // reserve a connection id
    uint32_t orig_id = _client_next_tracked.load();
    auto client_id = _handler->get_next_free (Conn_ID {orig_id});
    uint32_t old_val = orig_id;
    uint32_t new_val = static_cast<uint32_t> (client_id) + 1;
    // now try to reserve "new_val - 1"
    while (!_client_next_tracked.compare_exchange_weak (old_val, new_val)) {
        // could not exchange.
        if (old_val >= new_val) {
            if (orig_id <= new_val) {
                // other threads already reserved new_val. Try next.
                new_val = 1 + static_cast<uint32_t> (_handler->get_next_free (
                                                            Conn_ID {old_val}));
            } // else get_next(old_val) had overlowed. all ok.
        } else {
            // new_val < old_val
            if (old_val < orig_id) {
                // overflow. we can't reserve new_val. Try next.
                new_val = 1 + static_cast<uint32_t> (_handler->get_next_free (
                                                            Conn_ID {old_val}));
                orig_id = old_val;
            } // else all ok. we can still reserve new_val - 1
        }
    }
    // we have reserved "new_val - 1"
    client_id = Conn_ID {new_val - 1};

    // FIXME: get the device id /service id/usernames raw_auth_data somewhere
    Device_ID test_dev_id   {{{ 4,4,4,4,4,4,4,4,
                                4,4,4,4,4,4,4,4 }}};
    Service_ID test_service {{{ 42,42,42,42,42,42,42,42,
                                42,42,42,42,42,42,42,42 }}};
    Username client_auth_u { test_user };
    Username client_service_u { test_user };
    auto cleartext_data = c_auth.as_cleartext (enc_write->bytes_header() +
                                                    hmac_write->bytes_header() +
                                                    ecc_write->bytes_header(),
                                                    enc_write->bytes_footer() +
                                                    hmac_write->bytes_footer() +
                                                    ecc_write->bytes_footer());
    assert ((auth_data_len - (enc_write->bytes_overhead() +
                                    hmac_write->bytes_overhead() +
                                    ecc_write->bytes_overhead()))
                                        == cleartext_data.size() &&
                                            "Fenrir: Hshake c_auth size error");
    auto auth_data = Conn0_Auth_Data (cleartext_data,
                                        test_dev_id,
                                        test_service,
                                        client_auth_u,
                                        client_service_u,
                                        Conn_ID {client_id},
                                        _rnd->uniform<uint8_t> (0, 7), // TODO: more random?
                                        Packet::Alignment_Flag::UINT8,
                                        Crypto::Auth::ID {1}, // FIXME: generalize
                                        Lattice::TOP, // FIXME: generalize
                                        Stream_ID {_rnd->uniform<uint16_t>()},
                                        test_token,
                                        streams_num);
    if (!auth_data)
        return; // TODO: report error
    // TODO: more streams.
    auth_data._streams[0]._id = Stream_ID {_rnd->uniform<uint16_t>()};
    auth_data._streams[0]._counter_start = Counter {_rnd->uniform<uint32_t> (0,
                                        static_cast<uint32_t> (max_counter))};
    auth_data._streams[0]._type = Storage_t::COMPLETE | Storage_t::ORDERED  |
                                                        Storage_t::RELIABLE |
                                                        Storage_t::UNICAST;
    auth_data._streams[0]._priority = Stream_PRIO {0}; // TODO: priority

    // add the CLEARTEXT packet (copy) to the tracking system
    auto copy_raw = answer->raw;
    auto copy_pkt = std::make_unique<Packet> (std::move(copy_raw));
    copy_pkt->add_stream (pkt.stream[0].id(),Stream::Fragment::FULL,
                                            pkt.stream[0].counter(), total_len);

    enc_write->encrypt   (c_auth.as_cleartext (hmac_write->bytes_header() +
                                                    ecc_write->bytes_header(),
                                                    hmac_write->bytes_footer() +
                                                    ecc_write->bytes_footer()));
    hmac_write->add_hmac (c_auth.as_cleartext (ecc_write->bytes_header(),
                                                    ecc_write->bytes_footer()));
    ecc_write->add_ecc   (c_auth._enc_data);
    Shared_Lock_Guard<Shared_Lock_Write> w_lock {Shared_Lock_NN{&_mtx}};
    // search previous packet in _active handshakes
    auto pkt_it = std::lower_bound (_client_active.begin(),_client_active.end(),
                        hshake_id,
                            [] (const auto &el, const Handshake::ID id)
                                { return std::get<Handshake::ID> (el) < id; });

    if (pkt_it == _client_active.end() ||
                std::get<Handshake::ID> (*pkt_it) != hshake_id ||
                std::get<state_client> (*pkt_it)._type != Conn0_Type::C_COOKIE){
        // did we lose the hadshake?
        // can happen if the server sends the S_KEYS packet twice to avoid
        // trasnmission errors, and we reeive both. This means we already
        // answered one. Don't answer again.
        return;
    }

    std::get<state_client> (*pkt_it)._type = Conn0_Type::C_AUTH;
    std::get<state_client> (*pkt_it)._pkt = std::move(copy_pkt);
    std::get<state_client> (*pkt_it)._enc_read = std::move(enc_read);
    std::get<state_client> (*pkt_it)._hmac_read = std::move(hmac_read);
    std::get<state_client> (*pkt_it)._ecc_read = std::move(ecc_read);
    std::get<state_client> (*pkt_it)._enc_write = std::move(enc_write);
    std::get<state_client> (*pkt_it)._hmac_write = std::move(hmac_write);
    std::get<state_client> (*pkt_it)._ecc_write = std::move(ecc_write);
    std::get<state_client> (*pkt_it)._user_kdf = std::move(user_kdf);
    std::get<state_client> (*pkt_it)._client_key = nullptr;
    w_lock.early_unlock();


    // test the received data correctness based on our sent data:
    return _handler->proxy_enqueue (recv_to, recv_from,
                                        std::move(answer), Conn0_Type::C_AUTH);
}

FENRIR_INLINE void Handshake::answer_c_auth (const Link_ID recv_from,
                                                        const Link_ID recv_to,
                                                        const Packet &pkt,
                                                        const Conn0_C_AUTH data)
{
    if (pkt.raw.size() < pkt_init_minlen || !data ||
                        data._enc_data.size() <  Conn0_Auth_Data::min_size() ||
                        static_cast<size_t> (data._srv_key.size()) !=
                                        (_srv_secret->bytes_overhead() +
                                            sizeof(struct srv_key_format))) {
        return;
    }

    // authenticate/decrypt the _srv_key
    std::vector<uint8_t> dec_srv_key_v (sizeof(srv_key_format) +
                                            _srv_secret->bytes_overhead(), 0);
    gsl::span<uint8_t> dec_srv_key (dec_srv_key_v);
    if (_srv_secret->decrypt (data._srv_key, dec_srv_key) != Impl::Error::NONE){
        return;
    }
    auto srv = reinterpret_cast<struct srv_key_format*> (dec_srv_key.data());
    const int64_t time_now =
            std::chrono::duration_cast<std::chrono::milliseconds> (
                std::chrono::steady_clock::now().time_since_epoch()).count();
    if (srv->_time > time_now || (srv->_time + pkt_timeout.count()) < time_now)
        return;

    if (!_load->has<Crypto::Encryption> (srv->_enc) ||
                                    !_load->has<Crypto::Hmac> (srv->_hmac)) {
        return;
    }

    //The next check is not definitive, try again when allocating the connection
    if (_handler->get_connection (srv->_reserved_conn) != nullptr)
        return;

    auto kdf = _load->get_shared<Crypto::KDF> (srv->_kdf);
    auto user_kdf = _load->get_shared<Crypto::KDF> (srv->_kdf);
    if (kdf == nullptr || user_kdf == nullptr || !kdf->init (srv->_key))
        return;

    constexpr std::array<char, 8> context {{ "FENRIR_" }};
    std::array<uint8_t, 64> tmp_key;

    // load encryption keys
    auto enc_read = _load->get_shared<Crypto::Encryption> (srv->_enc);
    auto hmac_read = _load->get_shared<Crypto::Hmac> (srv->_hmac);
    auto ecc_read = _load->get_shared<Recover::ECC> (srv->_ecc);
    if (enc_read == nullptr || hmac_read == nullptr || ecc_read == nullptr)
        return;
    if (hmac_read->id() == Crypto::Hmac::ID{1} && !enc_read->is_authenticated())
        return;
    kdf->get (1, context, tmp_key);
    enc_read->set_key (tmp_key);
    kdf->get (2, context, tmp_key);
    hmac_read->set_key (tmp_key);
    kdf->get (3, context, tmp_key);
    if (!ecc_read->init (tmp_key))
        return;


    // test autenticity & decrypt
    gsl::span<uint8_t> decrypted_data {data._enc_data};
    if (ecc_read->correct (decrypted_data, decrypted_data) ==
                                                    Recover::ECC::Result::ERR) {
        return;
    }
    if (!hmac_read->is_valid (decrypted_data, decrypted_data))
        return;
    if (enc_read->decrypt (decrypted_data, decrypted_data) != Impl::Error::NONE)
        return;

    Conn0_Auth_Data client_auth_data (decrypted_data);

    if (!client_auth_data || client_auth_data._auth_username.size() <= 3 ||
                        client_auth_data._service_username.size() <= 3 ||
                        client_auth_data._auth_data.size() == 0 ||
                        client_auth_data._streams.size() == 0 ||
                        client_auth_data.r->_client_conn_id < Conn_Reserved) {
        return;
    }

    Shared_Lock_Guard<Shared_Lock_Read> readlock {Shared_Lock_NN{&_mtx_auths}};
    auto auth_it = std::lower_bound (auths.begin(), auths.end(),
                                    client_auth_data.r->_selected_auth,
                                        [] (const auto &it, Crypto::Auth::ID id)
                                            {return it->id() < id; });
    if (auth_it == auths.end())
        return;
    auto auth = *auth_it;
    readlock.early_unlock();

    Username client_auth_user { client_auth_data._auth_username };
    Username client_service_user { client_auth_data._service_username };
    if (!client_auth_user || !client_service_user)
        return;
    auto sh_lattice = _handler->search_lattice (client_auth_data.r->_service,
                                                    client_service_user.domain);
    auto auth_res = auth->authenticate (client_auth_data.r->_dev_id,
                                            client_auth_data.r->_service,
                                            std::move(sh_lattice),
                                            client_auth_data.r->_lattice_node,
                                            client_auth_user,
                                            client_service_user,
                                            client_auth_data._auth_data, _db);

    const Stream_ID srv_control_stream {_rnd->uniform<uint16_t>()};
    const Counter control_stream_start {_rnd->uniform<uint32_t> (0,
                                        static_cast<uint32_t> (max_counter))};
    const uint8_t srv_max_padding = _rnd->uniform<uint8_t> (0, 16);
    const auto srv_alignment = Packet::Alignment_Flag::UINT8;
    std::shared_ptr<Crypto::Encryption> enc_write;
    std::shared_ptr<Crypto::Hmac> hmac_write;
    std::shared_ptr<Recover::ECC> ecc_write;
    if (!auth_res._failed) {
        // TODO: Contact service, ask for the connection ID, XORed keys
        // and whatnot

        // allocate connection
        enc_write = _load->get_shared<Crypto::Encryption> (srv->_enc);
        hmac_write = _load->get_shared<Crypto::Hmac> (srv->_hmac);
        ecc_write = _load->get_shared<Recover::ECC> (srv->_ecc);
        if (enc_write == nullptr || hmac_write == nullptr ||
                                                        ecc_write == nullptr) {
            return;
        }
        kdf->get (4, context, tmp_key);
        enc_write->set_key (tmp_key);
        kdf->get (5, context, tmp_key);
        hmac_write->set_key (tmp_key);
        kdf->get (6, context, tmp_key);
        if (!ecc_write->init (tmp_key))
            return;
        kdf->get (7, context, tmp_key);
        if (!user_kdf->init (tmp_key))
            return;


        auto conn = Connection::mk_shared (Role::Server,
                                            auth_res._user_id,
                                            _loop,
                                            _handler,
                                            client_auth_data.r->_control_stream,
                                            srv_control_stream,
                                            client_auth_data.r->_client_conn_id,
                                            srv->_reserved_conn,
                                            control_stream_start,
                                            Packet::Flag_To_Byte (
                                                client_auth_data.r->_alignment),
                                            Packet::Flag_To_Byte(srv_alignment),
                                            client_auth_data.r->_max_padding,
                                            srv_max_padding,
                                            enc_write, hmac_write, ecc_write,
                                            enc_read,  hmac_read,  ecc_read,
                                            user_kdf);
        conn->add_Link_out (recv_from);

        if (_handler->add_connection (std::move(conn)) != Error::NONE)
            return; // there already is such a connection.
    }

    // build answer
    const uint16_t answer_length = Conn0_Auth_Result::min_size() +
                                Conn0_Auth_Result::stream_info_size (1) +
                                enc_read->bytes_overhead() + hmac_read->bytes_overhead();

    auto answer = std::make_unique<Packet> (Conn0_S_RESULT::min_size() +
                                                    answer_length + PKT_MINLEN);
    answer->set_header (Conn_ID{0}, 0, _rnd);
    Stream *str = answer->add_stream (pkt.stream[0].id(),Stream::Fragment::FULL,
                                                    pkt.stream[0].counter(),
                                                    Conn0_S_RESULT::min_size()
                                                            + answer_length);
    Conn0_S_RESULT srv_res (str->data(), answer_length);
    Conn0_Auth_Result srv_data (srv_res.as_cleartext (
                                                    enc_write->bytes_header() +
                                                    hmac_write->bytes_header() +
                                                    ecc_write->bytes_header(),
                                                    enc_write->bytes_footer() +
                                                    hmac_write->bytes_footer() +
                                                    ecc_write->bytes_footer()),
                                srv->_reserved_conn,
                                control_stream_start,
                                srv_control_stream,
                                srv_alignment,
                                srv_max_padding,
                                1); // TODO: moar streamz
    srv_data._streams[0]._id = Stream_ID {_rnd->uniform<uint16_t>()};
    srv_data._streams[0]._counter_start = Counter {_rnd->uniform<uint32_t> (0,
                                        static_cast<uint32_t> (max_counter))};
    srv_data._streams[0]._type = Storage_t::COMPLETE | Storage_t::ORDERED  |
                                                        Storage_t::RELIABLE |
                                                        Storage_t::UNICAST;
    srv_data._streams[0]._priority = Stream_PRIO {0}; // TODO: priority

    if (auth_res._failed) {
        // Connection 0 is reserved, we can use this to give back any
        // kind of failure. We don't want to give back any
        // "no such user", "wrong pass" or internal error anyway.
        srv_data.w->_conn_id = Conn_ID{0};
    }


    if (enc_write->encrypt (srv_res.as_cleartext (hmac_write->bytes_header() +
                                                    ecc_write->bytes_header(),
                                                  hmac_write->bytes_footer() +
                                                    ecc_write->bytes_footer()))
                                                                != Error::NONE){
        return; // TODO: what to do now? drop earlier connection?
    }
    if (hmac_write->add_hmac (srv_res.as_cleartext (ecc_write->bytes_header(),
                                                    ecc_write->bytes_footer()))
                                                                != Error::NONE){
        return;
    }
    if (ecc_write->add_ecc (srv_res._enc) != Error::NONE)
        return;

    return _handler->proxy_enqueue (recv_to, recv_from,
                                    std::move(answer), Conn0_Type::S_RESULT);
}

FENRIR_INLINE void Handshake::parse_s_result (const Link_ID recv_from,
                                                        const Link_ID recv_to,
                                                        const Packet &pkt,
                                                        Conn0_S_RESULT data)
{
    FENRIR_UNUSED (recv_from);
    FENRIR_UNUSED (recv_to);
    if (!data || data._enc.size() < Conn0_Auth_Result::min_size()) {
        return;
    }

    const Handshake::ID hshake_id ({pkt.stream[0].counter(),
                                                        pkt.stream[0].id()});

    Shared_Lock_Guard<Shared_Lock_Read> r_lock {Shared_Lock_NN{&_mtx}};
    // search previous packet in _client_ative handshakes
    auto it = std::lower_bound (_client_active.begin(), _client_active.end(),
                        hshake_id,
                            [] (const auto &el, const Handshake::ID id)
                                { return std::get<Handshake::ID> (el) < id; });

    if (it == _client_active.end() ||
                    std::get<Handshake::ID> (*it) != hshake_id ||
                    std::get<state_client> (*it)._type != Conn0_Type::C_AUTH) {
        return; // random packet. Don't answer.
    }

    auto enc_read   = std::get<state_client> (*it)._enc_read;
    auto hmac_read  = std::get<state_client> (*it)._hmac_read;
    auto ecc_read   = std::get<state_client> (*it)._ecc_read;
    auto enc_write  = std::get<state_client> (*it)._enc_write;
    auto hmac_write = std::get<state_client> (*it)._hmac_write;
    auto ecc_write  = std::get<state_client> (*it)._ecc_write;
    auto user_kdf   = std::get<state_client> (*it)._user_kdf;

    // save data for later, so we don't lock things too much
    Conn0_C_AUTH prev_data (std::get<state_client>(*it)._pkt->stream[0].data());
    r_lock.early_unlock();

    auto cleartext_client_auth = Conn0_Auth_Data (prev_data.as_cleartext (
                                                enc_read->bytes_header() +
                                                    hmac_read->bytes_header() +
                                                    ecc_read->bytes_header(),
                                                enc_read->bytes_footer() +
                                                    hmac_read->bytes_footer() +
                                                    ecc_read->bytes_footer()));

    auto client_alignment = cleartext_client_auth.r->_alignment;
    auto client_Conn_ID = cleartext_client_auth.r->_client_conn_id;
    auto client_control_stream = cleartext_client_auth.r->_control_stream;
    auto client_max_pading = cleartext_client_auth.r->_max_padding;

    // test hmac
    gsl::span<uint8_t> decrypted_data = data._enc;
    if (ecc_read->correct (decrypted_data, decrypted_data) ==
                                                    Recover::ECC::Result::ERR) {
        return;
    }
    if (!hmac_read->is_valid (decrypted_data, decrypted_data))
        return; // trasnmission corruption or malicious packet
    // decrypt
    if (enc_read->decrypt (decrypted_data, decrypted_data) != Error::NONE)
        return; // trasnmission corruption or malicious packet

    Shared_Lock_Guard<Shared_Lock_Write> w_lock {Shared_Lock_NN{&_mtx}};
    // Get the iterator again, it might have changed
    it = std::lower_bound (_client_active.begin(), _client_active.end(),
                    hshake_id,
                        [] (const auto &el, const Handshake::ID id)
                            { return std::get<Handshake::ID> (el) < id; });

    if (it == _client_active.end() ||
                std::get<Handshake::ID> (*it) != hshake_id ||
                std::get<state_client> (*it)._type != Conn0_Type::C_AUTH) {
        // if .end() then we got two very close packets and two threads
        // were racing to add the connection
        return; // random packet. Don't answer.
    }
    _client_active.erase (it);
    w_lock.early_unlock();

    auto srv_clear_data = Conn0_Auth_Result (decrypted_data);
    if (srv_clear_data.r->_conn_id < Conn_Reserved)
        return; // FIXME: report auth failed

    // Auth OK, allocate connection
    auto conn = Connection::mk_shared (Role::Client,
                                        User_ID {0},
                                        _loop,
                                        _handler,
                                        client_control_stream,
                                        srv_clear_data.r->_control_stream,
                                        client_Conn_ID,
                                        srv_clear_data.r->_conn_id,
                                        srv_clear_data.r->_data_counter_start,
                                        Packet::Flag_To_Byte (client_alignment),
                                        Packet::Flag_To_Byte (
                                                srv_clear_data.r->_alignment),
                                        client_max_pading,
                                        srv_clear_data.r->_max_padding,
                                        std::move(enc_write),
                                        std::move(hmac_write),
                                        std::move(ecc_write),
                                        std::move(enc_read),
                                        std::move(hmac_read),
                                        std::move(ecc_read),
                                        std::move(user_kdf)
                                        );

    if (conn == nullptr) {
        // could not allocate?
        // Horror and panic, forget it all.
        // FIXME: report auth failed
        return;
    }
    conn->add_Link_in (recv_from);

    if (_handler->add_connection (std::move(conn)) != Error::NONE) {
        // There already is such a connection. Duplicated packet?
        // but why caould we get the enc/hmac ??
        // Horror and panic, forget it all.
        // FIXME: report auth failed
        return;
    }

    // TODO: report succesful connection
}

} // namespace Impl
} // namespace Fenrir__v1
