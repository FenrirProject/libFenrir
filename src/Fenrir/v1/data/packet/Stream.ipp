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

#include "Fenrir/v1/data/packet/Stream.hpp"

namespace Fenrir__v1 {
namespace Impl {

Stream::Fragment operator| (const Stream::Fragment a,
                                                    const Stream::Fragment b)
{
    return static_cast<Stream::Fragment> (static_cast<uint8_t> (a) |
                                                    static_cast<uint8_t> (b));
}
Stream::Fragment operator|= (const Stream::Fragment a,
                                                    const Stream::Fragment b)
{
    return static_cast<Stream::Fragment> (static_cast<uint8_t> (a) |
                                                    static_cast<uint8_t> (b));
}

Stream::Fragment operator& (const Stream::Fragment a,
                                                    const Stream::Fragment b)
{
    return static_cast<Stream::Fragment> (static_cast<uint8_t> (a) &
                                                    static_cast<uint8_t> (b));
}
Stream::Fragment operator&= (const Stream::Fragment a,
                                                    const Stream::Fragment b)
{
    return static_cast<Stream::Fragment> (static_cast<uint8_t> (a) &
                                                    static_cast<uint8_t> (b));
}

Stream::Fragment operator! (const Stream::Fragment a)
{
    switch (a) {
    case Stream::Fragment::START:
        return Stream::Fragment::END;
    case Stream::Fragment::END:
        return Stream::Fragment::START;
    case Stream::Fragment::MIDDLE:
        return Stream::Fragment::FULL;
    case Stream::Fragment::FULL:
        return Stream::Fragment::MIDDLE;
    }
}

bool fragment_has (const Stream::Fragment value, const Stream::Fragment test)
    { return (value & test) == test; }

} // namespace Impl
} // namespace Fenrir__v1
