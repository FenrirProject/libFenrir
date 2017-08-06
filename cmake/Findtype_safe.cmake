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

FIND_PATH(TYPE_SAFE_INCLUDE_DIR
    NAMES type_safe
    PATH_SUFFIXES include/ type_safe
    PATHS
    ${TYPE_SAFE_ROOT}
    $ENV{TYPE_SAFE_ROOT}
    /usr/
    /usr/local/
    ${PROJECT_SOURCE_DIR}/external/type_safe/include
)

IF(TYPE_SAFE_INCLUDE_DIR)
    SET(TYPE_SAFE_FOUND TRUE)
ELSE(TYPE_SAFE_INCLUDE_DIR)
    SET(TYPE_SAFE_FOUND FALSE)
ENDIF(TYPE_SAFE_INCLUDE_DIR)

IF(TYPE_SAFE_FOUND)
    MESSAGE(STATUS "Found type_safe in ${TYPE_SAFE_INCLUDE_DIR}")
ELSE(TYPE_SAFE_FOUND)
    IF(TYPE_SAFE_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find \"type_safe\" library. please run\n"
                            "\tgit submodule init\n"
                            "\tgit submodule update\n")
    ENDIF(TYPE_SAFE_FIND_REQUIRED)
ENDIF(TYPE_SAFE_FOUND)

MARK_AS_ADVANCED(
    TYPE_SAFE_INCLUDE_DIR
)

