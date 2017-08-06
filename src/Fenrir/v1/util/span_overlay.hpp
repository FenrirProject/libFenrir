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
#include "Fenrir/v1/util/endian.hpp"
#include <gsl/span>
#include <type_safe/optional.hpp>
#include <type_traits>
#include <vector>

namespace Fenrir__v1 {
namespace Impl {

// overlay on span:
// for added typesafety we use this on top of span<uint8_t>
// this format includes a uin16_t for the size, and the actual span of T
// we also handle failed parsing of data through _size == nullptr
template<typename T>
class Span_Overlay final : public gsl::span<T>
{
public:
    Span_Overlay () : _size (nullptr) {}

    Span_Overlay (const Span_Overlay&) = default;
    Span_Overlay& operator= (const Span_Overlay&) = default;
    Span_Overlay (Span_Overlay&&) = default;
    Span_Overlay& operator= (Span_Overlay&&) = default;
    ~Span_Overlay() = default;

    // CONST
    // load from raw bytes to overlay
    template <typename T1 = T,
            typename = std::enable_if_t<std::is_const<T1>::value>>
    static Span_Overlay<T> mk_overlay (const gsl::span<const uint8_t> span)
    {
        constexpr ssize_t sz_16 = static_cast<ssize_t>(sizeof(uint16_t));
        if (span.length() < sz_16)
            return Span_Overlay<T>();
        const uint16_t test_size = l_to_h<uint16_t> (
                            *reinterpret_cast<const uint16_t*> (span.data()));
        if (test_size <= ((static_cast<size_t>(span.size()) - sizeof(uint16_t))
                                                                / sizeof(T))) {
            return Span_Overlay<T> (span.subspan (0, sizeof(uint16_t) +
                                            test_size * sizeof(T)), test_size);
        }
        return Span_Overlay<T>();
    }


    // NON CONST
    // load from raw bytes to overlay
    template <typename T1 = T,
                        typename = std::enable_if_t<!std::is_const<T1>::value>>
    static Span_Overlay<T> mk_overlay (const gsl::span<uint8_t> span)
    {
        constexpr ssize_t sz_16 = static_cast<ssize_t>(sizeof(uint16_t));
        if (span.length() < sz_16)
            return Span_Overlay<T>();
        const uint16_t test_size = l_to_h<uint16_t> (
                            *reinterpret_cast<const uint16_t*> (span.data()));
        if (test_size <= ((static_cast<size_t>(span.size()) - sizeof(uint16_t))
                                                                / sizeof(T))) {
            return Span_Overlay<T> (span.subspan (0, sizeof(uint16_t) +
                                            test_size * sizeof(T)), test_size);
        }
        return Span_Overlay<T>();
    }


    // CONST
    // load from vector to raw bytes
    template <typename T1 = T,
                        typename = std::enable_if_t<std::is_const<T1>::value>>
    static Span_Overlay<T> mk_overlay (const gsl::span<const uint8_t> sp,
                                                    const std::vector<T> &in)
    {
        // test the vector length
        // now load the vector
        const uint16_t in_length = static_cast<uint16_t> (in.size());
        const ssize_t in_bytes = (sizeof(uint16_t) + in_length * sizeof(T));
        if (sp.size_bytes() < in_bytes) {
            return Span_Overlay<T>();
        }
        Span_Overlay<T> ret {sp.subspan(0, in_bytes), in_length};
        std::copy (in.cbegin(), in.cend(), ret.begin());
        return ret;
    }

    // NON CONST
    // load from vector to raw bytes
    template <typename T1 = T,
                        typename = std::enable_if_t<!std::is_const<T1>::value>>
    static Span_Overlay<T> mk_overlay (const gsl::span<uint8_t> sp,
                                                    const std::vector<T> &in)
    {
        // test the vector length
        // now load the vector
        const uint16_t in_length = static_cast<uint16_t> (in.size());
        const ssize_t in_bytes = (sizeof(uint16_t) + in_length * sizeof(T));
        if (sp.size_bytes() < in_bytes) {
            return Span_Overlay<T>();
        }
        Span_Overlay<T> ret {sp.subspan(0, in_bytes), in_length};
        std::copy (in.cbegin(), in.cend(), ret.begin());
        return ret;
    }


