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
#include "Fenrir/v1/data/Device_ID.hpp"
#include "Fenrir/v1/service/Service_ID.hpp"
#include "Fenrir/v1/net/Link_defs.hpp"
#include "Fenrir/v1/util/span_overlay.hpp"
#include <gsl/span>
#include <type_traits>

namespace Fenrir__v1 {
namespace Impl {
namespace Control {

// Define the controla messages and provide an interface
// to somehow easily manage them.


// Derive this class for every message.

enum class Access : uint8_t { READ_ONLY, READ_WRITE };

template <Access A = Access::READ_ONLY>
class FENRIR_LOCAL Base
{
public:
    enum class Type : uint8_t {
        LINK_ACTIVATION_SRV = 0x00,  // link activation message: server
        LINK_ACTIVATION_CLI = 0x01  // link activation message: client
    };

    // const or not depending on template
    typename std::conditional_t<A == Access::READ_ONLY,
                                                gsl::span<const uint8_t>,
                                                gsl::span<uint8_t>> _raw;
    struct data {
        Type _type;
    };
    struct data const *const r;
    // pointer to write always present, but does not allow to write if not RW
    typename std::conditional_t<A == Access::READ_ONLY,
                                                struct data const *const,
                                                struct data *const> w;

    // only enable for READ-ONLY ACCESS
    template <Access A1 = A,
        typename std::enable_if_t<A1 == Access::READ_ONLY, uint32_t> = 0>
    Base (gsl::span<const uint8_t> raw, const Base::Type t)
    :_raw (raw), r (reinterpret_cast<struct data const*>(_raw.data())),
                 w (reinterpret_cast<struct data const*>(_raw.data()))
    {
        // just check the type
        if (_raw.size() == 0 || _raw[0] != static_cast<uint8_t> (t))
            _raw = gsl::span<const uint8_t> ();
    }

    // only enable for READ-WRITE ACCESS
    template <Access A1 = A,
            typename std::enable_if_t<A1 == Access::READ_WRITE, uint32_t> = 0>
    Base (gsl::span<uint8_t> raw, const Base::Type t)
        :_raw (raw), r (reinterpret_cast<struct data*>(_raw.data())),
                     w (reinterpret_cast<struct data*>(_raw.data()))
    {
        if (_raw.size() > 0)
            w->_type = t;
    }
    Base() = delete;
    Base (const Base&) = default;
    Base& operator= (const Base&) = default;
    Base (Base &&) = default;
    Base& operator= (Base &&) = default;
    ~Base() = default;

    explicit operator bool() const
        { return _raw.size() > 0; }
    size_t size() const
        { return static_cast<size_t> (_raw.size()); }
    static constexpr uint16_t min_size()
        { return data_offset; }
    static type_safe::optional<Type> get_type (const
                                                gsl::span<const uint8_t> raw)
    {
        if (raw.size() == 0)
            return type_safe::nullopt;
        switch (raw[0]) {
        case static_cast<uint8_t> (Type::LINK_ACTIVATION_SRV):
            return Type::LINK_ACTIVATION_SRV;
        case static_cast<uint8_t> (Type::LINK_ACTIVATION_CLI):
            return Type::LINK_ACTIVATION_CLI;
        default:
            return type_safe::nullopt;;
        }
    }
protected:
    static constexpr uint16_t data_offset = sizeof(struct data);
};

//using Type = Base<Access::READ_ONLY>::Type;


///////////////////////
// Link_Acrivation_Srv
///////////////////////

template <Access A = Access::READ_ONLY>
class FENRIR_LOCAL Link_Activation_Srv final : public Base<A>
{
public:
    struct data {
        typename Base<A>::Type _type;
        Link_ID _link_id;
    };
    struct data const *const r;
    typename std::conditional_t<A == Access::READ_ONLY,
                                                struct data const *const,
                                                struct data *const> w;
    typename std::conditional_t<A == Access::READ_ONLY,
                                            Span_Overlay<const uint8_t>,
                                            Span_Overlay<uint8_t>> _activation;

    Link_Activation_Srv() = delete;
    Link_Activation_Srv (const Link_Activation_Srv&) = default;
    Link_Activation_Srv& operator= (const Link_Activation_Srv&) = default;
    Link_Activation_Srv (Link_Activation_Srv &&) = default;
    Link_Activation_Srv& operator= (Link_Activation_Srv &&) = default;
    ~Link_Activation_Srv() = default;

    // only enable for READ-ONLY ACCESS
    template <Access A1 = A,
        typename std::enable_if_t<A1 == Access::READ_ONLY, uint32_t> = 0>
    Link_Activation_Srv (const gsl::span<const uint8_t> raw);  // received pkt

    // only enable for READ-WRITE ACCESS
    template <Access A1 = A,
        typename std::enable_if_t<A1 == Access::READ_WRITE, uint32_t> = 0>
    Link_Activation_Srv (gsl::span<uint8_t> raw, const Link_ID &link);

    explicit operator bool() const;

    static constexpr uint16_t min_size();
private:
    static constexpr uint16_t activation_offset = sizeof(struct data);
    static constexpr uint16_t min_data_len = activation_offset+sizeof(uint16_t);
};


///////////////////////
// Link_Acrivation_Cli
///////////////////////

template <Access A = Access::READ_ONLY>
class FENRIR_LOCAL Link_Activation_CLi final : public Base<A>
{
public:
    struct data {
        typename Base<A>::Type _type;
        Link_ID _link_id;
    };
    struct data const *const r;
    typename std::conditional_t<A == Access::READ_ONLY,
                                                struct data const *const,
                                                struct data *const> w;
    typename std::conditional_t<A == Access::READ_ONLY,
                                            Span_Overlay<const uint8_t>,
                                            Span_Overlay<uint8_t>> _activation;

    Link_Activation_CLi() = delete;
    Link_Activation_CLi (const Link_Activation_CLi&) = default;
    Link_Activation_CLi& operator= (const Link_Activation_CLi&) = default;
    Link_Activation_CLi (Link_Activation_CLi &&) = default;
    Link_Activation_CLi& operator= (Link_Activation_CLi &&) = default;
    ~Link_Activation_CLi() = default;

    // only enable for READ-ONLY ACCESS
    template <Access A1 = A,
        typename std::enable_if_t<A1 == Access::READ_ONLY, uint32_t> = 0>
    Link_Activation_CLi (gsl::span<uint8_t> raw);  // received pkt

    // only enable for READ-WRITE ACCESS
    template <Access A1 = A,
        typename std::enable_if_t<A1 == Access::READ_WRITE, uint32_t> = 0>
    Link_Activation_CLi (gsl::span<uint8_t> raw, const Link_ID &link);

    explicit operator bool() const;

    static constexpr uint16_t min_size();
private:
    static constexpr uint16_t activation_offset = sizeof(struct data);
    static constexpr uint16_t min_data_len = activation_offset+sizeof(uint16_t);
};


} // namespace Control
} // namespace Impl
} // namespace Fenrir__v1

#ifdef FENRIR_HEADER_ONLY
#include "Fenrir/v1/data/control/Control.hpp"
#endif
