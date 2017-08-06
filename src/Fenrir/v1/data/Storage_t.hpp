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

enum class FENRIR_LOCAL Storage_t : uint8_t {
                                            NOT_SET    = 0x00,
                                            COMPLETE   = 0x01,
                                            INCOMPLETE = 0x02,
                                            ORDERED    = 0x04,
                                            UNORDERED  = 0x08,
                                            RELIABLE   = 0x10,
                                            UNRELIABLE = 0x20,
                                            UNICAST    = 0x40,
                                            MULTICAST  = 0x80,
                                            };

FENRIR_LOCAL Storage_t operator| (const Storage_t a, const Storage_t b);
FENRIR_LOCAL Storage_t operator& (const Storage_t a, const Storage_t b);

FENRIR_LOCAL bool storage_t_validate (const Storage_t value);
FENRIR_LOCAL bool storage_t_has (const Storage_t value, const Storage_t test);

} // namespace Impl
} // namespace Fenrir__v1

