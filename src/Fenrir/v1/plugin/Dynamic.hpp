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
#include <memory>
#include <type_traits>

namespace Fenrir__v1 {
namespace Impl {

class Random;
class Lib;
class Loader;

namespace Event {
class Loop;
class Plugin_Timer;
} // namespace Event

enum class FENRIR_LOCAL Dynamic_Type : uint32_t {
    CRYPT  = 0x01,
    HMAC   = 0x02,
    KEY    = 0x03,
    KDF    = 0x04,
    DB     = 0x05,
    AUTH   = 0x06,
    RATE   = 0x07,
    RESOLV = 0x08,
    ECC    = 0x09
};

class Db;

namespace Crypto {
class Auth;
class Encryption;
class Hmac;
class Key;
class KDF;
}
namespace Rate {
class Rate;
}
namespace Resolve {
class Resolver;
}
namespace Recover {
class ECC;
}

template<typename T>
inline constexpr Dynamic_Type Dynamic_Type_From_Template()
{
    if (std::is_same<T, Crypto::Encryption>::value)
        return Dynamic_Type::CRYPT;
    if (std::is_same<T, Crypto::Hmac>::value)
        return Dynamic_Type::HMAC;
    if (std::is_same<T, Crypto::Key>::value)
        return Dynamic_Type::KEY;
    if (std::is_same<T, Crypto::KDF>::value)
        return Dynamic_Type::KDF;
    if (std::is_same<T, Db>::value)
        return Dynamic_Type::DB;
    if (std::is_same<T, Crypto::Auth>::value)
        return Dynamic_Type::AUTH;
    if (std::is_same<T, Rate::Rate>::value)
        return Dynamic_Type::RATE;
    if (std::is_same<T, Resolve::Resolver>::value)
        return Dynamic_Type::RESOLV;
    if (std::is_same<T, Recover::ECC>::value)
        return Dynamic_Type::ECC;
    return Dynamic_Type::ECC;
}

class FENRIR_LOCAL Dynamic {
public:
    // ptr to lib: used to close library only when all plugins are
    // unloaded
    const std::shared_ptr<Lib> _lib;
    // weak_ptr to self: used by events to destroy class only when
    // all events have been destroyed
    std::weak_ptr<Dynamic> _self;
    // empty class. extend this when you want to load something from
    // a dynamic library

    Dynamic () = delete;
    Dynamic (const Dynamic&) = default;
    Dynamic& operator= (const Dynamic&) = default;
    Dynamic (Dynamic &&) = default;
    Dynamic& operator= (Dynamic &&) = default;
    virtual ~Dynamic() = default;

    explicit Dynamic (const std::shared_ptr<Lib> from, Event::Loop *const loop,
                                                        Loader *const loader,
                                                        Random *const rnd)
        : _lib (std::move(from)), _loop (loop), _loader (loader), _rnd (rnd)
    {}

    virtual void parse_event (std::shared_ptr<Event::Plugin_Timer> ev) = 0;
protected:
    Event::Loop *const _loop;
    Loader *const _loader;
    Random *const _rnd;
};

} // namespace Impl
} // namespace Fenrir__v1
