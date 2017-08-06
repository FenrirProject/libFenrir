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

#include <memory>
#include <vector>
#include "Fenrir/v1/auth/Auth.hpp"
#include "Fenrir/v1/db/Db.hpp"
#include "Fenrir/v1/data/Username.hpp"
#include "Fenrir/v1/plugin/Lib.hpp"
#include "Fenrir/v1/common.hpp"

namespace Fenrir__v1 {
namespace Impl {
namespace Crypto {

class FENRIR_LOCAL Token final : public Auth
{
public:
    Token ()
        : Auth (std::shared_ptr<Lib> (nullptr), nullptr, nullptr, nullptr)
        {}
    Token (const Token&) = default;
    Token& operator= (const Token&) = default;
    Token (Token &&) = default;
    Token& operator= (Token &&) = default;
    ~Token() = default;
    void parse_event (std::shared_ptr<Event::Plugin_Timer> ev) override
        { FENRIR_UNUSED (ev); }

    Auth_Result authenticate (const Device_ID &dev_id,
                              const Service_ID &service,
                              const Username &auth_user,
                              const Username &service_user,
                              const gsl::span<uint8_t> data, Db *db) override;
    Auth::ID id() const override
        { return Auth::ID {1}; }
};

FENRIR_INLINE Auth_Result Token::authenticate (const Device_ID &dev_id,
                                        const Service_ID &service,
                                        const Username &auth_user,
                                        const Username &service_user,
                                        const gsl::span<uint8_t> data, Db *db)
{
    auto db_result = db->get_auth (dev_id, service, auth_user, service_user,
                                                                        id());

    Auth_Result ret;
    if (auth_user.is_anonymous() && service_user.is_anonymous()) {
        ret._failed = false;
        return ret;
    }

    // TODO: XORed, OTP version.

    if (data.size() != sizeof(Token_t) || db_result._user_id == User_ID{0})
        return ret;
    const Token_t *t_ptr = reinterpret_cast<const Token_t*> (data.data());
    if (db_result._token == *t_ptr) {
        ret._user_id = db_result._user_id;
        ret._anonymous = false;
        ret._failed = false;
    }
    return ret;
}

} // namespace Auth
} // namespace Impl
} // namespace Fenrir__v1
