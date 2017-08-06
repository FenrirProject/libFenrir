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

namespace Fenrir__v1 {
namespace Report {

// TODO: template
// template<typename T>
class FENRIR_API Base {
public:
    enum class ID : uint32_t {
        RESOLVE
    };
    const ID _type;
    // FIXME: everything is async, we need to somehow identify the user request
    // return a uint64_t counter on user request?
    //   the user needs to find() he reuqest in some structure
    // return an object
    //   the user needs to wait() on the object, or scan through them
    // get an ID from the user (64 bits).
    //   could be a pointer to something, The user would not need to scan
    //   any structure. But the user can only give a raw pointer, not a
    //   shared one. Shared pointer to ourselves trick.
    //   what about weak_ptr? this can not be a template. ... or can it?
    //     base could be a template on raw_ptr, shared_ptr or weak_ptr.
    //     the type would discriminate on the ptr type. At this point just give
    //     a general argument.

    // final solution: user provides a uint8_t enum. enum tells what to
    // static_cast<> to?

    Base () = delete;
    Base (const ID type)
        : _type (type) {}
};

class FENRIR_API Resolve final : public Base {
public:
    Resolve (Fenrir__v1::Error err)
        : Base (Base::ID::RESOLVE), _err (err) {}
    Fenrir__v1::Error _err;
};

} // namespace Report
} // namespace Fenrir__v1
