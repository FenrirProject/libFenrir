#
# Copyright (c) 2015, Luca Fulchir<luca@fulchir.it>, All rights reserved.
#
# This file is part of libFenrir.
#
# libFenrir is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3
# of the License, or (at your option) any later version.
#
# libFenrir is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# and a copy of the GNU Lesser General Public License
# along with libFenrir.  If not, see <http://www.gnu.org/licenses/>.
#

FIND_PATH(DEBUG_ASSERT_INCLUDE_DIR
    NAMES debug_assert.hpp
    PATH_SUFFIXES include/ debug_assert
    PATHS
    ${DEBUG_ASSERT_ROOT}
    $ENV{DEBUG_ASSERT_ROOT}
    /usr/
    /usr/local/
    ${PROJECT_SOURCE_DIR}/external/debug_assert
)

IF(DEBUG_ASSERT_INCLUDE_DIR)
    SET(DEBUG_ASSERT_FOUND TRUE)
ELSE(DEBUG_ASSERT_INCLUDE_DIR)
    SET(DEBUG_ASSERT_FOUND FALSE)
ENDIF(DEBUG_ASSERT_INCLUDE_DIR)

IF(DEBUG_ASSERT_FOUND)
    MESSAGE(STATUS "Found debug_assert in ${DEBUG_ASSERT_INCLUDE_DIR}")
ELSE(DEBUG_ASSERT_FOUND)
    IF(DEBUG_ASSERT_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find \"debug_assert\" library. please run\n"
                            "\tgit submodule init\n"
                            "\tgit submodule update\n")
    ENDIF(DEBUG_ASSERT_FIND_REQUIRED)
ENDIF(DEBUG_ASSERT_FOUND)

MARK_AS_ADVANCED(
    DEBUG_ASSERT_INCLUDE_DIR
)

