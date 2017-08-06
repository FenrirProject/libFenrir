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
#include "Fenrir/v1/auth/Auth.hpp"
#include "Fenrir/v1/crypto/Crypto.hpp"
#include "Fenrir/v1/data/Conn0_Type.hpp"
#include "Fenrir/v1/data/Device_ID.hpp"
#include "Fenrir/v1/data/packet/Packet.hpp"
#include "Fenrir/v1/data/Storage_t.hpp"
#include "Fenrir/v1/service/Service_ID.hpp"
#include "Fenrir/v1/util/Random.hpp"
#include "Fenrir/v1/util/span_overlay.hpp"
#include <gsl/span>
#include <type_safe/optional.hpp>
#include <type_safe/strong_typedef.hpp>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {


constexpr uint32_t min_pkt_len = 1200; // bytes

// Strong typedef for identifiers,to avoid up/down casting, and other
// operations which do not make sense for an identifier.
struct FENRIR_LOCAL Fenrir_Version :
            type_safe::strong_typedef<Fenrir_Version, uint8_t>,
            type_safe::strong_typedef_op::equality_comparison<Fenrir_Version>
    { using strong_typedef::strong_typedef; };

struct FENRIR_LOCAL  Hshake_srv_Track :
        type_safe::strong_typedef<Hshake_srv_Track, uint32_t>,
        type_safe::strong_typedef_op::equality_comparison<Hshake_srv_Track>,
        type_safe::strong_typedef_op::relational_comparison<Hshake_srv_Track>
    { using strong_typedef::strong_typedef; };

// Derive this class for every message.
// the fenrir_version is put at 0 on error, so check it *every*time*
// the default fenrir_version is 1
// FIXME: we do not list the supported fenrir versions on both sides.
// before that. is the version evennecessary?
// FIXME: list KDF, too

// FIXME: make sure everything is little endian

class FENRIR_LOCAL Conn0
{
public:
    gsl::span<uint8_t> _raw;
    struct data {
        Fenrir_Version _version; // == 0 on error
        Conn0_Type _type;
    };
    struct data const *const r;
    struct data *const w;

    Conn0 (gsl::span<uint8_t> raw, const Conn0_Type t,
                                                const Fenrir_Version version);
    Conn0() = delete;
    Conn0 (const Conn0&) = default;
    Conn0& operator= (const Conn0&) = default;
    Conn0 (Conn0 &&) = default;
    Conn0& operator= (Conn0 &&) = default;
    ~Conn0() = default;

    explicit operator bool() const;
    size_t size() const
        { return static_cast<size_t> (_raw.size()); }
    static constexpr uint16_t min_size();
    static type_safe::optional<Conn0_Type> get_type (
                                                const gsl::span<uint8_t> raw);
protected:
    static constexpr uint16_t Conn0_offset = sizeof(struct data);
};

///////////////
// Conn0_C_INIT
///////////////

// Strong typedef for identifiers,to avoid up/down casting, and other
// operations which do not make sense for an identifier.
struct FENRIR_LOCAL Nonce :
                    type_safe::strong_typedef<Nonce, std::array<uint8_t, 16>>,
                    type_safe::strong_typedef_op::equality_comparison<Nonce>
    { using strong_typedef::strong_typedef; };

class FENRIR_LOCAL Conn0_C_INIT final : public Conn0
{
public:
    struct data {
        Fenrir_Version _version; // == 0 on error
        Conn0_Type _type;
        Crypto::Key::Serial _key_id;
        Nonce _nonce;
    };
    struct data const *const r;
    struct data *const w;
    Span_Overlay<Crypto::Encryption::ID> _supported_crypt;
    Span_Overlay<Crypto::Hmac::ID> _supported_hmac;
    Span_Overlay<Crypto::Key::ID> _supported_key;

    Conn0_C_INIT() = delete;
    Conn0_C_INIT (const Conn0_C_INIT&) = default;
    Conn0_C_INIT& operator= (const Conn0_C_INIT&) = default;
    Conn0_C_INIT (Conn0_C_INIT &&) = default;
    Conn0_C_INIT& operator= (Conn0_C_INIT &&) = default;
    ~Conn0_C_INIT() = default;

