#
# Copyright (c) 2017, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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

FIND_PATH(FENRIR_SODIUM_INCLUDE_DIR
    NAMES sodium.h
    PATH_SUFFIXES include/ include/sodium
    PATHS
    ${SODIUM_ROOT}
    $ENV{SODIUM_ROOT}
    /usr/
    /usr/local/
    ${CMAKE_CURRENT_SOURCE_DIR}/external/libsodium/src/libsodium
)

FIND_LIBRARY(FENRIR_SODIUM_LIB
    NAMES sodium
    PATH_SUFFIXES lib/
    PATHS
    ${SODIUM_ROOT}
    $ENV{SODIUM_ROOT}
    /usr/
    /usr/local/
)

IF(FENRIR_SODIUM_INCLUDE_DIR)
    SET(FENRIR_SODIUM_FOUND TRUE)
ELSE(FENRIR_SODIUM_INCLUDE_DIR)
    SET(FENRIR_SODIUM_FOUND FALSE)
ENDIF(FENRIR_SODIUM_INCLUDE_DIR)

IF(FENRIR_SODIUM_FOUND)
    IF(BUILD_SODIUM)
        # force our own lz4 libary
        SET(FENRIR_SODIUM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/libsodium/src/libsodium/include/)
    ENDIF()
    MESSAGE(STATUS "Found libsodium in ${FENRIR_SODIUM_INCLUDE_DIR}")
    # we need both headers and library, or we use our own
    IF(FENRIR_SODIUM_INCLUDE_DIR MATCHES "${CMAKE_CURRENT_SOURCE_DIR}/external/libsodium/src/libsodium/include" OR FENRIR_SODIUM_LIB MATCHES "FENRIR_SODIUM_LIB-NOTFOUND")
        MESSAGE(WARNING "We will build our own libsodium(stable) library and statically link it.")
        SET(BUILD_SODIUM TRUE)
    ENDIF()
ELSE(FENRIR_SODIUM_FOUND)
    IF(FENRIR_SODIUM_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find \"libsodium\" library."
                            "  Please install the \"libsodium\" library"
                            " or at least run:\n"
                            "  cd ${CMAKE_CURRENT_SOURCE_DIR}\n"
                            "  git submodule init\n"
                            "  git submodule update\n"
                            " so that we can build our own\n")
    ENDIF(FENRIR_SODIUM_FIND_REQUIRED)
ENDIF(FENRIR_SODIUM_FOUND)

MARK_AS_ADVANCED(
    FENRIR_SODIUM_INCLUDE_DIR
)
