/*
 * Copyright (c) 2017, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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
#include "Fenrir/v1/crypto/Crypto.hpp"
#include "Fenrir/v1/util/Random.hpp"
#include <array>
#include <gsl/span>
#include <memory>
#include <mutex>
#include <sodium.h>
#include <string.h>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {

namespace Crypto {

class FENRIR_LOCAL ChaCha20_Poly1305_IETF : public Encryption
{
public:
    ChaCha20_Poly1305_IETF()
        : Encryption(std::shared_ptr<Lib> (nullptr), nullptr, nullptr, nullptr),
            _nonces_sent (0), _initialized (sodium_init() >= 0)
        { randombytes_buf (_last_nonce.data(), _last_nonce.size()); }
    ChaCha20_Poly1305_IETF (const ChaCha20_Poly1305_IETF&) = default;
    ChaCha20_Poly1305_IETF& operator= (const ChaCha20_Poly1305_IETF&)=default;
    ChaCha20_Poly1305_IETF (ChaCha20_Poly1305_IETF &&) = default;
    ChaCha20_Poly1305_IETF& operator= (ChaCha20_Poly1305_IETF &&) = default;
    ~ChaCha20_Poly1305_IETF() override {}
    void parse_event (std::shared_ptr<Event::Plugin_Timer> ev) override
        { FENRIR_UNUSED (ev); }

    Encryption::ID id() const override
        { return Encryption::ID {2}; }
    uint16_t bytes_header() const override
        { return crypto_aead_chacha20poly1305_IETF_NPUBBYTES; }
    uint16_t bytes_footer() const override
        { return crypto_aead_chacha20poly1305_ietf_ABYTES; }
    uint16_t bytes_overhead() const override
        { return bytes_header() + bytes_footer(); }
    bool set_key (const std::array<uint8_t, 64> &key) override
    {
        memcpy (_key.data(), key.data(), _key.size());
        return true;
    }
    Impl::Error encrypt (gsl::span<uint8_t> in) override
    {
        if (in.size() < bytes_overhead())
            return Impl::Error::WRONG_INPUT;
        gsl::span<uint8_t, crypto_aead_chacha20poly1305_IETF_NPUBBYTES> nonce
                    (in.data(), crypto_aead_chacha20poly1305_IETF_NPUBBYTES);
        {
            std::unique_lock<std::mutex> lock (_mtx);
            FENRIR_UNUSED (lock);
            sodium_increment (reinterpret_cast<uint8_t*> (_last_nonce.data()),
                                                            _last_nonce.size());
            memcpy (nonce.data(), _last_nonce.data(), nonce.size());
            ++_nonces_sent;
        }
        uint8_t *cipher_start = in.data() +
                                    crypto_aead_chacha20poly1305_IETF_NPUBBYTES;
        unsigned long long ciphertext_len;
        unsigned long long data_size = static_cast<unsigned long long> (
                                                in.size() - bytes_overhead());
        crypto_aead_chacha20poly1305_ietf_encrypt (cipher_start,
                                                                &ciphertext_len,
                                                                cipher_start,
                                                                data_size,
                                                                nullptr,
                                                                0,
                                                                nullptr,
                                                                nonce.data(),
                                                                _key.data());
        return Impl::Error::NONE;
    }
    Impl::Error decrypt (const gsl::span<uint8_t> in,
                                            gsl::span<uint8_t> &out) override
    {
        if (in.size() < bytes_overhead())
            return Impl::Error::WRONG_INPUT;
        gsl::span<uint8_t, crypto_aead_chacha20poly1305_IETF_NPUBBYTES> nonce
                    (in.data(), crypto_aead_chacha20poly1305_IETF_NPUBBYTES);

        uint8_t *cipher_start = in.data() +
                                    crypto_aead_chacha20poly1305_IETF_NPUBBYTES;
        unsigned long long cleartext_len;
        const unsigned long long data_size = static_cast<unsigned long long> (
                                                in.size() - bytes_header());
        int res = crypto_aead_chacha20poly1305_ietf_decrypt (cipher_start,
                                                                &cleartext_len,
                                                                nullptr,
                                                                cipher_start,
                                                                data_size,
                                                                nullptr,
                                                                0,
                                                                nonce.data(),
                                                                _key.data());
        if (res)
            return Impl::Error::WRONG_INPUT;
        out = in.subspan (crypto_aead_chacha20poly1305_IETF_NPUBBYTES,
                                                in.size() - bytes_overhead());
        return Impl::Error::NONE;
    }
    bool is_authenticated() const override
        { return true; }
private:
    uint64_t _nonces_sent;
    std::array<uint8_t, 32> _key;
    std::array<uint8_t, crypto_aead_chacha20poly1305_IETF_NPUBBYTES>_last_nonce;
    std::mutex _mtx;
    bool _initialized;
};

class FENRIR_LOCAL Ed25519 : public Key
{
    // This MUST include both key and key-exhange algorithms
public:
    // serial of the key.
    struct FENRIR_LOCAL Serial :
                    type_safe::strong_typedef<Serial, uint16_t>,
                    type_safe::strong_typedef_op::equality_comparison<Serial>,
                    type_safe::strong_typedef_op::relational_comparison<Serial>
        { using strong_typedef::strong_typedef; };
    // plugin ID
    struct FENRIR_LOCAL ID :
                    type_safe::strong_typedef<ID, uint16_t>,
                    type_safe::strong_typedef_op::equality_comparison<ID>,
                    type_safe::strong_typedef_op::relational_comparison<ID>
        { using strong_typedef::strong_typedef; };
    Ed25519()
        : Key(std::shared_ptr<Lib> (nullptr), nullptr, nullptr, nullptr)
        {}
    Ed25519 (const Ed25519&) = default;
    Ed25519& operator= (const Ed25519&) = default;
    Ed25519 (Ed25519 &&) = default;
    Ed25519& operator= (Ed25519 &&) = default;
    ~Ed25519() override {}
    void parse_event (std::shared_ptr<Event::Plugin_Timer> ev) override
        { FENRIR_UNUSED (ev); }

    Key::ID id() const override
        { return Key::ID { 1 }; }
    uint16_t signature_length() const override
        { return crypto_sign_ed25519_BYTES; }
    uint16_t get_publen () const override
        { return crypto_sign_ed25519_PUBLICKEYBYTES; }
    bool get_pubkey (gsl::span<uint8_t> out) override
    {
        if (out.size() != static_cast<ssize_t> (_pub.size()))
            return false;
        memcpy (out.data(), _pub.data(), _pub.size());
        return true;
    }
    bool init (const gsl::span<const uint8_t> pub) override
    {
        if (static_cast<uint32_t> (pub.size()) != _pub.size())
            return false;
        memcpy (_pub.data(), pub.data(), _pub.size());
        return true;
    }
    bool init (const gsl::span<const uint8_t> priv,
                                    const gsl::span<const uint8_t> pub) override
    {
        if (static_cast<uint32_t> (priv.size()) != _priv.size() ||
                            static_cast<uint32_t> (pub.size()) != _pub.size()) {
            return false;
        }
        memcpy (_priv.data(), priv.data(), _priv.size());
        // generate public key from private, so that we test validity of both
        if (crypto_sign_ed25519_sk_to_pk (_pub.data(), _priv.data()) != 0)
            return false;
        if (sodium_memcmp (pub.data(), _pub.data(), _pub.size()) != 0)
            return false;
        return true;
    }
    bool init() override
    {
        crypto_sign_ed25519_keypair (_pub.data(), _priv.data());
        return true;
    }
    bool verify (const gsl::span<const uint8_t> data,
                            const gsl::span<const uint8_t> signature) override
    {
        if (signature.size() != crypto_sign_ed25519_BYTES)
            return false;
        if (crypto_sign_ed25519_verify_detached (signature.data(), data.data(),
                    static_cast<uint32_t> (data.size()), _pub.data()) != 0) {
            return false;
        }
        return true;
    }
    bool sign (const gsl::span<const uint8_t> in,
                                                gsl::span<uint8_t> out) override
    {
        if (out.size() != crypto_sign_ed25519_BYTES)
            return false;
        long long unsigned int size = 0;
        if (crypto_sign_ed25519_detached (out.data(), &size, in.data(),
                        static_cast<uint32_t> (in.size()), _priv.data()) != 0) {
            return false;
        }
        if (size != static_cast<unsigned long long> (out.size()))
            return false;
        return true;
    }
    bool is_the_same (const gsl::span<const uint8_t> priv,
                                const gsl::span<const uint8_t> pub) override
    {
        if (static_cast<uint32_t> (priv.size()) != _priv.size() ||
                            static_cast<uint32_t> (pub.size()) != _pub.size()) {
            return false;
        }

        if (sodium_memcmp (_priv.data(), priv.data(), _priv.size()) != 0 ||
                    sodium_memcmp (_pub.data(), pub.data(), _pub.size()) != 0) {
            return false;
        }
        return true;
    }
    // generate initial key data.
    uint16_t get_key_data_length() const override
        { return 0; }
    bool get_key_data (gsl::span<uint8_t> data) override
    {
        if (data.size() != 0)
            return false;
        return true;
    }
    // key exchange
    // 512 bits per key required.
    bool exchange_key (const Key *other_pubkey,
                                    const gsl::span<const uint8_t> key_data,
                                    const gsl::span<const uint8_t> our_key_data,
                                    gsl::span<uint8_t, 64> output,
                                    const Role role) override
    {
        FENRIR_UNUSED (key_data);
        FENRIR_UNUSED (our_key_data);
        if (other_pubkey->id() != id())
            return false;
        const Ed25519 *other_ed_key = reinterpret_cast<const Ed25519*> (
                                                                other_pubkey);

        std::array<uint8_t, crypto_scalarmult_curve25519_BYTES> curve_other_pub;
        std::array<uint8_t, crypto_scalarmult_curve25519_BYTES> curve_pub;
        std::array<uint8_t, crypto_scalarmult_curve25519_BYTES> curve_priv;
        if (crypto_sign_ed25519_pk_to_curve25519 (curve_other_pub.data(),
                                            other_ed_key->_pub.data()) != 0) {
            return false;
        }
        if (crypto_sign_ed25519_pk_to_curve25519 (curve_pub.data(),
                                                            _pub.data()) != 0) {
            return false;
        }
        if (crypto_sign_ed25519_sk_to_curve25519 (curve_priv.data(),
                                                            _priv.data()) != 0){
            return false;
        }

        // this the key exchange generates 2 keys of
        // crypto_kx_SESSIONKEYBYTES == 32 bytes each.
        // but we don't use those directly, we will pass it as input to a KDF.
        if (role == Role::Client) {
            if (crypto_kx_client_session_keys (output.data() + 32,output.data(),
                                            curve_pub.data(), curve_priv.data(),
                                            curve_other_pub.data()) != 0) {
                return false;
            }
        } else {
            if (crypto_kx_server_session_keys (output.data(),output.data() + 32,
                                            curve_pub.data(), curve_priv.data(),
                                            curve_other_pub.data()) != 0) {
                return false;
            }
        }
        return true;
    }
private:
    std::array<uint8_t, crypto_sign_ed25519_PUBLICKEYBYTES> _pub;
    std::array<uint8_t, crypto_sign_ed25519_SECRETKEYBYTES> _priv;
};

class FENRIR_LOCAL blake2b final : public KDF
{
public:
    blake2b()
        : KDF(std::shared_ptr<Lib> (nullptr), nullptr, nullptr, nullptr)
        {}
    blake2b (const blake2b&) = default;
    blake2b& operator= (const blake2b&) = default;
    blake2b (blake2b &&) = default;
    blake2b& operator= (blake2b &&) = default;
    ~blake2b() override {}
    void parse_event (std::shared_ptr<Event::Plugin_Timer> ev) override
        { FENRIR_UNUSED (ev); }

    KDF::ID id() const override
        { return KDF::ID {1}; }

    bool init (const gsl::span<uint8_t, 64> key) override
    {
        memcpy (secret.data(), key.data(), secret.size());
        return true;
    }

    bool get (const uint64_t id, const std::array<char, 8> context,
                                        std::array<uint8_t, 64> &out) override
    {
        crypto_kdf_derive_from_key (out.data(), static_cast<size_t>(out.size()),
                                            id, context.data(), secret.data());
        return true;
    }
private:
    std::array<uint8_t, 64> secret;
};

} // namespace Crypto
} // namespace Impl
} // namespace Fenrir__v1
