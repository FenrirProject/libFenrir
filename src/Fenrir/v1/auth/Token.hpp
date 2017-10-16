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

#include "Fenrir/v1/auth/Auth.hpp"
#include "Fenrir/v1/auth/Lattice.hpp"
#include "Fenrir/v1/common.hpp"
#include "Fenrir/v1/data/Username.hpp"
#include "Fenrir/v1/db/Db.hpp"
#include "Fenrir/v1/plugin/Lib.hpp"
#include <memory>
#include <vector>

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
    ~Token() override = default;
    void parse_event (std::shared_ptr<Event::Plugin_Timer> ev) override
        { FENRIR_UNUSED (ev); }

    Auth_Result authenticate (const Device_ID &dev_id,
                              const Service_ID &service,
                              const std::shared_ptr<Lattice> lattice,
                              const Lattice_Node::ID lattice_node,
                              const Username &auth_user,
                              const Username &service_user,
                              const gsl::span<uint8_t> data, Db *db) override;
    Auth::ID id() const override
        { return Auth::ID {1}; }
};

} // namespace Auth
} // namespace Impl
} // namespace Fenrir__v1
