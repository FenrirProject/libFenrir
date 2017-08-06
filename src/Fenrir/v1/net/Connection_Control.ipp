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
#include "Fenrir/v1/data/control/Control.ipp"
#include "Fenrir/v1/net/Connection.hpp"

namespace Fenrir__v1 {
namespace Impl {


FENRIR_INLINE void Connection::parse_rel_control()
{
    auto rel_it = std::lower_bound (_streams_in.begin(), _streams_in.end(),
                                    _rel_read_control_stream,
                                    [] (const auto &it, Stream_ID _rel)
                                { return std::get<Stream_ID> (it) < _rel; });
    assert (rel_it != _streams_in.end() &&
                                        "Fenrir: no reliable control stream!");

    std::tuple<Counter, Stream::Fragment, std::vector<uint8_t>> data;
    do {
        data = std::get<Stream_Track_In> (*rel_it)._received->get_user_data (
                                                    _rel_read_control_stream);
        if (std::get<std::vector<uint8_t>> (data).size() > 0)
            parse_control (std::move (std::get<std::vector<uint8_t>> (data)));
    } while (std::get<std::vector<uint8_t>> (data).size() > 0);
}

FENRIR_INLINE void Connection::parse_unrel_control()
{
    auto unrel_it = std::lower_bound (_streams_in.begin(), _streams_in.end(),
                                    _unrel_read_control_stream,
                                    [] (const auto &it, Stream_ID _rel)
                                { return std::get<Stream_ID> (it) < _rel; });
    assert (unrel_it != _streams_in.end() &&
                                    "Fenrir: no unreliable control stream!");

    std::tuple<Counter, Stream::Fragment, std::vector<uint8_t>> data;
    do {
        data = std::get<Stream_Track_In> (*unrel_it)._received->get_user_data (
                                                    _unrel_read_control_stream);
        if (std::get<std::vector<uint8_t>> (data).size() > 0)
            parse_control (std::move (std::get<std::vector<uint8_t>> (data)));
    } while (std::get<std::vector<uint8_t>> (data).size() > 0);

}
FENRIR_INLINE void Connection::parse_control (const std::vector<uint8_t> &data)
{
    assert (data.size() > 0 && "Fenrir:Wrog call parameter - parse_control");

    auto type = Control::Base<>::get_type (gsl::span<const uint8_t> (
                            data.data(), static_cast<ssize_t> (data.size())));
    if (!type.has_value())
        return;
    const gsl::span<uint8_t> data_span {const_cast<uint8_t*> (data.data()),
                            const_cast<uint8_t*> (data.data() + data.size())};
    switch (type.value())
    {
    case Control::Base<>::Type::LINK_ACTIVATION_CLI:
        parse_control (Control::Link_Activation_CLi<Control::Access::READ_ONLY>
                                                                {data_span});
        return; // NOTE the Control:: classes use references to out "data"
    case Control::Base<>::Type::LINK_ACTIVATION_SRV:
        parse_control (Control::Link_Activation_Srv<Control::Access::READ_ONLY>
                                                                {data_span});
        return; // NOTE the Control:: classes use references to out "data"
    }

    assert (false && "Fenrir: nonexaustive switch: parse_control");
}

void Connection::parse_control (const Control::Link_Activation_CLi<
                                            Control::Access::READ_ONLY> &&data)
{
    std::unique_lock<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);
    auto lnk_it = std::lower_bound (_incoming.begin(), _incoming.end(),
                                                data.r->_link_id,
                                                [] (const auto &it, Link_ID lnk)
                                                    { return it._link < lnk; });
    if (lnk_it == _incoming.end() || lnk_it->_link != data.r->_link_id)
        return;
    if (lnk_it->_activation.size() == 0)
        return;
    if (memcmp (lnk_it->_activation.data(), data._activation.data(),
                                            lnk_it->_activation.size()) != 0) {
        return;
    }
    lnk_it->_activation = std::vector<uint8_t>();
    lnk_it->keepalive (_loop);
    add_Link_in (lnk_it->_link);
}

void Connection::parse_control (const Control::Link_Activation_Srv<
                                            Control::Access::READ_ONLY> &&data)
{
    std::unique_lock<std::mutex> lock (_mtx);
    FENRIR_UNUSED (lock);
    auto rel_it = std::lower_bound (_streams_in.begin(), _streams_in.end(),
                                    _rel_read_control_stream,
                                    [] (const auto &it, Stream_ID _rel)
                                { return std::get<Stream_ID> (it) < _rel; });
    assert (rel_it != _streams_in.end() &&
                                        "Fenrir: no reliable control stream!");

    std::vector<uint8_t> copy (static_cast<size_t> (data._raw.size()), 0);
    const Control::Link_Activation_CLi<Control::Access::READ_WRITE> answer (
                                                        copy, data.r->_link_id);
    std::copy (data._activation.begin(), data._activation.end(),
                                                    answer._activation.begin());

    std::get<Stream_Track_In> (*rel_it)._received->add_data (
                                        _rel_read_control_stream, answer._raw);
}




} // namespace Impl
} // namespace Fenrir__v1
