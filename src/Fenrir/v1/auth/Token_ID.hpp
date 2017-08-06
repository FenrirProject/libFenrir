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
#include <array>
#include <type_safe/strong_typedef.hpp>

namespace Fenrir__v1 {
namespace Impl {

struct FENRIR_LOCAL Token_ID :
                type_safe::strong_typedef<Token_ID, std::array<uint8_t, 16>>
{
    using strong_typedef::strong_typedef;
    bool operator== (const Token_ID &rhs)
    {
        size_t diff = 0;
        for (size_t idx = 0; idx < 16; ++idx) {
            if (static_cast<std::array<uint8_t, 16>> (*this)[idx] !=
                            static_cast<std::array<uint8_t, 16>> (rhs)[idx]) {
                ++diff;
            }
        }
        return diff == 0;
    }

    bool operator< (const Token_ID &rhs)
    {
        size_t diff = 0;
        for (size_t idx = 0; idx < 16; ++idx) {
            if (static_cast<std::array<uint8_t, 16>> (*this)[idx] <
                            static_cast<std::array<uint8_t, 16>> (rhs)[idx]) {
                if (diff == 0)
                    ++diff;
            }
        }
        return diff == 0;
    }
};

} // namespace Impl
} // namespace Fenrir__v1
