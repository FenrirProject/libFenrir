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

#include "Fenrir/v1/data/Conn0.hpp"
#include <algorithm>
#include <string.h>

namespace Fenrir__v1 {
namespace Impl {

// FIXME: handle endianness (remove const from constructor?)
// move all to little endian (x86 style)
// mk_endian() <-- nice?

////////
// Conn0
////////

Conn0::Conn0 (gsl::span<uint8_t> raw, const Conn0_Type t,
                                                const Fenrir_Version version)
    : _raw (raw), r (reinterpret_cast<struct data*>(_raw.data())),
                  w (reinterpret_cast<struct data*>(_raw.data()))
{
    if (_raw.size() <= Conn0_offset) {
        return;
    }
    w->_version = version;
    w->_type = t;
}

Conn0::operator bool() const
{
    return _raw.size() > Conn0_offset && r->_version != Fenrir_Version (0) &&
                                        get_type (_raw) != type_safe::nullopt;
}

constexpr uint16_t Conn0::min_size()
    { return sizeof(Fenrir_Version) + sizeof(Conn0_Type); }

type_safe::optional<Conn0_Type> Conn0::get_type (
                                                const gsl::span<uint8_t> raw)
{
    if (static_cast<size_t>(raw.size()) < min_size())
        return type_safe::nullopt;
    return static_cast<Conn0_Type> (raw[sizeof(Fenrir_Version)]);
}

///////////////
// Conn0_C_INIT
///////////////

Conn0_C_INIT::Conn0_C_INIT (const gsl::span<uint8_t> raw)
    : Conn0 (raw, Conn0_Type::C_INIT, Fenrir_Version (1)),
        r (reinterpret_cast<struct data*>(_raw.data())),
        w (reinterpret_cast<struct data*>(_raw.data())),
        _supported_crypt (Span_Overlay<Crypto::Encryption::ID>::mk_overlay(
                                            raw.subspan (supported_offset))),
        _supported_hmac (Span_Overlay<Crypto::Hmac::ID>::mk_overlay(
                                        raw.subspan (static_cast<ssize_t> (
                                                supported_offset +
                                                _supported_crypt.raw_size())))),
        _supported_ecc (Span_Overlay<Recover::ECC::ID>::mk_overlay(
                            raw.subspan (static_cast<ssize_t> (
                                                supported_offset +
                                                _supported_crypt.raw_size() +
                                                _supported_hmac.raw_size())))),
        _supported_key (Span_Overlay<Crypto::Key::ID>::mk_overlay(
                            raw.subspan (static_cast<ssize_t> (
                                                supported_offset +
                                                _supported_crypt.raw_size() +
                                                _supported_hmac.raw_size() +
                                                _supported_ecc.raw_size())))),
        _supported_kdf (Span_Overlay<Crypto::KDF::ID>::mk_overlay(
                            raw.subspan (static_cast<ssize_t> (
                                                supported_offset +
                                                _supported_crypt.raw_size() +
                                                _supported_hmac.raw_size() +
                                                _supported_ecc.raw_size() +
                                                _supported_key.raw_size()))))
{
    if (_raw.size() < min_data_len || !_supported_crypt ||
                                        !_supported_hmac || !_supported_ecc ||
                                        !_supported_key || !_supported_kdf) {
        w->_version = Fenrir_Version (0);
        return;
    }
}

Conn0_C_INIT::Conn0_C_INIT (gsl::span<uint8_t> raw,
                    Random *const rnd, const Crypto::Key::Serial key_id,
                    const std::vector<Crypto::Encryption::ID>&supported_crypt,
                    const std::vector<Crypto::Hmac::ID> &supported_hmac,
                    const std::vector<Recover::ECC::ID> &supported_ecc,
                    const std::vector<Crypto::Key::ID> &supported_key,
                    const std::vector<Crypto::KDF::ID> &supported_kdf)
    : Conn0 (raw, Conn0_Type::C_INIT, Fenrir_Version (1)),
        r (reinterpret_cast<struct data*>(_raw.data())),
        w (reinterpret_cast<struct data*>(_raw.data())),
        _supported_crypt (Span_Overlay<Crypto::Encryption::ID>::mk_overlay(
                            _raw.subspan (supported_offset), supported_crypt)),
        _supported_hmac (Span_Overlay<Crypto::Hmac::ID>::mk_overlay(
                                        _raw.subspan (static_cast<ssize_t> (
                                                supported_offset +
                                                _supported_crypt.raw_size())),
                                                            supported_hmac)),
        _supported_ecc (Span_Overlay<Recover::ECC::ID>::mk_overlay(
                                        _raw.subspan (static_cast<ssize_t> (
                                                supported_offset +
                                                _supported_crypt.raw_size() +
                                                _supported_hmac.raw_size())),
                                                                supported_ecc)),
        _supported_key (Span_Overlay<Crypto::Key::ID>::mk_overlay(
                                        _raw.subspan (static_cast<ssize_t> (
                                                supported_offset +
                                                _supported_crypt.raw_size() +
                                                _supported_hmac.raw_size() +
                                                _supported_ecc.raw_size())),
                                                                supported_key)),
        _supported_kdf (Span_Overlay<Crypto::KDF::ID>::mk_overlay(
                                        _raw.subspan (static_cast<ssize_t> (
                                                supported_offset +
                                                _supported_crypt.raw_size() +
                                                _supported_hmac.raw_size() +
                                                _supported_ecc.raw_size() +
                                                _supported_key.raw_size())),
                                                                supported_kdf))
{
    if (_raw.size() < min_data_len || !_supported_crypt ||
                                        !_supported_hmac || !_supported_ecc ||
                                        !_supported_key || !_supported_kdf) {
        w->_version = Fenrir_Version (0);
        return;
    }
    w->_key_id = key_id;
    w->_nonce = rnd->uniform<Nonce>();
}

constexpr uint16_t Conn0_C_INIT::min_size()
{
    return Conn0::min_size() + sizeof(Crypto::Key::Serial) + sizeof(Nonce) +
                                                        sizeof(uint16_t) * 5;
}

/////////////////
// Conn0_S_COOKIE
/////////////////

Conn0_S_COOKIE::Conn0_S_COOKIE (gsl::span<uint8_t> raw)
    : Conn0 (raw, Conn0_Type::S_COOKIE, Fenrir_Version (1)),
        r (reinterpret_cast<struct data*>(_raw.data())),
        w (reinterpret_cast<struct data*>(_raw.data())),
        _supported_auth (Span_Overlay<Crypto::Auth::ID>::mk_overlay(
                                            _raw.subspan (supported_offset))),
        _cs_data_signature (Span_Overlay<uint8_t>::mk_overlay(
                                        _raw.subspan (static_cast<ssize_t> (
                                                supported_offset +
                                                _supported_auth.raw_size())))),
        _s_data_signature (Span_Overlay<uint8_t>::mk_overlay(
                                        (_raw.subspan (static_cast<ssize_t> (
                                            supported_offset +
                                            _supported_auth.raw_size() +
                                            _cs_data_signature.raw_size())))))
{
    if (_raw.size() < min_data_len || !_supported_auth || !_cs_data_signature ||
                                                            !_s_data_signature){
        w->_version = Fenrir_Version (0);
        return;
    }
}

Conn0_S_COOKIE::Conn0_S_COOKIE (gsl::span<uint8_t> raw,
                            const Crypto::Encryption::ID selected_crypt,
                            const Crypto::Hmac::ID selected_hmac,
                            const Recover::ECC::ID selected_ecc,
                            const Crypto::Key::ID selected_key,
                            const Crypto::KDF::ID selected_kdf,
                            const int64_t timestamp,
                            const std::vector<Crypto::Auth::ID> &supported_auth,
                            const uint16_t signature_length)
    : Conn0 (raw, Conn0_Type::S_COOKIE, Fenrir_Version (1)),
        r (reinterpret_cast<struct data*>(_raw.data())),
        w (reinterpret_cast<struct data*>(_raw.data())),
        _supported_auth (Span_Overlay<Crypto::Auth::ID>::mk_overlay(
                        _raw.subspan (supported_offset), supported_auth)),
        _cs_data_signature (Span_Overlay<uint8_t>::mk_overlay(
                        _raw.subspan (static_cast<ssize_t> (supported_offset
                            + _supported_auth.raw_size())), signature_length)),
        _s_data_signature (Span_Overlay<uint8_t>::mk_overlay(
                        _raw.subspan (static_cast<ssize_t> (supported_offset
                                                + _supported_auth.raw_size() +
                                                _cs_data_signature.raw_size())),
                                                            signature_length))
{
    if (_raw.size() < min_data_len || !_supported_auth ||
                                    !_cs_data_signature || !_s_data_signature) {
        w->_version = Fenrir_Version (0);
        return;
    }
    w->_selected_crypt = selected_crypt;
    w->_selected_hmac  = selected_hmac;
    w->_selected_ecc   = selected_ecc;
    w->_selected_key   = selected_key;
    w->_selected_kdf   = selected_kdf;
    w->_timestamp = timestamp;
}

constexpr uint16_t Conn0_S_COOKIE::min_size()
    { return min_data_len; }

const std::vector<uint8_t> Conn0_S_COOKIE::client_server_data_tosign (
                                                const Conn0_C_INIT &req) const
{

    const uint32_t s_cookie_length = static_cast<uint32_t>(supported_offset +
                                                    _supported_auth.raw_size());
    std::vector<uint8_t> ret (static_cast<size_t> (req._raw.size()) +
                                                            s_cookie_length, 0);
    memcpy (ret.data(), req._raw.data(), static_cast<size_t>(req._raw.size()));
    memcpy (ret.data() + req._raw.size(), _raw.data(), s_cookie_length);

    return ret;
}

const gsl::span<uint8_t> Conn0_S_COOKIE::server_data_tosign() const
    { return _raw.subspan (0, static_cast<uint32_t>(supported_offset)); }

/////////////////
// Conn0_C_COOKIE
/////////////////

Conn0_C_COOKIE::Conn0_C_COOKIE (const gsl::span<uint8_t> raw)
    : Conn0 (raw, Conn0_Type::C_COOKIE, Fenrir_Version (1)),
      r (reinterpret_cast<struct data*>(_raw.data())),
      w (reinterpret_cast<struct data*>(_raw.data())),
      _cookie (Span_Overlay<uint8_t>::mk_overlay(
                                            _raw.subspan (cookie_offset))),
      _client_pubkey (Span_Overlay<uint8_t>::mk_overlay(
                                            _raw.subspan (static_cast<ssize_t> (
                                                        cookie_offset +
                                                        _cookie.raw_size())))),
      _client_key_data (Span_Overlay<uint8_t>::mk_overlay(
                                        _raw.subspan (static_cast<ssize_t> (
                                                cookie_offset +
                                                _cookie.raw_size() +
                                                _client_pubkey.raw_size()))))
{
    if (_raw.size() < min_data_len || !_cookie || !_client_pubkey ||
                                                            !_client_key_data){
        w->_version = Fenrir_Version (0);
        return;
    }
}

Conn0_C_COOKIE::Conn0_C_COOKIE (gsl::span<uint8_t> raw,
                                    const Crypto::Key::Serial pubkey_id,
                                    const Crypto::Encryption::ID selected_crypt,
                                    const Crypto::Hmac::ID selected_hmac,
                                    const Recover::ECC::ID selected_ecc,
                                    const Crypto::Key::ID selected_key,
                                    const Crypto::KDF::ID selected_kdf,
                                    const int64_t timestamp,
                                    Random *const rnd,
                                    const uint16_t cookie_len,
                                    const uint16_t client_pubkey_len,
                                    const uint16_t client_key_data_len)
    : Conn0 (raw, Conn0_Type::C_COOKIE, Fenrir_Version (1)),
        r (reinterpret_cast<struct data*>(_raw.data())),
        w (reinterpret_cast<struct data*>(_raw.data())),
        _cookie (Span_Overlay<uint8_t>::mk_overlay(
                                    _raw.subspan (cookie_offset), cookie_len)),
        _client_pubkey (Span_Overlay<uint8_t>::mk_overlay(
                                            _raw.subspan (static_cast<ssize_t> (
                                                        cookie_offset +
                                                        _cookie.raw_size())),
                                                        client_pubkey_len)),
        _client_key_data (Span_Overlay<uint8_t>::mk_overlay(
                                            _raw.subspan (static_cast<ssize_t> (
                                                    cookie_offset +
                                                    _cookie.raw_size() +
                                                    _client_pubkey.raw_size())),
                                                    client_key_data_len))
{
    if (_raw.size() < min_data_len || !_cookie || !_client_pubkey ||
                                                            !_client_key_data){
        w->_version = Fenrir_Version (0);
        return;
    }

    w->_key_id = pubkey_id;
    w->_selected_crypt = selected_crypt;
    w->_selected_hmac  = selected_hmac;
    w->_selected_ecc   = selected_ecc;
    w->_selected_key   = selected_key;
    w->_selected_kdf   = selected_kdf;
    w->_timestamp = timestamp;
    w->_nonce = rnd->uniform<Nonce>();
}

constexpr uint16_t Conn0_C_COOKIE::min_size()
    { return min_data_len; }

///////////////
// Conn0_S_KEYS
///////////////

Conn0_S_KEYS::Conn0_S_KEYS (const gsl::span<uint8_t> raw)
    : Conn0 (raw, Conn0_Type::S_KEYS, Fenrir_Version (1)),
        r (reinterpret_cast<struct data*>(_raw.data())),
        w (reinterpret_cast<struct data*>(_raw.data())),
        _pubkey (Span_Overlay<uint8_t>::mk_overlay (
                                                _raw.subspan (pubkey_offset))),
        _key_data (Span_Overlay<uint8_t>::mk_overlay(
                                            _raw.subspan (static_cast<ssize_t> (
                                                        pubkey_offset +
                                                        _pubkey.raw_size())))),
        _srv_enc (Span_Overlay<uint8_t>::mk_overlay(
                                            _raw.subspan (static_cast<ssize_t> (
                                                      pubkey_offset +
                                                      _pubkey.raw_size() +
                                                      _key_data.raw_size())))),
        _sign (Span_Overlay<uint8_t>::mk_overlay(
                                        _raw.subspan (static_cast<ssize_t> (
                                                        pubkey_offset +
                                                        _pubkey.raw_size() +
                                                        _key_data.raw_size() +
                                                        _srv_enc.raw_size()))))
{
    if (_raw.size() < min_data_len || !_pubkey || !_key_data ||
                                                        !_srv_enc || !_sign) {
        w->_version = Fenrir_Version (0);
        return;
    }

}

Conn0_S_KEYS::Conn0_S_KEYS (gsl::span<uint8_t> raw,
                                           const Nonce &client_nonce,
                                           const uint16_t pubkey_length,
                                           const uint16_t key_exchange_length,
                                           const uint16_t srv_enc_length,
                                           const uint16_t signature_length)
    : Conn0 (raw, Conn0_Type::S_KEYS, Fenrir_Version (1)),
        r (reinterpret_cast<struct data*>(_raw.data())),
        w (reinterpret_cast<struct data*>(_raw.data())),
        _pubkey (Span_Overlay<uint8_t>::mk_overlay(
                                _raw.subspan (pubkey_offset), pubkey_length)),
        _key_data (Span_Overlay<uint8_t>::mk_overlay(
                                            _raw.subspan (static_cast<ssize_t> (
                                                        pubkey_offset +
                                                        _pubkey.raw_size())),
                                                        key_exchange_length)),
        _srv_enc (Span_Overlay<uint8_t>::mk_overlay(
                                           _raw.subspan (static_cast<ssize_t> (
                                                   pubkey_offset +
                                                   _pubkey.raw_size() +
                                                   _key_data.raw_size())),
                                                   srv_enc_length)),
        _sign (Span_Overlay<uint8_t>::mk_overlay(
                                            _raw.subspan (static_cast<ssize_t> (
                                                    pubkey_offset +
                                                    _pubkey.raw_size() +
                                                    _key_data.raw_size() +
                                                    _srv_enc.raw_size())),
                                                    signature_length))
{
    if (_raw.size() < min_data_len || !_pubkey || !_key_data ||
                                                        !_srv_enc || !_sign) {
        w->_version = Fenrir_Version (0);
        return;
    }

    w->_client_nonce = client_nonce;
}

constexpr uint16_t Conn0_S_KEYS::min_size()
    { return min_data_len; }

gsl::span<const uint8_t> Conn0_S_KEYS::data_tosign() const
{
    return _raw.subspan (0, static_cast<ssize_t> (pubkey_offset +
                                    _pubkey.raw_size() + _key_data.raw_size()));
}

///////////////
// Conn0_C_AUTH
///////////////

Conn0_C_AUTH::Conn0_C_AUTH (const gsl::span<uint8_t> raw)
    : Conn0 (raw, Conn0_Type::C_AUTH, Fenrir_Version (1)),
        r (reinterpret_cast<struct data*>(_raw.data())),
        w (reinterpret_cast<struct data*>(_raw.data())),
        _srv_key (Span_Overlay<uint8_t>::mk_overlay (
                                                _raw.subspan (enc_offset))),
        _enc_data (Span_Overlay<uint8_t>::mk_overlay(
                                            _raw.subspan (static_cast<ssize_t> (
                                                        enc_offset +
                                                        _srv_key.raw_size()))))
{
    if (_raw.size() < min_data_len || !_srv_key || !_enc_data) {
        w->_version = Fenrir_Version (0);
        return;
    }
}

Conn0_C_AUTH::Conn0_C_AUTH (gsl::span<uint8_t> raw,
                                           const Span_Overlay<uint8_t> srv_key,
                                           const uint16_t enc_data_length)
    : Conn0 (raw, Conn0_Type::C_AUTH, Fenrir_Version (1)),
        r (reinterpret_cast<struct data*>(_raw.data())),
        w (reinterpret_cast<struct data*>(_raw.data())),
        _srv_key (Span_Overlay<uint8_t>::mk_overlay(
                            _raw.subspan (enc_offset),
                                    static_cast<uint16_t> (srv_key.size()))),
        _enc_data (Span_Overlay<uint8_t>::mk_overlay(
                                            _raw.subspan (static_cast<ssize_t> (
                                                        enc_offset +
                                                        _srv_key.raw_size())),
                                                        enc_data_length))
{
    if (_raw.size() < min_data_len || !_srv_key || !_enc_data) {
        w->_version = Fenrir_Version (0);
        return;
    }

    memcpy (_srv_key.data(), srv_key.data(),
                                        static_cast<size_t>(srv_key.size()));
}

constexpr uint32_t Conn0_C_AUTH::min_size()
    { return min_data_len; }

gsl::span<uint8_t> Conn0_C_AUTH::as_cleartext (const uint16_t header,
                                                        const uint16_t footer)
{
    if ((header + footer) >= _enc_data.raw_size())
        return gsl::span<uint8_t>();
    return _enc_data.subspan (header, _enc_data.size() - (header + footer));
}

//////////////////
// Conn0_Auth_Data
//////////////////


Conn0_Auth_Data::Conn0_Auth_Data (const gsl::span<uint8_t> raw_pkt)
    : _raw (std::move(raw_pkt)),
        r (reinterpret_cast<struct data*>(_raw.data())),
        w (reinterpret_cast<struct data*>(_raw.data())),
        _auth_username (Span_Overlay<uint8_t>::mk_overlay(
                            _raw.subspan (enc_offset))),
        _service_username (Span_Overlay<uint8_t>::mk_overlay(
                            _raw.subspan (
                                static_cast<ssize_t> (enc_offset +
                                                _auth_username.raw_size())))),
        _auth_data (Span_Overlay<uint8_t>::mk_overlay (
                                    _raw.subspan (
                                        static_cast<ssize_t> (enc_offset +
                                            _auth_username.raw_size() +
                                            _service_username.raw_size())))),
        _streams (Span_Overlay<stream_info>::mk_overlay(
                                            _raw.subspan (static_cast<ssize_t> (
                                                    enc_offset +
                                                    _auth_username.raw_size() +
                                                    _service_username.raw_size()
                                                    + _auth_data.raw_size()))))
{
    if (_raw.size() < min_data_len || !_auth_username || ! _service_username ||
                                                    !_auth_data || !_streams ||
            static_cast<const uint8_t>(r->_alignment) >
                static_cast<const uint8_t> (Packet::Alignment_Flag::UINT64)) {
        _error = true;
        return;
    }
    for (const auto &val : _streams) {
        if (!storage_t_validate(val._type) ||
                                        (val._counter_start >= max_counter)) {
            _error = true;
            return;
        }
    }
    _error = false;
}


Conn0_Auth_Data::Conn0_Auth_Data (gsl::span<uint8_t> raw_pkt,
                                        const Device_ID &dev_id,
                                        const Service_ID &service,
                                        const Username &auth_username,
                                        const Username &service_username,
                                        const Conn_ID client_conn_id,
                                        const uint8_t padding,
                                        const Packet::Alignment_Flag alignment,
                                        const Crypto::Auth::ID selected_auth,
                                        const Lattice_Node::ID lattice_node,
                                        const Stream_ID control_stream,
                                        const gsl::span<const uint8_t>auth_data,
                                        const uint16_t streams)
    : _raw (std::move(raw_pkt)),
        r (reinterpret_cast<struct data*>(_raw.data())),
        w (reinterpret_cast<struct data*>(_raw.data())),
        _auth_username (Span_Overlay<uint8_t>::mk_overlay(
                            _raw.subspan (enc_offset),
                                static_cast<uint16_t> (auth_username.size()))),
        _service_username (Span_Overlay<uint8_t>::mk_overlay(
                        _raw.subspan (
                            static_cast<ssize_t> (enc_offset +
                                                    _auth_username.raw_size())),
                            static_cast<uint16_t> (service_username.size()))),
        _auth_data (Span_Overlay<uint8_t>::mk_overlay(
                                _raw.subspan (static_cast<ssize_t> (enc_offset +
                                                _auth_username.raw_size() +
                                                _service_username.raw_size())),
                                    static_cast<uint16_t> (auth_data.size()))),
        _streams (Span_Overlay<stream_info>::mk_overlay(
                                        _raw.subspan (static_cast<ssize_t> (
                                                enc_offset +
                                                _auth_username.raw_size() +
                                                _service_username.raw_size() +
                                                _auth_data.raw_size())),
                                            streams))
{
    if (_raw.size() < min_data_len ||
            !_auth_username ||
            ! _service_username ||
            !_auth_data ||
            !_streams) {
        _error = true;
        return;
    }
    w->_dev_id = dev_id;
    w->_service = service;
    w->_client_conn_id = client_conn_id;
    w->_max_padding = padding;
    w->_alignment = alignment;
    w->_control_stream = control_stream;
    w->_selected_auth = selected_auth;
    w->_lattice_node = lattice_node;
    auth_username (_auth_username);
    service_username (_service_username);
    memcpy (_auth_data.data(), auth_data.data(),
                                    static_cast<size_t> (_auth_data.size()));
    _error = false;
}

constexpr uint16_t Conn0_Auth_Data::min_size()
    { return min_data_len; }

uint16_t Conn0_Auth_Data::stream_info_size (const uint16_t streams)
    { return streams * sizeof(stream_info); }


////////////////////
// Conn0_Auth_Result
////////////////////

Conn0_Auth_Result::Conn0_Auth_Result (const gsl::span<uint8_t> raw)
    : _raw (std::move(raw)),
        r (reinterpret_cast<struct data*>(_raw.data())),
        w (reinterpret_cast<struct data*>(_raw.data())),
        _streams (Span_Overlay<stream_info>::mk_overlay (
                                                _raw.subspan (stream_offset)))
{
    if (_raw.size() < min_data_len || !_streams || r->_conn_id < Conn_Reserved||
            static_cast<const uint8_t>(r->_alignment) >
                static_cast<const uint8_t> (Packet::Alignment_Flag::UINT64)){
        _error = true;
        return;
    }
    for (const auto &val : _streams) {
        if (!storage_t_validate(val._type) ||
                                        (val._counter_start >= max_counter)) {
            _error = true;
            return;
        }
    }
    _error = false;
}

Conn0_Auth_Result::Conn0_Auth_Result (gsl::span<uint8_t> raw,
                                            const Conn_ID conn_id,
                                            const Counter data_counter_start,
                                            const Stream_ID control_stream,
                                            const Packet::Alignment_Flag align,
                                            const uint8_t max_padding,
                                            const uint16_t streams)
    : _raw (std::move(raw)),
        r (reinterpret_cast<struct data*>(_raw.data())),
        w (reinterpret_cast<struct data*>(_raw.data())),
        _streams (Span_Overlay<stream_info>::mk_overlay (
                                        _raw.subspan (stream_offset), streams))
{
    if (_raw.size() < min_data_len || !_streams) {
        _error = true;
        return;
    }
    w->_conn_id = conn_id;
    w->_data_counter_start = data_counter_start;
    w->_control_stream = control_stream;
    w->_alignment = align;
    w->_max_padding = max_padding;
    _error = false;
}

constexpr uint16_t Conn0_Auth_Result::min_size()
    { return min_data_len; }

uint16_t Conn0_Auth_Result::stream_info_size (const uint16_t streams)
    { return streams * sizeof(stream_info); }


/////////////////
// Conn0_S_RESULT
/////////////////


Conn0_S_RESULT::Conn0_S_RESULT (const gsl::span<uint8_t> raw)
    : Conn0 (raw, Conn0_Type::S_RESULT, Fenrir_Version (1)),
        r (reinterpret_cast<struct data*>(_raw.data())),
        w (reinterpret_cast<struct data*>(_raw.data())),
        _enc (Span_Overlay<uint8_t>::mk_overlay (_raw.subspan (enc_offset)))
{
    if (_raw.size() < min_data_len || !_enc) {
        w->_version = Fenrir_Version (0);
        return;
    }
}

Conn0_S_RESULT::Conn0_S_RESULT (gsl::span<uint8_t> raw,
                                                    const uint16_t enc_length)
    : Conn0 (raw, Conn0_Type::S_RESULT, Fenrir_Version (1)),
        r (reinterpret_cast<struct data*>(_raw.data())),
        w (reinterpret_cast<struct data*>(_raw.data())),
        _enc (Span_Overlay<uint8_t>::mk_overlay (_raw.subspan (enc_offset),
                                                                    enc_length))
{
    if (_raw.size() < min_data_len || !_enc) {
        w->_version = Fenrir_Version (0);
        return;
    }
}

constexpr uint16_t Conn0_S_RESULT::min_size()
    { return min_data_len; }

gsl::span<uint8_t> Conn0_S_RESULT::as_cleartext (const uint16_t crypto_header,
                                                const uint16_t crypto_footer)
{
    if ((crypto_header + crypto_footer)>= _enc.raw_size())
        return gsl::span<uint8_t>();
    return _enc.subspan (crypto_header, _enc.size()
                                            - (crypto_header + crypto_footer));
}

} // namespace Impl
} // namespace Fenrir__v1