    Conn0_C_INIT (const gsl::span<uint8_t> raw);  // received pkt
    Conn0_C_INIT (gsl::span<uint8_t> raw,         // sent pkt
                    Random *const rnd, const Crypto::Key::Serial key_id,
                    const std::vector<Crypto::Encryption::ID>&supported_crypt,
                    const std::vector<Crypto::Hmac::ID> &supported_hmac,
                    const std::vector<Crypto::Key::ID> &supported_key);
    static constexpr uint16_t min_size();
private:
    static constexpr uint16_t supported_offset = sizeof(struct data);
    static constexpr uint16_t min_data_len = supported_offset +
                                                        3 * sizeof(uint16_t);
};


/////////////////
// Conn0_S_COOKIE
/////////////////

class FENRIR_LOCAL Conn0_S_COOKIE final : public Conn0
{
public:
    struct data {
        Fenrir_Version _version; // == 0 on error
        Conn0_Type _type;
        Crypto::Encryption::ID _selected_crypt;
        Crypto::Hmac::ID _selected_hmac;
        Crypto::Key::ID _selected_key;
        int64_t _timestamp;
    };
    struct data const *const r;
    struct data *const w;
    Span_Overlay<Crypto::Auth::ID> _supported_auth;
    Span_Overlay<uint8_t> _cs_data_signature;
    Span_Overlay<uint8_t> _s_data_signature;

    Conn0_S_COOKIE() = delete;
    Conn0_S_COOKIE (const Conn0_S_COOKIE&) = default;
    Conn0_S_COOKIE& operator= (const Conn0_S_COOKIE&) = default;
    Conn0_S_COOKIE (Conn0_S_COOKIE &&) = default;
    Conn0_S_COOKIE& operator= (Conn0_S_COOKIE &&) = default;
    ~Conn0_S_COOKIE() = default;

    Conn0_S_COOKIE (const gsl::span<uint8_t> raw);    // received pkt
    Conn0_S_COOKIE (      gsl::span<uint8_t> raw,     // send pkt
                            const Crypto::Encryption::ID selected_crypt,
                            const Crypto::Hmac::ID selected_hmac,
                            const Crypto::Key::ID selected_key,
                            const int64_t timestamp,
                            const std::vector<Crypto::Auth::ID>&supported_auth,
                            const uint16_t signature_length);
    static constexpr uint16_t min_size();
    const std::vector<uint8_t> client_server_data_tosign (
                                                const Conn0_C_INIT &req) const;
    const gsl::span<uint8_t> server_data_tosign() const;
private:
    static constexpr uint16_t supported_offset = sizeof(struct data);
    static constexpr uint16_t min_data_len = supported_offset +
                                                        3 * sizeof(uint16_t);
};


/////////////////
// Conn0_C_COOKIE
/////////////////

class FENRIR_LOCAL Conn0_C_COOKIE final : public Conn0
{
public:
    struct data {
        Fenrir_Version _version; // == 0 on error
        Conn0_Type _type;
        Crypto::Key::Serial _key_id;
        Crypto::Encryption::ID _selected_crypt;
        Crypto::Hmac::ID _selected_hmac;
        Crypto::Key::ID _selected_key;
        int64_t _timestamp;
        Nonce _nonce;
    };
    struct data const *const r;
    struct data *const w;
    Span_Overlay<uint8_t> _cookie;
    Span_Overlay<uint8_t> _client_pubkey;
    Span_Overlay<uint8_t> _client_key_data;

