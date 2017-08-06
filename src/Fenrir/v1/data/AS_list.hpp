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
#include "Fenrir/v1/auth/Auth.hpp"
#include "Fenrir/v1/crypto/Crypto.hpp"
#include "Fenrir/v1/data/IP.hpp"
#include "Fenrir/v1/net/Link_defs.hpp"
#include "Fenrir/v1/util/Random.hpp"
#include "type_safe/optional.hpp"
#include <algorithm>
#include <cassert>
#include <memory>
#include <utility>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {


namespace Resolve {

class FENRIR_LOCAL AS_list
{
public:
    AS_list() {}
    AS_list (const AS_list&) = delete;
    AS_list& operator= (const AS_list&) = delete;
    AS_list (AS_list &&) = default;
    AS_list& operator= (AS_list &&) = default;
    ~AS_list() = default;

    std::vector<uint8_t> _fqdn;
    std::vector<std::pair<Crypto::Key::Serial, std::shared_ptr<Crypto::Key>>>
                                                                        _key;

    class Srv_info {
    public:
        Srv_info (const IP _ip, const uint16_t _port, const uint8_t _priority,
                                                        const uint8_t _weight)
            : ip (_ip), port (_port), priority (_priority), weight (_weight)
        {}
        IP ip;
        UDP_Port port;
        uint8_t priority;
        uint8_t weight;
    };

    std::vector<Srv_info> _servers;
    std::vector<Crypto::Auth::ID> _auth_protocols;
    Impl::Error _err;

    void sort () {
            std::sort (_servers.begin (), _servers.end (),
                [] (const Srv_info &a, const Srv_info &b) {
                    assert (a.priority < 8 && a.weight < 8 &&
                            b.priority < 8 && b.weight < 8);
                    if (a.priority < b.priority)
                        return true;
                    if (a.priority == b.priority) {
                        if (a.weight < b.weight)
                            return true;
                    }
                    return false;
                });
    }

    type_safe::optional<uint16_t> get_idx (const uint32_t fallback,
                                                            Random *const rnd)
    {
        if (_servers.size() == 0)
            return type_safe::nullopt;

        // select one of the servers.
        // "fallback" regulates how many "priorities" to skip.
        // then choose one randomly from the weight section
        int16_t last_priority = -1;
        auto skip = fallback;
        uint32_t idx_from;
        for (idx_from = 0; idx_from < _servers.size() && skip > 0; ++idx_from) {
            if (last_priority < _servers[idx_from].priority) {
                last_priority = _servers[idx_from].priority;
                --skip;
            }
        }
        if (idx_from >= _servers.size())
            return type_safe::nullopt;
        while (idx_from < _servers.size() &&
                                last_priority == _servers[idx_from].priority) {
            ++idx_from;
        }
        if (idx_from >= _servers.size())
            return type_safe::nullopt;
        uint32_t weight_sum = 0;
        auto idx_to = idx_from;
        while (idx_to < _servers.size() &&
                                    last_priority == _servers[idx_to].priority){
            weight_sum += _servers[idx_to].weight;
            ++idx_from;
        }
        --idx_to;

        auto selected = rnd->uniform<uint32_t> (0, weight_sum);

        while (idx_from != idx_to) {
            if (selected <= _servers[idx_from].weight)
                break;
            selected -= _servers[idx_from].weight;
        }
        return static_cast<uint16_t> (idx_from);
    }
};

} // namespace Resolve
} // namespace Impl
} // namespace Fenrir__v1
