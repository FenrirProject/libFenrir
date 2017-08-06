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
#include <cmath>


namespace Fenrir__v1 {
namespace Impl {

    constexpr uint32_t pow (const uint32_t base, const uint32_t exponent)
        { return (exponent == 0) ? 1 : (base * pow(base, exponent - 1)); }


    template<typename T>
    constexpr T FENRIR_LOCAL div_floor (const T a, const T b);
    template<typename T>
    constexpr T FENRIR_LOCAL div_ceil (const T a, const T b);

    template<>
    float FENRIR_LOCAL div_floor (const float a, const float b)
        { return std::floor (a / b); }
    template<>
    float FENRIR_LOCAL div_ceil (const float a, const float b)
        { return std::ceil (a / b); }
    template<>
    double FENRIR_LOCAL div_floor (const double a, const double b)
        { return std::floor (a / b); }
    template<>
    double FENRIR_LOCAL div_ceil (const double a, const double b)
        { return std::ceil (a / b); }


    // integers
    template<typename T>
    constexpr T FENRIR_LOCAL div_floor (const T a, const T b)
        { return a / b; }
    template<typename T>
    constexpr T FENRIR_LOCAL div_ceil (const T a, const T b)
        { return (a % b == 0 ? (a / b) : (a / b) + 1); }

} // namespace Impl
} // namespace Fenrir__v1