    // CONST
    // make sure we have the right size
    template <typename T1 = T,
                        typename = std::enable_if_t<std::is_const<T1>::value>>
    static Span_Overlay<T> mk_overlay (const gsl::span<const uint8_t> span,
                                                        const uint16_t length)
    {
        if ((span.length() - sizeof(uint16_t)) / sizeof(T) < length)
            return Span_Overlay<T>();
        return Span_Overlay<T> (span.subspan (0, sizeof(uint16_t) +
                                                sizeof(T) * length), length);
    }

    // NON-CONST
    // make sure we have the right size
    template <typename T1 = T,
                typename = std::enable_if_t<!std::is_const<T1>::value>>
    static Span_Overlay<T> mk_overlay (const gsl::span<uint8_t> span,
                                                        const uint16_t length)
    {
        if ((span.length() - sizeof(uint16_t)) / sizeof(T) < length)
            return Span_Overlay<T>();
        return Span_Overlay<T> (span.subspan (0, sizeof(uint16_t) +
                                                sizeof(T) * length), length);
    }

    explicit operator bool() const
        { return _size != nullptr; }
    size_t raw_size() const
        { return sizeof(uint16_t) +
                            static_cast<size_t> (gsl::span<T>::size_bytes()); }
private:
    // TODO: simplify. The private constructor should already get
    //   verified parameters. move to raw poitner to make this a bit quicker

    // CONST
    // with provided size
    template <typename T1 = T,
            typename = std::enable_if_t<std::is_const<T1>::value>>
    Span_Overlay (const gsl::span<const uint8_t> sp, const uint16_t length)
        : gsl::span<T> (reinterpret_cast<T*> (sp.data() + sizeof(uint16_t)),
                        (sp.size() - static_cast<ssize_t> (sizeof(uint16_t))) /
                                            static_cast<ssize_t> (sizeof(T))),
        _size (reinterpret_cast<uint16_t *> (const_cast<uint8_t*> (sp.data())))
    {
        assert (length == (static_cast<size_t>(sp.length()) - sizeof(uint16_t))
                                                                / sizeof(T) &&
                                "Fenrir:Span_Overlay: wrong initialization.");
        assert (length == gsl::span<T>::size() &&
                                "Fenrir:Span_Overlay: err initializing.");
        assert (length == l_to_h<uint16_t> (*_size) &&
                                        "Fenrir:Span_Overlay: wrong length");
    }

    // NON-CONST
    // with provided size
    template <typename T1 = T,
            typename = std::enable_if_t<!std::is_const<T1>::value>>
    Span_Overlay (const gsl::span<uint8_t> sp, const uint16_t length)
        : gsl::span<T> (reinterpret_cast<T*> (sp.data() + sizeof(uint16_t)),
                        (sp.size() - static_cast<ssize_t> (sizeof(uint16_t))) /
                                            static_cast<ssize_t> (sizeof(T))),
          _size (reinterpret_cast<uint16_t*> (sp.data()))
    {
        assert (length == (static_cast<size_t>(sp.length()) - sizeof(uint16_t))
                                                                / sizeof(T) &&
                                "Fenrir:Span_Overlay: wrong initialization.");
        assert (length == gsl::span<T>::size() &&
                                "Fenrir:Span_Overlay: err initializing.");
        //_size = reinterpret_cast<uint16_t*> (sp.data());
        *_size = h_to_l<uint16_t> (length);
    }

    uint16_t *_size;
};

} // namespace Impl
} // namespace Fenrir__v1
