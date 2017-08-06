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

namespace Fenrir__v1 {
namespace Impl {

enum class Endianness : uint8_t {
    LITTLE, // x86 like
    BIG
};

constexpr Endianness FENRIR_LOCAL get_endianness()
{
    // Can not use unions, since you can't use the non-initialized member
    // of the union. Can't use reinterpret_cast either, so is there something
    // else? ..maybe C++17 or later?
    // let's leave this here, it's constexpr anyway..
#if defined(FENRIR_BIG_ENDIAN)
    return Endianness::BIG;
#elif defined(FENRIR_LITTLE_ENDIAN)
    return Endianness::LITTLE;
#else
    static_assert(false, "Fenrir: please define \"FENRIR_BIG_ENDIAN\" or "
                                                        "FENRIR_LITTLE_ENDIAN");
#endif
}


template<typename T>
constexpr T FENRIR_LOCAL h_to_b (const T host)
{
    static_assert (!(std::is_const<T>::value &&
                                    get_endianness() != Endianness::BIG),
                                        "Fenrir endian: type can't be const");

    if (get_endianness() == Endianness::BIG)
        return host;
    T ret {};
    uint8_t *h = const_cast<uint8_t*> (reinterpret_cast<const uint8_t*>(&host));
    uint8_t *b = const_cast<uint8_t*> (
                            reinterpret_cast<const uint8_t*>(&ret) + sizeof(T));
    while (b > reinterpret_cast<const uint8_t*>(&host)) {
        --b;
        *b = *h;
        ++h;
    }
    return ret;
}

template<typename T>
constexpr T FENRIR_LOCAL b_to_h (const T network)
{
    static_assert (!(std::is_const<T>::value &&
                                    get_endianness() != Endianness::BIG),
                                        "Fenrir endian: type can't be const");

    if (get_endianness() == Endianness::BIG)
        return network;
    T ret {0};
    uint8_t *hst = reinterpret_cast<uint8_t*> (&ret);
    const uint8_t *b = reinterpret_cast<const uint8_t*>(&network) + sizeof(T);
    while (b > reinterpret_cast<const uint8_t*>(&network)) {
        --b;
        *hst = *b;
        ++hst;
    }
    return ret;
}

template<typename T>
constexpr T FENRIR_LOCAL h_to_l (const T host)
{
    static_assert (!(std::is_const<T>::value &&
                                    get_endianness() != Endianness::LITTLE),
                                        "Fenrir endian: type can't be const");

    if (get_endianness() == Endianness::LITTLE)
        return host;
    T ret {};
    uint8_t *h = const_cast<uint8_t*> (reinterpret_cast<const uint8_t*>(&host));
    uint8_t *l = const_cast<uint8_t*> (
                            reinterpret_cast<const uint8_t*>(&ret) + sizeof(T));
    while (l > reinterpret_cast<const uint8_t*>(&host)) {
        --l;
        *l = *h;
        ++h;
    }
    return ret;
}

template<typename T>
constexpr T FENRIR_LOCAL l_to_h (const T network)
{
    static_assert (!(std::is_const<T>::value &&
                                    get_endianness() != Endianness::LITTLE),
                                        "Fenrir endian: type can't be const");

    if (get_endianness() == Endianness::LITTLE)
        return network;
    T ret {0};
    uint8_t *hst = reinterpret_cast<uint8_t*> (&ret);
    const uint8_t *l = reinterpret_cast<const uint8_t*>(&network) + sizeof(T);
    while (l > reinterpret_cast<const uint8_t*>(&network)) {
        --l;
        *hst = *l;
        ++hst;
    }
    return ret;
}


} // namespace Impl
} // namespace Fenrir__v1

