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
#include "Fenrir/v1/data/packet/Stream.hpp"
#include "Fenrir/v1/plugin/Dynamic.hpp"
#include <gsl/span>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {
namespace Recover {

// Error Correcting Codes:
// per-packet correction.
// This should be used to recover from
// transmission errors
class FENRIR_LOCAL ECC : public Dynamic
{
public:
    struct FENRIR_LOCAL ID :
                    type_safe::strong_typedef<ID, uint16_t>,
                    type_safe::strong_typedef_op::equality_comparison<ID>,
                    type_safe::strong_typedef_op::relational_comparison<ID>
        { using strong_typedef::strong_typedef; };
    enum class FENRIR_LOCAL Result : uint8_t {
        ERR = 0x00,
        OK = 0x01,
        CORRECTED = 0x02
    };

    ECC (const std::shared_ptr<Lib> from, Event::Loop *const loop,
                                        Loader *const loader, Random *const rnd)
        : Dynamic (from, loop, loader, rnd) {}
    ECC() = delete;
    ECC (const ECC&) = default;
    ECC& operator= (const ECC&) = default;
    ECC (ECC &&) = default;
    ECC& operator= (ECC &&) = default;
    virtual ~ECC () {}

    virtual bool init (const std::vector<uint8_t> &pref_raw) = 0;
    virtual std::vector<uint8_t> get_preferences() = 0;
    virtual ECC::ID get_id() const = 0;
    virtual uint16_t get_overhead() const = 0;

    virtual Result correct (const gsl::span<uint8_t> raw,
                                    gsl::span<uint8_t> &raw_no_ecc_header) = 0;
    virtual Impl::Error add_ecc (gsl::span<uint8_t> raw) = 0;
};

} // namespace Recover
} // namespace Impl
} // namespace Fenrir__v1
