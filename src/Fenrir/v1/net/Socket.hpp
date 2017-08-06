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

#include <functional> // hash
#ifdef _WIN32
    #include <In6addr.h>
    #include <Inaddr.h>
#else
    #include <netinet/in.h>
#endif

#include <netinet/in.h>
#include <sys/socket.h>
#include <utility> // pair
#include "Fenrir/v1/common.hpp"
#include "Fenrir/v1/data/IP.hpp"
#include "Fenrir/v1/util/hash.hpp"

namespace std {
template <> struct hash<Fenrir__v1::Impl::IP>
{
	size_t operator() (const Fenrir__v1::Impl::IP &ip) const
	{
		uint64_t *tmp1, *tmp2;
		tmp1 = reinterpret_cast<uint64_t *> (const_cast<struct in6_addr *>
																(&ip.ip.v6));
		tmp2 = tmp1 + 1;
		return hash<pair<uint64_t,uint64_t>>()({*tmp1, *tmp2});
	}
};
}
#ifdef _WIN32
    #include "Fenrir/v1/net/platforms/Socket_Windows.hpp"
#else
    #include "Fenrir/v1/net/platforms/Socket_Unix.hpp"
#endif

