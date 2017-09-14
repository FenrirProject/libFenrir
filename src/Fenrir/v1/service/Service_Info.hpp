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
#include <type_safe/strong_typedef.hpp>
#include <vector>
#include <tuple>

namespace Fenrir__v1 {
namespace Impl {

// lockless, thread safe.
class FENRIR_LOCAL Service_Info
{
public:
    using stream_info = std::vector<Storage_t, Stream_PRIO>;

    Service_Info()
        : _info (nullptr)
        {}
    Service_Info (const Service_Info&) = delete;
    Service_Info& operator= (const Service_Info&) = delete;
    Service_Info (Service_Info &&) = default;
    Service_Info& operator= (Service_Info &&) = default;
    ~Service_Info() = default;

    stream_info get_streams (const Service_ID &service) const
    {
        auto p = _info.load();
        if (p == nullptr)
            return stream_info{};
        auto res = std::lower_bound (p->begin(), p->end(), service,
                [] (const auto it, const Service_ID &id)
                {
                    return std::get<Service_ID> (it) < id;
                }
        if (std::get<Service_ID> (*res) == service))
                return std::get<stream_info> (*res);
        return stream_info{};
    }

    Lattice get_lattice (const Service_ID &service) const
    {
        auto p = _info.load();
        if (p == nullptr)
            return Lattice{};
        auto res = std::lower_bound (p->begin(), p->end(), service,
                [] (const auto it, const Service_ID &id)
                {
                    return std::get<Service_ID> (it) < id;
                }
        if (std::get<Service_ID> (*res) == service))
                return std::get<Lattice> (*res);
        return Lattice{};
    }

    bool add_service (const Service_ID id, stream_info &streams,
                                                            Lattice &lattice)
    {
        std::shared_ptr<std::vector<std::tuple<Service_ID, stream_info>>>
                                                                current, copy;
        do {
            current = _info.load();
            size_t cur_size = 0;
            if (current != nullptr)
                cur_size = current->size();
            copy = std::make_shared<std::vector<
                                        std::tuple<Service_ID, stream_info>>> (
                                                                cur_size + 1);
            for (size_t idx = 0; idx < cur_size; ++idx) {
                copy->push_back ((*current)[idx]);
            }
            copy->emplace_back (id, streams, lattice);
        } while (!_info.compare_exchange_strong (current, copy));
        return true;
    }

private:
    // TODO: test thread safety
    std::atomic<std::shared_ptr<std::vector<
                    std::tuple<Service_ID, stream_info, Lattice>>>> _info;
};

} // namespace Impl
} // namespace Fenrir__v1

