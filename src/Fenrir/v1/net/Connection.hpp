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
#include "Fenrir/v1/data/control/Control.hpp"
#include "Fenrir/v1/recover/Error_Correction.hpp"
#include "Fenrir/v1/data/IP.hpp"
#include "Fenrir/v1/data/packet/Packet.hpp"
#include "Fenrir/v1/data/Storage.hpp"
#include "Fenrir/v1/data/Username.hpp"
#include "Fenrir/v1/net/Link.hpp"
#include "Fenrir/v1/net/Role.hpp"
#include "Fenrir/v1/util/Random.hpp"
#include <map>
#include <memory>
#include <type_safe/strong_typedef.hpp>
#include <type_safe/optional.hpp>
#include <utility>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {
class Handler;
namespace Event {
class Loop;
}
namespace Crypto {
class Encryption;
class Hmac;
class KDF;
}
namespace Recover {
class ECC;
}

class FENRIR_LOCAL user_data {
public:
    const Stream_ID _id;
    using data = std::tuple<Counter, Stream::Fragment, std::vector<uint8_t>>;
    std::vector<data> _messages;

    user_data (const Stream_ID id) : _id (id) {}
    user_data (const Stream_ID id, std::vector<data> &&msg)
        : _id (id), _messages (std::move (msg)) {}
};


// This represents a full connection, not half-enstablished states.
class FENRIR_LOCAL Connection
{
public:

    template<typename... Args>
    static std::shared_ptr<Connection> mk_shared (Args &&... args)
    {
        class make_shared_enabler final : public Connection
        {
        public:
            make_shared_enabler(Args &&... sh_args):Connection (
                                            std::forward<Args>(sh_args)...){}
        };
        auto ret = std::make_shared<make_shared_enabler> (
                                                std::forward<Args>(args)...);
        ret->_ourselves = ret;
        return ret;
    }

    Connection() = delete;
    Connection (const Connection&) = delete;
    Connection& operator= (const Connection&) = delete;
    Connection (Connection &&) = default;
    Connection& operator= (Connection &&) = default;
    ~Connection() = default;

    std::vector<Link> _incoming;
    std::vector<Link> _outgoing;
    Conn_ID _read_connection_id, _write_connection_id;
    User_ID _user_id;
    // the _unrel_ streams are initialized as "_rel_ + 1"
    const Stream_ID _rel_read_control_stream, _unrel_read_control_stream;
    const Stream_ID _rel_write_control_stream, _unrel_write_control_stream;
    const Packet::Alignment_Byte _read_al, _write_al;
    uint8_t _max_read_padding, _max_write_padding;
    const Role _role;

    std::pair<Impl::Error, Stream_ID> add_stream_out (const Storage_t s,
                            const type_safe::optional<Stream_ID> linked_with);
    Impl::Error add_stream_in (const Stream_ID id, const Storage_t s,
                            const Counter window_start,
                            const type_safe::optional<Stream_ID> linked_with);
    Error del_stream_out (const Stream_ID id);
    Error del_stream_in  (const Stream_ID id);
    void recv (Packet &pkt);
    std::vector<user_data> get_data();
    // add source and queue activation pkt
    // returns the activation vector (if the link need sto be activated)
    std::unique_ptr<Packet> update_source (const Link_ID from);
    Error add_Link_in  (const Link_ID id);
    Error add_Link_out (const Link_ID id);
    void missed_keepalive_in (const Link_ID id, const uint8_t max_fail);
    type_safe::optional<std::tuple<Link_ID,
            std::unique_ptr<Packet>>> missed_keepalive_out (const Link_ID id);

