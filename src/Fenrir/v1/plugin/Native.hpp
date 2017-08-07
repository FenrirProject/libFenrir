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
#include "Fenrir/v1/auth/Token.hpp"
#include "Fenrir/v1/crypto/Crypto_NULL.hpp"
#include "Fenrir/v1/crypto/Sodium.hpp"
#include "Fenrir/v1/db/Db_Fake.hpp"
#include "Fenrir/v1/plugin/Lib.hpp"
#include "Fenrir/v1/recover/ECC_NULL.hpp"
#include "Fenrir/v1/resolve/DNSSEC.hpp"
#include <memory>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {


namespace Event {
class Loop;
} // namespace Event
class Loader;
class Random;

FENRIR_INLINE std::vector<std::pair<Dynamic_Type, uint16_t>> Lib::list_native()
{
    std::vector<std::pair<Dynamic_Type, uint16_t>> ret ( NATIVE_CIPHER +
                                                                NATIVE_HMAC +
                                                                NATIVE_KEY +
                                                                NATIVE_KDF +
                                                                NATIVE_DB +
                                                                NATIVE_AUTH +
                                                                NATIVE_RATE +
                                                                NATIVE_RESOLV +
                                                                NATIVE_ECC
                                                                );
    for (uint16_t i = 1; i <= NATIVE_CIPHER; i++)
        ret.push_back ({Dynamic_Type::CRYPT, i});
    for (uint16_t i = 1; i <= NATIVE_HMAC; i++)
        ret.push_back ({Dynamic_Type::HMAC, i});
    for (uint16_t i = 1; i <= NATIVE_KEY; i++)
        ret.push_back ({Dynamic_Type::KEY, i});
    for (uint16_t i = 1; i <= NATIVE_KDF; i++)
        ret.push_back ({Dynamic_Type::KDF, i});
    for (uint16_t i = 1; i <= NATIVE_DB; i++)
        ret.push_back ({Dynamic_Type::DB, i});
    for (uint16_t i = 1; i <= NATIVE_AUTH; i++)
        ret.push_back ({Dynamic_Type::AUTH, i});
    for (uint16_t i = 1; i <= NATIVE_RATE; i++)
        ret.push_back ({Dynamic_Type::RATE, i});
    for (uint16_t i = 1; i <= NATIVE_RESOLV; i++)
        ret.push_back ({Dynamic_Type::RESOLV, i});
    for (uint16_t i = 1; i <= NATIVE_ECC; i++)
        ret.push_back ({Dynamic_Type::ECC, i});
    return ret;
}

FENRIR_INLINE std::shared_ptr<Dynamic> Lib::get_shared_native (
                                                Event::Loop *const loop,
                                                Loader *const loader,
                                                Random *const rnd,
                                                const std::shared_ptr<Lib> self,
                                                const Dynamic_Type type,
                                                const uint16_t id)
{
    // one-case switches. I know. It's just to keep the structure
    // for future plugins. Compilers will optimize this anyway
    FENRIR_UNUSED(loop);
    FENRIR_UNUSED(rnd);
    FENRIR_UNUSED(self);
    std::shared_ptr<Dynamic> ret (nullptr);
    switch (type) {
    case Dynamic_Type::CRYPT:
        switch (id) {
        case 1:
            ret = std::make_shared<Crypto::Crypto_NULL>();
            ret->_self = ret;
            break;
        case 2:
            ret = std::make_shared<Crypto::ChaCha20_Poly1305_IETF>();
            ret->_self = ret;
        }
        break;
    case Dynamic_Type::HMAC:
        switch (id) {
        case 1:
            ret = std::make_shared<Crypto::Hmac_NULL>();
            ret->_self = ret;
        }
        break;
    case Dynamic_Type::KEY:
        switch (id) {
        case 1:
            ret = std::make_shared<Crypto::Ed25519>();
            ret->_self = ret;
        }
        break;
    case Dynamic_Type::KDF:
        switch (id) {
        case 1:
            ret = std::make_shared<Crypto::blake2b>();
            ret->_self = ret;
        }
        break;
    case Dynamic_Type::DB:
        switch (id) {
        case 1:
            ret = std::make_shared<Db_Fake>();
            ret->_self = ret;
        }
        break;
    case Dynamic_Type::AUTH:
        switch (id) {
        case 1:
            ret = std::make_shared<Crypto::Token>();
            ret->_self = ret;
        }
        break;
    case Dynamic_Type::RATE:
            return nullptr; // no default algorithm yet
    case Dynamic_Type::RESOLV:
        switch (id) {
        case 1:
            ret = std::make_shared<Resolve::DNSSEC> (loop, loader, rnd);
            ret->_self = ret;
        }
        break;
    case Dynamic_Type::ECC:
        switch (id) {
        case 1:
            ret = std::make_shared<Recover::ECC_NULL>();
            ret->_self = ret;
        }
        break;
    }
    return ret;
}

} // namespace Impl
} // namespace Fenrir__v1
