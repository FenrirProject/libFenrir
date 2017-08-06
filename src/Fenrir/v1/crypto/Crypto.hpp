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
#include "Fenrir/v1/plugin/Dynamic.hpp"
#include "Fenrir/v1/net/Role.hpp"
#include <array>
#include <gsl/span>
#include <memory>
#include <tuple>
#include <type_safe/strong_typedef.hpp>
#include <type_safe/optional.hpp>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {

class Lib;
class Loader;
class Random;
namespace Event {
class Loop;
} // namespace Event

namespace Crypto {

class FENRIR_LOCAL Encryption : public Dynamic
{
public:
    struct FENRIR_LOCAL ID :
                    type_safe::strong_typedef<ID, uint16_t>,
                    type_safe::strong_typedef_op::equality_comparison<ID>,
                    type_safe::strong_typedef_op::relational_comparison<ID>
        { using strong_typedef::strong_typedef; };

    Encryption (const std::shared_ptr<Lib> from, Event::Loop *const loop,
                                        Loader *const loader, Random *const rnd)
        : Dynamic (from, loop, loader, rnd) {}
    Encryption() = delete;
    Encryption (const Encryption&) = default;
    Encryption& operator= (const Encryption&) = default;
    Encryption (Encryption &&) = default;
    Encryption& operator= (Encryption &&) = default;
    virtual ~Encryption () {}

    virtual Encryption::ID id() const = 0;
    virtual uint16_t bytes_header() const = 0;
    virtual uint16_t bytes_footer() const = 0;
    virtual uint16_t bytes_overhead() const = 0;
    virtual bool set_key (const std::array<uint8_t, 64> &key) = 0;
    virtual Impl::Error encrypt (gsl::span<uint8_t> in) = 0;
    virtual Impl::Error decrypt (const gsl::span<uint8_t> in,
                                                gsl::span<uint8_t> &out) = 0;
    virtual bool is_authenticated() const = 0;
};

class FENRIR_LOCAL Hmac : public Dynamic
{
public:
    struct FENRIR_LOCAL ID :
                    type_safe::strong_typedef<ID, uint16_t>,
                    type_safe::strong_typedef_op::equality_comparison<ID>,
                    type_safe::strong_typedef_op::relational_comparison<ID>
        { using strong_typedef::strong_typedef; };
    Hmac (const std::shared_ptr<Lib> from, Event:: Loop *const loop,
                                        Loader *const loader, Random *const rnd)
        : Dynamic (from, loop, loader, rnd) {}
    Hmac() = delete;
    Hmac (const Hmac&) = default;
    Hmac& operator= (const Hmac&) = default;
    Hmac (Hmac &&) = default;
    Hmac& operator= (Hmac &&) = default;
    virtual ~Hmac () {}

    virtual Hmac::ID id() const = 0;
    virtual uint16_t bytes_header() const = 0;
    virtual uint16_t bytes_footer() const = 0;
    virtual uint16_t bytes_overhead() const = 0;
    virtual bool set_key (const std::array<uint8_t, 64> &key) = 0;
    virtual bool is_valid (const gsl::span<uint8_t> in,
                                                gsl::span<uint8_t> &out) = 0;
    virtual Impl::Error add_hmac (gsl::span<uint8_t> in) = 0;
};


class FENRIR_LOCAL Key : public Dynamic
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
    Key (const std::shared_ptr<Lib> from, Event::Loop *const loop,
                                        Loader *const loader, Random *const rnd)
        : Dynamic (from, loop, loader, rnd) {}
    Key() = delete;
    Key (const Key&) = default;
    Key& operator= (const Key&) = default;
    Key (Key &&) = default;
    Key& operator= (Key &&) = default;
    virtual ~Key () {}

    virtual Key::ID id() const = 0;
    virtual uint16_t signature_length() const = 0;
    virtual uint16_t get_publen () const = 0;
    virtual bool get_pubkey (gsl::span<uint8_t> out) = 0;
    virtual bool init (const gsl::span<const uint8_t> priv,
                                        const gsl::span<const uint8_t> pub) = 0;
    virtual bool init (const gsl::span<const uint8_t> pub) = 0;
    virtual bool init() = 0;
    virtual bool verify (const gsl::span<const uint8_t> data,
                                const gsl::span<const uint8_t> signature) = 0;
    virtual bool sign (const gsl::span<const uint8_t> in,
                                                    gsl::span<uint8_t> out) = 0;
    virtual bool is_the_same (const gsl::span<const uint8_t> priv,
                                        const gsl::span<const uint8_t> pub) = 0;
    // generate initial key data.
    virtual uint16_t get_key_data_length() const = 0;
    virtual bool get_key_data (gsl::span<uint8_t> data) = 0;
    // key exchange
    // 512 bits per key required.
    virtual bool exchange_key (const Key *other_pubkey,
                                    const gsl::span<const uint8_t> key_data,
                                    const gsl::span<const uint8_t> our_key_data,
                                    gsl::span<uint8_t, 64> output,
                                    const Role role) = 0;
};

class KDF : public Dynamic
{
public:
    // plugin ID
    struct FENRIR_LOCAL ID :
                    type_safe::strong_typedef<ID, uint16_t>,
                    type_safe::strong_typedef_op::equality_comparison<ID>,
                    type_safe::strong_typedef_op::relational_comparison<ID>
        { using strong_typedef::strong_typedef; };
    KDF (const std::shared_ptr<Lib> from, Event::Loop *const loop,
                                        Loader *const loader, Random *const rnd)
        : Dynamic (from, loop, loader, rnd) {}
    KDF() = delete;
    KDF (const KDF&) = default;
    KDF& operator= (const KDF&) = default;
    KDF (KDF &&) = default;
    KDF& operator= (KDF &&) = default;
    virtual ~KDF () {}
    virtual KDF::ID id() const = 0;

    virtual bool init (const gsl::span<uint8_t, 64> key) = 0;

    virtual bool get (const uint64_t id, const std::array<char, 8> context,
                                            std::array<uint8_t, 64> &out) = 0;
};

} // namespace Crypto
} // namespace Impl
} // namespace Fenrir__v1
