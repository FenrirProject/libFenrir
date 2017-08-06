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

#include "Fenrir/v1/data/control/Control.hpp"

namespace Fenrir__v1 {
namespace Impl {
namespace Control {

///////////////////////
// Link_Acrivation_Srv
///////////////////////

template <Access A>
template <Access A1,
                typename std::enable_if_t<A1 == Access::READ_ONLY, uint32_t>>
FENRIR_INLINE Link_Activation_Srv<A>::Link_Activation_Srv (
                                            const gsl::span<const uint8_t> raw)
    : Base<A> (raw, Base<A>::Type::LINK_ACTIVATION_SRV),
                r (reinterpret_cast<struct data const*>(Base<A>::_raw.data())),
                w (reinterpret_cast<struct data const*>(Base<A>::_raw.data()))
{
    if (Base<A>::_raw.size() >= min_data_len) {
        _activation = Span_Overlay<const uint8_t>::mk_overlay(
                                    Base<A>::_raw.subspan (activation_offset));
    }
}

template <Access A>
template <Access A1,
                typename std::enable_if_t<A1 == Access::READ_WRITE, uint32_t>>
FENRIR_INLINE Link_Activation_Srv<A>::Link_Activation_Srv (
                                                        gsl::span<uint8_t> raw,
                                                            const Link_ID &link)
    : Base<A> (raw, Base<A>::Type::LINK_ACTIVATION_SRV),
                    r (reinterpret_cast<struct data*>(Base<A>::_raw.data())),
                    w (reinterpret_cast<struct data*>(Base<A>::_raw.data()))
{
    if (Base<A>::_raw.size() >= min_data_len) {
        w->_link_id = link;
        _activation = Span_Overlay<uint8_t>::mk_overlay(
                                    Base<A>::_raw.subspan (activation_offset));
    }
}

template <Access A>
FENRIR_INLINE Link_Activation_Srv<A>::operator bool() const
{
    return Base<A>::_raw.size() >= min_data_len && r->_link_id && _activation;
}

template <Access A>
FENRIR_INLINE constexpr uint16_t Link_Activation_Srv<A>::min_size()
    { return min_data_len; }


///////////////////////
// Link_Acrivation_Cli
///////////////////////

template <Access A>
template <Access A1,
                typename std::enable_if_t<A1 == Access::READ_ONLY, uint32_t>>
FENRIR_INLINE Link_Activation_CLi<A>::Link_Activation_CLi (
                                                const gsl::span<uint8_t> raw)
    : Base<A> (raw, Base<A>::Type::LINK_ACTIVATION_CLI),
                r (reinterpret_cast<struct data const*>(Base<A>::_raw.data())),
                w (reinterpret_cast<struct data const*>(Base<A>::_raw.data()))
{
    if (Base<A>::_raw.size() >= min_data_len) {
        _activation = Span_Overlay<const uint8_t>::mk_overlay(
                                    Base<A>::_raw.subspan (activation_offset));
    }
}

template <Access A>
template <Access A1,
                typename std::enable_if_t<A1 == Access::READ_WRITE, uint32_t>>
FENRIR_INLINE Link_Activation_CLi<A>::Link_Activation_CLi (
                                                const gsl::span<uint8_t> raw,
                                                const Link_ID &link)
    : Base<A> (raw, Base<A>::Type::LINK_ACTIVATION_CLI),
                    r (reinterpret_cast<struct data*>(Base<A>::_raw.data())),
                    w (reinterpret_cast<struct data*>(Base<A>::_raw.data()))
{
    if (Base<A>::_raw.size() >= min_data_len) {
        w->_link_id = link;
        _activation = Span_Overlay<uint8_t>::mk_overlay(
                                    Base<A>::_raw.subspan (activation_offset));
    }
}

template <Access A>
FENRIR_INLINE Link_Activation_CLi<A>::operator bool() const
{
    return Base<A>::_raw.size() >= min_data_len && r->_link_id && _activation;
}

template <Access A>
FENRIR_INLINE constexpr uint16_t Link_Activation_CLi<A>::min_size()
    { return min_data_len; }



} // namespace Control
} // namespace Impl
} // namespace Fenrir__v1
