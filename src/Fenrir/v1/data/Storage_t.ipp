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

#include "Fenrir/v1/data/Storage_t.hpp"

namespace Fenrir__v1 {
namespace Impl {

Storage_t operator| (const Storage_t a, const Storage_t b)
{
    return static_cast<Storage_t> (static_cast<uint8_t> (a) |
                                                    static_cast<uint8_t> (b));
}

Storage_t operator& (const Storage_t a, const Storage_t b)
{
    return static_cast<Storage_t> (static_cast<uint8_t> (a) &
                                                    static_cast<uint8_t> (b));
}

bool storage_t_has (const Storage_t value, const Storage_t test)
    { return (value & test) == test; }

bool storage_t_validate (const Storage_t value)
{
    Storage_t both;
    both = Storage_t::MULTICAST | Storage_t::UNICAST;
    if ((value & both) == both || (value & both) == Storage_t::NOT_SET)
        return false;
    both = Storage_t::UNRELIABLE | Storage_t::RELIABLE;
    if ((value & both) == both || (value & both) == Storage_t::NOT_SET)
        return false;
    both = Storage_t::UNORDERED | Storage_t::ORDERED;
    if ((value & both) == both || (value & both) == Storage_t::NOT_SET)
        return false;
    both = Storage_t::INCOMPLETE | Storage_t::COMPLETE;
    if ((value & both) == both || (value & both) == Storage_t::NOT_SET)
        return false;
    return true;
}

} // namespace Impl
} // namespace Fenrir__v1