    Conn0_C_COOKIE() = delete;
    Conn0_C_COOKIE (const Conn0_C_COOKIE&) = default;
    Conn0_C_COOKIE& operator= (const Conn0_C_COOKIE&) = default;
    Conn0_C_COOKIE (Conn0_C_COOKIE &&) = default;
    Conn0_C_COOKIE& operator= (Conn0_C_COOKIE &&) = default;
    ~Conn0_C_COOKIE() = default;
    Conn0_C_COOKIE (const gsl::span<uint8_t> raw);    // received pkt
    Conn0_C_COOKIE (      gsl::span<uint8_t> raw,     // send pkt
                                    const Crypto::Key::Serial pubkey_id,
                                    const Crypto::Encryption::ID selected_crypt,
                                    const Crypto::Hmac::ID selected_hmac,
                                    const Crypto::Key::ID selected_key,
                                    const int64_t timestamp,
                                    Random *const rnd, // for the Nonce
                                    const uint16_t cookie_len,
                                    const uint16_t client_pubkey_len,
                                    const uint16_t client_key_data_len);
    static constexpr uint16_t min_size();
private:
    static constexpr uint16_t cookie_offset = sizeof(struct data);
    static constexpr uint16_t min_data_len = cookie_offset +
                                                        3 * sizeof(uint16_t);
};

///////////////
// Conn0_S_KEYS
///////////////

class FENRIR_LOCAL Conn0_S_KEYS final : public Conn0
{
public:
    struct data {
        Fenrir_Version _version; // == 0 on error
        Conn0_Type _type;
        Nonce _client_nonce;
    };
    struct data const *const r;
    struct data *const w;
    Span_Overlay<uint8_t> _pubkey;
    Span_Overlay<uint8_t> _key_data;
    Span_Overlay<uint8_t> _srv_enc;
    // srv_enc cleartext: (78 bytes + overhead)
    //   ?? overhead for encryption
    //   64 bytes: key
    //   Crypto::Encryption::ID
    //   Crypto::Hmac::ID
    //   int64_t (time)
    //   Conn_ID
    Span_Overlay<uint8_t> _sign;

    Conn0_S_KEYS() = delete;
    Conn0_S_KEYS (const Conn0_S_KEYS&) = default;
    Conn0_S_KEYS& operator= (const Conn0_S_KEYS&) = default;
    Conn0_S_KEYS (Conn0_S_KEYS &&) = default;
    Conn0_S_KEYS& operator= (Conn0_S_KEYS &&) = default;
    ~Conn0_S_KEYS() = default;
    Conn0_S_KEYS (const gsl::span<uint8_t> raw);    // receive pkt
    Conn0_S_KEYS (      gsl::span<uint8_t> raw,     // send pkt
                                            const Nonce &client_nonce,
                                            const uint16_t pubkey_length,
                                            const uint16_t key_exchange_length,
                                            const uint16_t srv_enc_length,
                                            const uint16_t signature_length);

    static constexpr uint16_t min_size();
    gsl::span<const uint8_t> data_tosign() const;
private:
    static constexpr uint16_t pubkey_offset = sizeof(struct data);
    static constexpr uint16_t min_data_len = pubkey_offset +
                                                        4 * sizeof(uint16_t);
};

///////////////
// Conn0_C_AUTH
///////////////

class FENRIR_LOCAL Conn0_Auth_Data
{
private:
    gsl::span<uint8_t> _raw;
public:
    struct data {
        Device_ID _dev_id;
        Service_ID _service;
        Conn_ID _client_conn_id;
        uint8_t _max_padding;
        Packet::Alignment_Flag _alignment;
        Stream_ID _control_stream;
        Crypto::Auth::ID _selected_auth;
    };
    struct stream_info {
        Stream_ID _id;
        Counter _counter_start;
        Storage_t _type;
        Stream_PRIO _priority;
    };
    struct data const *const r;
    struct data *const w;
    Span_Overlay<uint8_t> _auth_username;
    Span_Overlay<uint8_t> _service_username;
    Span_Overlay<uint8_t> _auth_data;
    Span_Overlay<stream_info> _streams;

