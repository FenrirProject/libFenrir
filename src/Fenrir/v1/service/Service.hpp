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
#include "Fenrir/v1/data/packet/Stream.hpp"
#include "Fenrir/v1/data/Storage_t.hpp"
#include "Fenrir/v1/data/Username.hpp"
#include "Fenrir/v1/service/Service_ID.hpp"
#include "Fenrir/v1/util/span_overlay.hpp"
#include <array>
#include <type_safe/strong_typedef.hpp>

namespace Fenrir__v1 {
namespace Impl {

enum class Service_msg : uint8_t {
        NEW_USER      = 0x01,
        NEW_USER_ANSW = 0x02
        };

class FENRIR_LOCAL Service_INIT
{
public:
    using ID = Service_ID;
    struct data {
        Service_msg _type;
        User_ID _user_id;
    };
    struct stream_info {
        Stream_ID _id;
        Counter _counter_start;
        Storage_t _type;
        Stream_PRIO _priority;
    };
    struct data const *const r;
    struct data *const w;
    Span_Overlay<uint8_t> _service_username;
    Span_Overlay<stream_info> _streams;

    Service_INIT (const gsl::span<uint8_t> raw);
    Service_INIT (      gsl::span<uint8_t> raw, const Username username,
                                                        const User_ID user_id,
                                                        const uint16_t streams);

    static constexpr uint16_t min_size();
private:
    static constexpr uint16_t data_offset = sizeof(struct data);
    static constexpr uint16_t min_data_len = data_offset + 1 * sizeof(uint16_t);
};


} // namespace Impl
} // namespace Fenrir__v1
