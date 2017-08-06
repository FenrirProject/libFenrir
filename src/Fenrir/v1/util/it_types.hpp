/*
 * Copyright (c) 2017, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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

// Now just some macros to check the iterator type
#ifdef __cplusplus

#include <cassert>
#include <iterator>
#include <type_traits>

#define IS_RANDOM(IT, CLASS) \
        static_assert ( \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::random_access_iterator_tag>::value, \
            CLASS " is supposed to get a RANDOM ACCESS iterator\n");
#define IS_INPUT(IT, CLASS) \
        static_assert ( \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::input_iterator_tag>::value || \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::forward_iterator_tag>::value || \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::bidirectional_iterator_tag>::value || \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::random_access_iterator_tag>::value, \
            CLASS " is supposed to get an INPUT iterator\n");
#define IS_FORWARD(IT, CLASS) \
        static_assert ( \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::forward_iterator_tag>::value || \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::bidirectional_iterator_tag>::value || \
            std::is_same<typename std::iterator_traits<IT>::iterator_category, \
                                    std::random_access_iterator_tag>::value, \
            CLASS " is supposed to get a FORWARD iterator\n");
#endif

