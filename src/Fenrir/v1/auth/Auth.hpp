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

#include <vector>
#include "Fenrir/v1/common.hpp"
#include "Fenrir/v1/data/Device_ID.hpp"
#include "Fenrir/v1/data/Username.hpp"
#include "Fenrir/v1/plugin/Dynamic.hpp"
#include "Fenrir/v1/util/Random.hpp"
#include "Fenrir/v1/service/Service_ID.hpp"
#include <gsl/span>
#include <type_safe/strong_typedef.hpp>

namespace Fenrir__v1 {
namespace Impl {

class Db;

namespace Crypto {

// Info to return to the user. Used by the services
class FENRIR_LOCAL Auth_Result
{
public:
    Auth_Result()
        : _user_id {0}, _anonymous (true), _failed (true) {}

    Auth_Result (const Auth_Result&) = default;
    Auth_Result& operator= (const Auth_Result&) = default;
    Auth_Result (Auth_Result &&) = default;
    Auth_Result& operator= (Auth_Result &&) = default;
    ~Auth_Result() = default;

    User_ID _user_id;
    bool _anonymous, _failed;
};

class FENRIR_LOCAL Auth : public Dynamic
{
public:
    struct FENRIR_LOCAL ID :
                    type_safe::strong_typedef<ID, uint16_t>,
                    type_safe::strong_typedef_op::equality_comparison<ID>,
                    type_safe::strong_typedef_op::relational_comparison<ID>
        { using strong_typedef::strong_typedef; };
    Auth (const std::shared_ptr<Lib> from, Event::Loop *const loop,
                                        Loader *const loader, Random *const rnd)
        : Dynamic (from, loop, loader, rnd) {}
    Auth() = delete;
    Auth (const Auth&) = delete;
    Auth& operator= (const Auth&) = delete;
    Auth (Auth &&) = default;
    Auth& operator= (Auth &&) = default;
    virtual ~Auth() {}

    // see issue #1 (constant time) when implementing
    virtual Auth_Result authenticate (const Device_ID &dev_id,
                                    const Service_ID &service,
                                    const Username &auth_user,
                                    const Username &service_user,
                                    const gsl::span<uint8_t> data, Db *db) = 0;
    virtual Auth::ID id() const = 0;	// plugin id
};

} // namespace Auth
} // namespace Impl
} // namespace Fenrir__v1

