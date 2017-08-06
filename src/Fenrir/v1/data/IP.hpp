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
#include <array>
#include <cstring>
#ifdef _WIN32
    #include <In6addr.h>
    #include <Inaddr.h>
#else
    #include <netinet/in.h>
#endif
#include <type_safe/strong_typedef.hpp>

namespace {
// volatile: compilers *should* not optimize out this.
static void * (* const volatile memset_ptr)(void *, int, size_t) = memset;
} // empty namespace

namespace Fenrir__v1 {
namespace Impl {

// Strong typedef for Connection id,to avoid up/down casting, and other
// operations which do not make sense for an identifier.
struct FENRIR_LOCAL  UDP_Port : type_safe::strong_typedef<UDP_Port, uint16_t>,
                type_safe::strong_typedef_op::equality_comparison<UDP_Port>,
                type_safe::strong_typedef_op::relational_comparison<UDP_Port>
    { using strong_typedef::strong_typedef; };

class FENRIR_LOCAL IP
{
public:
    union {
        struct in_addr v4;
        struct in6_addr v6;
    } ip;
    bool ipv6;    // discriminator for the union

    IP()
    {
        (memset_ptr) (&ip, 0, sizeof(ip));
        ipv6 = true;
    }
    IP (const uint8_t *raw_ip, const bool is_ipv6)
    {
        // TODO: big/little endian?
        uint8_t *raw = reinterpret_cast<uint8_t *> (&ip);
        uint8_t size = (is_ipv6 ? 16 : 4);
        uint8_t idx = 0;
        for (; idx < size; ++idx)
            raw[idx] = *(raw_ip++);
        while (idx < 16)
            raw[idx++] = 0;
        ipv6 = is_ipv6;
    }
    IP (const IP&) = default;
    IP& operator= (const IP&) = default;
    IP (IP &&) = default;
    IP& operator= (IP &&) = default;
    ~IP() = default;

    explicit operator bool() const
    {
        // paranoia mode on: check that the bool value only has 2 values.
        uint8_t data = static_cast<uint8_t> (ipv6);
        if (data != static_cast<uint8_t> (true) &&
                                        data != static_cast<uint8_t> (false)){
            return false;
        }
        // now check that if this is a ipv4 address the last bytes are set to 0
        constexpr uint32_t v6_v4_diff = (sizeof (ip.v6) - sizeof (ip.v4));
        if (!ipv6) {
            std::array<char, v6_v4_diff> zero {{0x00, 0x00, 0x00, 0x00,
                                                0x00, 0x00, 0x00, 0x00,
                                                0x00, 0x00, 0x00, 0x00}};
            const char *p = reinterpret_cast<const char*> (&ip) + sizeof(ip.v4);
            if (!std::equal (zero.begin(), zero.end(), p, p + v6_v4_diff))
                return false;
        }
        return true;
    }

    bool operator== (const IP &ip2) const
    {
        return (ipv6 == ip2.ipv6) &&
                (!memcmp( &ip.v4, &ip2.ip.v4, sizeof(ip.v4))) &&
                (!ipv6 || (!memcmp(
                    reinterpret_cast<const char *>(&ip.v6) + sizeof(ip.v4),
                    reinterpret_cast<const char *>(&ip2.ip.v6) + sizeof(ip.v4),
                                                sizeof(ip) - sizeof(ip.v4))));
    }
    bool operator!= (const IP &ip2) const
    {
        return ! operator== (ip2);
    }
    bool operator< (const IP &ip2) const
    {
        if (ipv6 && !ip2.ipv6)
            return false;
        if (!ipv6 && ip2.ipv6)
            return true;
        if (!ipv6) {
            const uint32_t *p1 = reinterpret_cast<const uint32_t*> (&ip.v4);
            const uint32_t *p2 = reinterpret_cast<const uint32_t*> (&ip2.ip.v4);

            return *p1 < *p2;
        } else {
            const uint64_t *p1 = reinterpret_cast<const uint64_t*> (&ip.v6);
            const uint64_t *p2 = reinterpret_cast<const uint64_t*> (&ip2.ip.v6);
            #ifdef FENRIR_BIG_ENDIAN
                if (*p1 == *p2) {
                    ++p1;
                    ++p2;
                }
            #else
                ++p1;
                ++p2;
                if (*p1 == *p2) {
                    --p1;
                    --p2;
                }
            #endif
            return *p1 < *p2;
        }
    }
};

} // namespace Impl
} // namespace Fenrir__v1