    Conn0_Auth_Data (const gsl::span<uint8_t> raw_pkt);
    Conn0_Auth_Data (      gsl::span<uint8_t> raw_pkt,
                                        const Device_ID &dev_id,
                                        const Service_ID &service,
                                        const Username &auth_username,
                                        const Username &service_username,
                                        const Conn_ID client_conn_id,
                                        const uint8_t padding,
                                        const Packet::Alignment_Flag alignment,
                                        const Crypto::Auth::ID selected_auth,
                                        const Stream_ID control_stream,
                                        const gsl::span<const uint8_t>auth_data,
                                        const uint16_t streams);

    static constexpr uint16_t min_size();
    static uint16_t stream_info_size (const uint16_t streams);
    explicit operator bool() const
        { return _error == false; }
private:
    bool _error;
    static constexpr uint16_t enc_offset = sizeof(struct data);
    static constexpr uint16_t min_data_len = enc_offset + 4 * sizeof(uint16_t);
};

class FENRIR_LOCAL Conn0_C_AUTH final : public Conn0
{
public:
    struct data {
        Fenrir_Version _version; // == 0 on error
        Conn0_Type _type;
    };
    struct data const *const r;
    struct data *const w;
    Span_Overlay<uint8_t> _srv_key;
    Span_Overlay<uint8_t> _enc_data;

    Conn0_C_AUTH (const gsl::span<uint8_t> raw_pkt);
    Conn0_C_AUTH (      gsl::span<uint8_t> raw_pkt,
                                const Span_Overlay<uint8_t> srv_key,
                                const uint16_t enc_data_length);


    static constexpr uint32_t min_size();
    gsl::span<uint8_t> as_cleartext (const uint16_t crypto_header,
                                                const uint16_t crypto_footer);
private:
    static constexpr uint16_t enc_offset = sizeof(struct data);
    static constexpr uint16_t min_data_len = enc_offset + 2 * sizeof(uint16_t);
};

/////////////////
// Conn0_S_RESULT
/////////////////

class FENRIR_LOCAL Conn0_Auth_Result
{
private:
    gsl::span<uint8_t> _raw;
public:
    struct data {
        Conn_ID _conn_id;   // 0 on auth failure
        Counter _data_counter_start;
        Stream_ID _control_stream;
        Packet::Alignment_Flag _alignment;
        uint8_t _max_padding;
        // IP
        // UDP_Port
    };
    struct stream_info {
        Stream_ID _id;
        Counter _counter_start;
        Storage_t _type;
        Stream_PRIO _priority;
    };
    struct data const *const r;
    struct data *const w;
    Span_Overlay<stream_info> _streams;

    Conn0_Auth_Result (const gsl::span<uint8_t> span);
    Conn0_Auth_Result (      gsl::span<uint8_t> span,
                                            const Conn_ID conn_id,
                                            const Counter data_counter_start,
                                            const Stream_ID control_stream,
                                            const Packet::Alignment_Flag _align,
                                            const uint8_t max_padding,
                                            const uint16_t streams);

    static constexpr uint16_t min_size();
    static uint16_t stream_info_size (const uint16_t streams);
    explicit operator bool() const
        { return _error == false; }
private:
    bool _error;
    static constexpr uint16_t stream_offset = sizeof(struct data);
    static constexpr uint16_t min_data_len = stream_offset +
                                                        1 * sizeof(uint16_t);
};

class FENRIR_LOCAL Conn0_S_RESULT final : public Conn0
{
public:

    struct data {
        Fenrir_Version _version; // == 0 on error
        Conn0_Type _type;
    };
    struct data const *const r;
    struct data *const w;
    Span_Overlay<uint8_t> _enc;

    Conn0_S_RESULT (const gsl::span<uint8_t> raw);
    Conn0_S_RESULT (      gsl::span<uint8_t> raw, const uint16_t enc_length);

    static constexpr uint16_t min_size();
    gsl::span<uint8_t> as_cleartext (const uint16_t crypto_overhead,
                                                const uint16_t hmac_overhead);
private:
    static constexpr uint16_t enc_offset = sizeof(struct data);
    static constexpr uint16_t min_data_len = enc_offset + 1 * sizeof(uint16_t);
};


} // namespace Impl
} // namespace Fenrir__v1

