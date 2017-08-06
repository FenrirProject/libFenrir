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
#include "Fenrir/v1/auth/Auth.hpp"
#include "Fenrir/v1/data/Device_ID.hpp"
#include "Fenrir/v1/data/Token_t.hpp"
#include "Fenrir/v1/data/Username.hpp"
#include "Fenrir/v1/db/Db.hpp"
#include <vector>
#include <utility>

namespace Fenrir__v1 {
namespace Impl {

class FENRIR_LOCAL Db_Fake final : public Db
{
public:
    Db_Fake()
        : Db(std::shared_ptr<Lib> (nullptr), nullptr, nullptr, nullptr)
        {}
    Db_Fake (const Db_Fake&) = default;
    Db_Fake& operator= (const Db_Fake&) = default;
    Db_Fake (Db_Fake &&) = default;
    Db_Fake& operator= (Db_Fake &&) = default;
    ~Db_Fake() {}
    Db::ID id() const override
        { return Db::ID {1}; }

    void parse_event (std::shared_ptr<Event::Plugin_Timer> ev) override
        { FENRIR_UNUSED (ev); }

    struct auth_data get_auth (const Device_ID &device_id,
                                            const Service_ID &service,
                                            const Username &auth_user,
                                            const Username &service_user,
                                            const Crypto::Auth::ID id) override
    {
        FENRIR_UNUSED (id);
        const std::array<uint8_t, 16> test {{ 'u','s','e','r','@',
                                'e','x','a','m','p','l','e','.','c','o','m' }};
        const Username only_user {test};

        Device_ID only_dev_id   {{{ 4,4,4,4,4,4,4,4,
                                    4,4,4,4,4,4,4,4 }}};
        Service_ID only_service {{{ 42,42,42,42,42,42,42,42,
                                    42,42,42,42,42,42,42,42 }}};

        if (!(service == only_service))
            return auth_data {};
        if (!(device_id == only_dev_id))
            return auth_data {};
        if (!(auth_user == only_user) || !(service_user == only_user))
            return auth_data {};
        auth_data ret;
        ret._user_id = User_ID{1};
        ret._token = Token_t {{{32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
                                32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
                                32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
                                32, 32}}};
        ret._data = std::vector<uint8_t> ();
        return ret;
    }
};

} // namespace Impl
} // namespace Fenrir__v1
