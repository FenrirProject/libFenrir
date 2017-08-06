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

namespace Fenrir__v1 {
namespace Impl {


enum class Conn0_Type : uint8_t {
    C_INIT = 0x00,      // 3-RTT handshake client cookie request
    S_COOKIE = 0x01,    // 3-RTT handshake server cookie
    C_COOKIE = 0x02,    // 3-RTT cookie and supported algorithms
    S_KEYS = 0x03,      // 3-RTT server keys and selected algorithm
    C_AUTH = 0x04,      // 3-RTT client keys and authentication
    S_RESULT = 0x05,    // 3-RTT authentication result
};

} // namespace Impl
} // namespace Fenrir__v1
