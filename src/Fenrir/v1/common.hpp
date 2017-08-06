/*
 * Copyright (c) 2016-2017, Luca Fulchir<luca@fulchir.it>, All rights reserved.
 *
 * This file is part of "libFenrir".
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


// These macros were taken from http://gcc.gnu.org/wiki/Visibility
// Generic helper definitions for shared library support

#if defined _WIN32 || defined __CYGWIN__
    #define FENRIR_HELPER_DLL_IMPORT __declspec (dllimport)
    #define FENRIR_HELPER_DLL_EXPORT __declspec (dllexport)
    #define FENRIR_HELPER_DLL_LOCAL
#else
    #if __GNUC__ >= 4
        #define FENRIR_HELPER_DLL_IMPORT __attribute__ ((visibility("default")))
        #define FENRIR_HELPER_DLL_EXPORT __attribute__ ((visibility("default")))
        #define FENRIR_HELPER_DLL_LOCAL  __attribute__ ((visibility("hidden")))
    #else
        #define FENRIR_HELPER_DLL_IMPORT
        #define FENRIR_HELPER_DLL_EXPORT
        #define FENRIR_HELPER_DLL_LOCAL
    #endif
#endif

// Now we use the generic helper definitions above to define FENRIR_API
// and FENRIR_LOCAL.
// FENRIR_API is used for the public API symbols.
// It either DLL imports or DLL exports (or does nothing for static build)
// FENRIR_LOCAL is used for non-api symbols.

#ifdef FENRIR_DLL // defined if FENRIR is compiled as a DLL
    #ifdef FENRIR_DLL_EXPORTS // if we are building the FENRIR DLL(not using it)
        #define FENRIR_API FENRIR_HELPER_DLL_EXPORT
    #else
        #define FENRIR_API FENRIR_HELPER_DLL_IMPORT
    #endif // FENRIR_DLL_EXPORTS
    #define FENRIR_LOCAL FENRIR_HELPER_DLL_LOCAL
#else // FENRIR_DLL is not defined: this means FENRIR is a static lib.
    #define FENRIR_API
    #define FENRIR_LOCAL
#endif // FENRIR_DLL

#define FENRIR_DEPRECATED __attribute__ ((deprecated))

#define GSL_TERMINATE_ON_CONTRACT_VIOLATION

// silence some library warnings
#ifndef __cpp_exceptions
#define __cpp_exceptions 0
#endif

#ifndef __cplusplus
#include <stdint.h>
#include <stddef.h>
#else
// C++ version. keep the enum synced
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <type_traits>
#ifdef FENRIR_HEADER_ONLY
    #define FENRIR_INLINE inline
#else
    #define FENRIR_INLINE
#endif

// ssize_t is *NIX specific. bring it back to Windows and anything else.
using ssize_t = typename std::conditional<
                                        sizeof(std::size_t) == sizeof(int32_t),
                                                        int32_t, int64_t>::type;

namespace Fenrir__v1 {
namespace Impl {
    enum class FENRIR_LOCAL Error : uint8_t {
                NONE            = 0,
                WRONG_INPUT     = 1,
                WORKING         = 2,
                INITIALIZATION  = 3,
                EXITING         = 4,
                TIMEOUT         = 5,
                CAN_NOT_CONNECT = 6,
                UNSUPPORTED     = 100,
                RESOLVE_NOT_FOUND  = 101,
                RESOLVE_NOT_FENRIR = 102,
                NO_SUCH_FILE    = 103,
                ALREADY_PRESENT = 104,
                FULL            = 105,
                EMPTY           = 106
                };
}

// expose only a subset of the internal errors.
enum class FENRIR_API Error : uint8_t {
            NONE            = static_cast<uint8_t>(Impl::Error::NONE),
            WRONG_INPUT     = static_cast<uint8_t>(Impl::Error::WRONG_INPUT),
            INITIALIZATION  = static_cast<uint8_t>(Impl::Error::INITIALIZATION),
            NO_SUCH_DOMAIN  = static_cast<uint8_t>
                                            (Impl::Error::RESOLVE_NOT_FOUND),
            DOMAIN_NOT_FENRIR = static_cast<uint8_t>
                                            (Impl::Error::RESOLVE_NOT_FENRIR),
            NO_CONNECTION   = static_cast<uint8_t>
                                                (Impl::Error::CAN_NOT_CONNECT),
            };
} // namespace Fenrir__v1
#endif

#define FENRIR_UNUSED(x)    ((void)x)
