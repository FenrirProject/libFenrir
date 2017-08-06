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
namespace Impl {

enum class IO : uint8_t {   READ = 0x01,
                            WRITE= 0x02 };


FENRIR_LOCAL IO operator| (const IO a, const IO b);
FENRIR_LOCAL IO operator|= (const IO a, const IO b);
FENRIR_LOCAL IO operator& (const IO a, const IO b);
FENRIR_LOCAL IO operator&= (const IO a, const IO b);
FENRIR_LOCAL IO operator! (const IO a);
FENRIR_LOCAL bool io_has (const IO value, const IO test);

FENRIR_INLINE IO operator| (const IO a, const IO b)
{
    return static_cast<IO> (static_cast<uint8_t> (a) |
                                                    static_cast<uint8_t> (b));
}

FENRIR_INLINE IO operator|= (const IO a, const IO b)
{
    return static_cast<IO> (static_cast<uint8_t> (a) |
                                                    static_cast<uint8_t> (b));
}

FENRIR_INLINE IO operator& (const IO a, const IO b)
{
    return static_cast<IO> (static_cast<uint8_t> (a) &
                                                    static_cast<uint8_t> (b));
}

FENRIR_INLINE IO operator&= (const IO a, const IO b)
{
    return static_cast<IO> (static_cast<uint8_t> (a) &
                                                    static_cast<uint8_t> (b));
}

FENRIR_INLINE IO operator! (const IO a)
{
    switch (a) {
    case IO::READ:
        return IO::WRITE;
    case IO::WRITE:
        return IO::READ;
    }
}

FENRIR_INLINE bool io_has (const IO value, const IO test)
    { return (value & test) == test; }


} // namespace Impl
} // namespace Fenrir__v1

