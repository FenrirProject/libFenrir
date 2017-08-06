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
#include "Fenrir/v1/data/packet/Stream.hpp"
#include <type_safe/strong_typedef.hpp>

namespace Fenrir__v1 {
namespace Impl {

struct FENRIR_LOCAL Handshake_ID :
        type_safe::strong_typedef<Handshake_ID, std::pair<Counter, Stream_ID>>,
        type_safe::strong_typedef_op::equality_comparison<Handshake_ID>
{
    using strong_typedef::strong_typedef;
    bool operator< (const Handshake_ID id) const
    {
        using ctr_stream = std::pair<Counter, Stream_ID>;

        if (std::get<Counter> (static_cast<ctr_stream> (*this)) ==
                        std::get<Counter> (static_cast<ctr_stream> (id))) {
            return static_cast<bool> (
                    std::get<Stream_ID> (static_cast<ctr_stream> (*this)) <
                    std::get<Stream_ID> (static_cast<ctr_stream> (id)));
        }
        return static_cast<bool> (
                        std::get<Counter> (static_cast<ctr_stream> (*this)) <
                        std::get<Counter> (static_cast<ctr_stream> (id)));
    }
};


} // namespace Impl
} // namespace Fenrir__v1
