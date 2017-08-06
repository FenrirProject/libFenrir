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

#include <algorithm>
#include <array>
#include <vector>
#include "Fenrir/v1/common.hpp"
#include "Fenrir/v1/util/hash.hpp"
#include <gsl/span>
#include <type_safe/strong_typedef.hpp>

namespace Fenrir__v1 {
namespace Impl {

// Strong typedef for User id,to avoid up/down casting, and other
// operations which do not make sense for an identifier.
struct FENRIR_LOCAL User_ID : type_safe::strong_typedef<User_ID, uint64_t>,
                    type_safe::strong_typedef_op::equality_comparison<User_ID>
    { using strong_typedef::strong_typedef; };

// TODO: we check the value of '@', but that's the ASCII value.
// is that a big problem for other encodings? Get locale & fix (warn:multibyte?)

class FENRIR_LOCAL Username
{
public:
    Username() = default;
    Username (const Username&) = default;
    Username& operator= (const Username&) = default;
    Username (Username &&) = default;
    Username& operator= (Username &&) = default;
    ~Username() = default;

    // Not a string. NOT NULL-terminated.
    Username (const gsl::span<const uint8_t> raw)
    {
        // check the index: no '@', '@' at first char, or domain of less
        // than 4 chars

        // usernames/domains might get converted into strings.
        // avoid problems. TODO: charset
        const std::array<uint8_t, 4> bad = {{'\n', '\r', '\0', '\t' }};

        size_t count_at = 0;
        for (auto r_it = raw.begin(); r_it != raw.end(); ++r_it) {
            if (std::any_of (bad.begin(), bad.end(),
                            [r_it] (uint8_t tmp) { return tmp == *r_it; })) {
                return;
            } else {
                if (*r_it == '@')
                    ++count_at;
            }
        }
        if (count_at != 1)
            return;

        auto it = std::find (raw.begin(), raw.end(), static_cast<uint8_t>('@'));
        if (it == raw.end() || it == raw.begin() || (raw.end() - it) <= 4)
            return;

        user.reserve (static_cast<size_t> (it - raw.begin()));
        std::copy (raw.begin(), it, std::back_inserter(user));
        domain.reserve (static_cast<size_t> (raw.end() - it));
        ++it;
        std::copy (it, raw.end(), std::back_inserter(domain));
    }

    Error operator() (gsl::span<uint8_t> out) const
    {
        // NOTE: no trailing '\0'
        auto length = size();
        if (length == 0 || length != static_cast<size_t> (out.size()))
            return Error::WRONG_INPUT;

        auto it = out.begin();
        it = std::copy (user.begin(), user.end(), it);
        *it = '@';
        ++it;
        std::copy (domain.begin(), domain.end(), it);

        return Error::NONE;
    }

    explicit operator bool() const
        { return user.size() != 0 && domain.size() > 4; }
    bool operator== (const Fenrir__v1::Impl::Username& user2) const
    {
        return user.size() == user2.user.size() &&
                domain.size() == user2.domain.size() &&
                std::equal (user.begin(), user.end(), user2.user.begin()) &&
                std::equal (domain.begin(), domain.end(), user2.domain.begin());
    }

    size_t size() const {
        if (user.size() == 0 || domain.size() == 0)
            return 0;
        return user.size() + 1 + domain.size();
    }

    bool is_anonymous() const {
        std::array<uint8_t, 9> anon = {{ 'a','n','o','n','y','m','o','u','s' }};
        return anon.size() == user.size() &&
                            std::equal (user.begin(), user.end(), anon.begin());
    }
    // NOTE: no trailing '\0'
    std::vector<uint8_t> user;
    std::vector<uint8_t> domain;
};

} // namespace Impl
} // namespace Fenrir__v1
