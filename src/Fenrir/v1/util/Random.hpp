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
#include <chrono>
#include <gsl/span>
#include <limits>
#include <mutex>
#include <sodium.h>
#include <thread>

namespace Fenrir__v1 {
namespace Impl {

class FENRIR_LOCAL Random
{
public:
    Random()
    {
        while (sodium_init() < 0)
            std::this_thread::sleep_for (std::chrono::milliseconds {1});
    }
    Random (const Random&) = default;
    Random& operator= (const Random&) = default;
    Random (Random &&) = default;
    Random& operator= (Random &&) = default;
    ~Random() = default;

    template<typename T>
    T uniform()
    {
        T ret;
        randombytes_buf (&ret, sizeof(T));
        return ret;
    }

    template<typename T>
    T uniform (const T from, const T to)
    {
        static_assert (sizeof(T) <= sizeof(uint64_t),
                            "Fenrir Random: can't be used with types > 64bit");
        const T window = to - from;
        if (window == 0)
            return from;
        T ret;
        randombytes_buf (&ret, sizeof(T));
        ret %= window;
        return ret;
    }

    template<typename T>
    void uniform (gsl::span<T> out)
    {
        randombytes_buf (out.data(), sizeof(T) *
                                            static_cast<size_t> (out.size()));
    }

    template<typename T>
    void uniform (gsl::span<T> out, const T from, const T to)
    {
        static_assert (sizeof(T) <= sizeof(uint64_t),
                            "Fenrir Random: can't be used with types > 64bit");
        const T window = to - from;
        if (window == 0) {
            for (size_t idx = 0; idx < out.size(); ++idx)
                out[idx] = from;
            return;
        }
        randombytes_buf (out.data(), sizeof(T) * out.size());
        for (size_t idx = 0; idx < out.size(); ++idx)
            out[idx] %= window;
    }
};

} // namespace Impl
} // namespace Fenrir__v1

