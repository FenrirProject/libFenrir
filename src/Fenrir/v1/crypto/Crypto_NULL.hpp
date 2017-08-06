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

#include <memory>
#include "Fenrir/v1/crypto/Crypto.hpp"

namespace Fenrir__v1 {
namespace Impl {
namespace Crypto {

class FENRIR_LOCAL Crypto_NULL final : public Encryption
{
public:
    Crypto_NULL ()
        : Encryption (std::shared_ptr<Lib> (nullptr), nullptr, nullptr, nullptr)
    {}
    Crypto_NULL (const Crypto_NULL&) = default;
    Crypto_NULL& operator= (const Crypto_NULL&) = default;
    Crypto_NULL (Crypto_NULL &&) = default;
    Crypto_NULL& operator= (Crypto_NULL &&) = default;
    ~Crypto_NULL() {}
    void parse_event (std::shared_ptr<Event::Plugin_Timer> ev) override
        { FENRIR_UNUSED (ev); }

    Encryption::ID id() const override
        { return Encryption::ID (1); }
    uint16_t bytes_header() const override
        { return 0; }
    uint16_t bytes_footer() const override
        { return 0; }
    uint16_t bytes_overhead() const override
        { return 0; }
    bool set_key (const std::array<uint8_t, 64> &key) override
    {
        FENRIR_UNUSED (key);
        return true;
    }

    Impl::Error encrypt (gsl::span<uint8_t> in) override
    {
        FENRIR_UNUSED (in);
        return Impl::Error::NONE;
    }

    Impl::Error decrypt (const gsl::span<uint8_t> in,
                                            gsl::span<uint8_t> &out) override
    {
        out = in;
        return Impl::Error::NONE;
    }
    bool is_authenticated() const override
        { return false; }
};

class FENRIR_LOCAL Hmac_NULL final : public Hmac
{
public:
    Hmac_NULL ()
        : Hmac (std::shared_ptr<Lib> (nullptr), nullptr, nullptr, nullptr)
    {}
    Hmac_NULL (const Hmac_NULL&) = default;
    Hmac_NULL& operator= (const Hmac_NULL&) = default;
    Hmac_NULL (Hmac_NULL &&) = default;
    Hmac_NULL& operator= (Hmac_NULL &&) = default;
    ~Hmac_NULL() {}
    void parse_event (std::shared_ptr<Event::Plugin_Timer> ev) override
        { FENRIR_UNUSED (ev); }

    Hmac::ID id() const override
        { return Hmac::ID (1); }
    uint16_t bytes_header() const override
        { return 0; }
    uint16_t bytes_footer() const override
        { return 0; }
    uint16_t bytes_overhead() const override
        { return 0; }
    bool set_key (const std::array<uint8_t, 64> &key) override
    {
        FENRIR_UNUSED(key);
        return true;
    }
    bool is_valid (const gsl::span<uint8_t> in, gsl::span<uint8_t> &out)override
    {
        out = in;
        return true;
    }
    Impl::Error add_hmac (gsl::span<uint8_t> in) override
    {
        FENRIR_UNUSED (in);
        return Impl::Error::NONE;
    }
};

} // namespace Crypto
} // namespace Impl
} // namespace Fenrir__v1
