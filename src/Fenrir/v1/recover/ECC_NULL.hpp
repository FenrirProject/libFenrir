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

#include "Fenrir/v1/recover/Error_Correction.hpp"

namespace Fenrir__v1 {
namespace Impl {
namespace Event {
class Plugin_Timer;
} // namespace Event
namespace Recover {

class FENRIR_LOCAL ECC_NULL final : public ECC
{
public:
    ECC_NULL()
        : ECC (nullptr, nullptr, nullptr, nullptr) {}
    ECC_NULL (const ECC_NULL&) = default;
    ECC_NULL& operator= (const ECC_NULL&) = default;
    ECC_NULL (ECC_NULL &&) = default;
    ECC_NULL& operator= (ECC_NULL &&) = default;
    ~ECC_NULL() {}

    void parse_event (std::shared_ptr<Event::Plugin_Timer> ev) override
    {
        FENRIR_UNUSED (ev);
        return;
    }

    bool init (const gsl::span<uint8_t, 64> random) override
    {
        FENRIR_UNUSED (random);
        return true;
    }
    ECC::ID get_id() const override
        { return ECC::ID {1}; }
    uint16_t bytes_header() const override
        { return 0; }
    uint16_t bytes_footer() const override
        { return 0; }
    uint16_t bytes_overhead() const override
        { return 0; }

    Result correct (const gsl::span<uint8_t> raw,
                                gsl::span<uint8_t> &raw_no_ecc_header) override
    {
        raw_no_ecc_header = raw;
        return Result::OK;
    }
    Impl::Error add_ecc (gsl::span<uint8_t> raw) override
    {
        FENRIR_UNUSED (raw);
        return Impl::Error::NONE;
    }
};

} // namespace Recover
} // namespace Impl
} // namespace Fenrir__v1
