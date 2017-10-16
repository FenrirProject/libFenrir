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
#include "Fenrir/v1/auth/Lattice.hpp"
#include "Fenrir/v1/data/Storage_t.hpp"
#include "Fenrir/v1/data/packet/Stream.hpp"
#include "Fenrir/v1/service/Service_ID.hpp"
#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <type_safe/strong_typedef.hpp>
#include <vector>
#include <tuple>

namespace Fenrir__v1 {
namespace Impl {

// a service is identified by its domain and its service_ID
// each service can create automatically a number of streams with different
// priorities. Different instances of the same service mught be able to handle
// a different number of streams, so this is a per-service setting.
//
// The same applies to the Lattices.
// Naively we can think that a Service_ID and a Lattice are interdependent,
// but protocols evolve, and we might have two services with slightly different
// lattices.
// This also implies that there will be a versioning / compatibility list
// between lattices. Nodes in the lattices are identified by a 8-bit ID, so
// extending an existing lattice while preserving the IDs should not be
// a problem. (TODO: versioning/compatiblity of lattices
// (hint: wither major/minor for compatibility or something tree-like)


using stream_info = std::vector<std::tuple<Storage_t, Stream_PRIO>>;
using Service_Info = std::tuple<stream_info, std::shared_ptr<Lattice>>;

} // namespace Impl
} // namespace Fenrir__v1
