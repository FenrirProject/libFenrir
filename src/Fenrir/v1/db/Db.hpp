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
#include "Fenrir/v1/data/Device_ID.hpp"
#include "Fenrir/v1/data/Token_t.hpp"
#include "Fenrir/v1/data/Username.hpp"
#include <vector>
#include <utility>

namespace Fenrir__v1 {
namespace Impl {


class FENRIR_LOCAL Db : public Dynamic
{
public:
    struct FENRIR_LOCAL ID :
                    type_safe::strong_typedef<ID, uint16_t>,
                    type_safe::strong_typedef_op::equality_comparison<ID>,
                    type_safe::strong_typedef_op::relational_comparison<ID>
        { using strong_typedef::strong_typedef; };

    Db() = delete;
    Db (const Db&) = default;
    Db& operator= (const Db&) = default;
    Db (Db &&) = default;
    Db& operator= (Db &&) = default;
    Db (const std::shared_ptr<Lib> from, Event::Loop *const loop,
                                                        Loader *const loader,
                                                        Random *const rnd)
        : Dynamic (from, loop, loader, rnd) {}
    virtual ~Db() {}

    virtual Db::ID id() const = 0;

    struct auth_data {
        User_ID _user_id;   // 0 = not found
        Token_t _token;     // saved token
        std::vector<uint8_t> _data; // additional auth data

         auth_data()
            : _user_id (User_ID{0}), _token()
        {}
    };

    // see issue #1 (constant time)
    // the DB should return data after the same time
    // wether the user exists or not. This can be tricky for
    // databases with lots of users, where finding the first is
    // easy but you need to go through the whole list to return
    // "non-existing".
    virtual struct auth_data get_auth (const Device_ID &device_id,
                                            const Service_ID &service,
                                            const Username &auth_user,
                                            const Username &service_user,
                                            const Crypto::Auth::ID id) = 0;
};

} // namespace Impl
} // namespace Fenrir__v1