    void update_destination (const Link_ID to);
    uint32_t total_overhead() const;
    uint32_t mtu (const Link_ID from, const Link_ID to) const;
    Error add_data (Packet_NN pkt, const uint32_t mtu);
    Error add_security (Packet_NN pkt);
private:
    class FENRIR_LOCAL Stream_Track_In
    {
    public:
        Stream_Track_In (const Stream_ID id, const Storage_t s,
                                                const Counter window_start,
                                                std::shared_ptr<Storage> str);
        Stream_Track_In() = delete;
        Stream_Track_In (const Stream_Track_In&) = delete;
        Stream_Track_In& operator= (const Stream_Track_In&) = delete;
        Stream_Track_In (Stream_Track_In &&) = default;
        Stream_Track_In& operator= (Stream_Track_In &&) = default;
        ~Stream_Track_In() = default;

        // FEC can be implemented as storage.
        // can be shared with mutiple streams.
        // note that you can share the same storage between input and output
        // streams. Can't think of a reason yet, but I'm sure you'll find one.
        std::shared_ptr<Storage> _received;
        uint64_t _bytes_received;
    };
    class FENRIR_LOCAL Stream_Track_Out
    {
    public:
        Stream_Track_Out (const Stream_ID id, const Storage_t s, Random *rnd,
                                                std::shared_ptr<Storage> str);
        Stream_Track_Out() = delete;
        Stream_Track_Out (const Stream_Track_Out&) = delete;
        Stream_Track_Out& operator= (const Stream_Track_Out&) = delete;
        Stream_Track_Out (Stream_Track_Out&&) = default;
        Stream_Track_Out& operator= (Stream_Track_Out&&) = default;
        ~Stream_Track_Out() = default;

        std::shared_ptr<Storage> _sent;
        uint64_t _bytes_sent;
    };
    std::weak_ptr<Connection> _ourselves;
    Random _rnd;
    Event::Loop *const _loop;
    Handler *const _handler;

    // TODO: split mutex in multiple shared_locks.
    std::mutex _mtx;
    std::vector<std::pair<Stream_ID, Stream_Track_In>>  _streams_in;
    std::vector<std::pair<Stream_ID, Stream_Track_Out>> _streams_out;
    Stream_ID _last_out;

    std::shared_ptr<Crypto::Encryption> _enc_send;
    std::shared_ptr<Crypto::Hmac> _hmac_send;
    std::shared_ptr<Recover::ECC> _ecc_send;
    std::shared_ptr<Crypto::Encryption> _enc_recv;
    std::shared_ptr<Crypto::Hmac> _hmac_recv;
    std::shared_ptr<Recover::ECC> _ecc_recv;
    std::shared_ptr<Crypto::KDF> _user_kdf;

    Connection (const Role role, const User_ID user,
                                Event::Loop *const loop,
                                Handler *const _handler,
                                const Stream_ID read_control_stream,
                                const Stream_ID write_control_stream,
                                const Conn_ID read, const Conn_ID write,
                                const Counter control_window_start,
                                const Packet::Alignment_Byte read_al,
                                const Packet::Alignment_Byte write_al,
                                const uint8_t max_read_padding,
                                const uint8_t max_write_padding,
                                std::shared_ptr<Crypto::Encryption> enc_send,
                                std::shared_ptr<Crypto::Hmac> hmac_send,
                                std::shared_ptr<Recover::ECC> ecc_send,
                                std::shared_ptr<Crypto::Encryption> enc_recv,
                                std::shared_ptr<Crypto::Hmac> hmac_recv,
                                std::shared_ptr<Recover::ECC> ecc_recv,
                                std::shared_ptr<Crypto::KDF> user_kdf);
    void parse_rel_control();
    void parse_unrel_control();
    void parse_control (const std::vector<uint8_t> &data);
    void parse_control (const
            Control::Link_Activation_CLi<Control::Access::READ_ONLY> &&data);
    void parse_control (const
            Control::Link_Activation_Srv<Control::Access::READ_ONLY> &&data);
};

} // namespace Impl
} // namespace Fenrir__v1

#ifdef FENRIR_HEADER_ONLY
#include "Fenrir/v1/net/Connection.ipp"
#include "Fenrir/v1/net/Connection_Control.ipp"
#endif

