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

# NOTE: we need our own unbound 'cause the one in the distributions
#   uses libevent. we use libev, and that clash breaks things


FIND_PATH(FENRIR_UNBOUND_INCLUDE_DIR
    NAMES unbound.h
    PATH_SUFFIXES include/
    PATHS
    ${UNBOUND_ROOT}
    $ENV{UNBOUND_ROOT}
    /usr/
    /usr/local/
    ${CMAKE_CURRENT_SOURCE_DIR}/external/unbound/libunbound/
)

FIND_LIBRARY(FENRIR_UNBOUND_LIB
    NAMES unbound
    PATH_SUFFIXES lib/
    PATHS
    ${UNBOUND_ROOT}
    $ENV{UNBOUND_ROOT}
    /usr/
    /usr/local/
)

IF(FENRIR_UNBOUND_INCLUDE_DIR)
    SET(FENRIR_UNBOUND_FOUND TRUE)
ELSE(FENRIR_UNBOUND_INCLUDE_DIR)
    SET(FENRIR_UNBOUND_FOUND FALSE)
ENDIF(FENRIR_UNBOUND_INCLUDE_DIR)

IF(FENRIR_UNBOUND_FOUND)
    IF(BUILD_UNBOUND)
        # force our own unbound libary
        SET(FENRIR_UNBOUND_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/unbound/libunbound/)
    ELSE()
        execute_process(COMMAND "ldd ${FENRIR_UNBOUND_LIB}| grep event" OUTPUT_VARIABLE HAS_LIBEVENT)
        string(REGEX MATCH ".*libevent.*" CMAKE_HAS_LIBEVENT "${HAS_LIBEVENT}")
        IF (NOT ${CMAKE_HAS_LIBEVENT} MATCHES "")
            # unbound was build with libevent. we use libev. clash and crash.
            SET(UNBOUND_WAS_FOUND "true")
            SET(FENRIR_UNBOUND_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/unbound/libunbound/)
        ENDIF()
    ENDIF()
    MESSAGE(STATUS "Found libsodium in ${FENRIR_UNBOUND_INCLUDE_DIR}")
    # we need both headers and library, or we use our own
    IF(FENRIR_UNBOUND_INCLUDE_DIR MATCHES "${CMAKE_CURRENT_SOURCE_DIR}/external/unbound/libunbound" OR FENRIR_UNBOUND_LIB MATCHES "FENRIR_UNBOUND_LIB-NOTFOUND")
        IF (${UNBOUND_WAS_FOUND} MATCHES "true")
            MESSAGE(WARNING "We found libunbound, but it was build with libevent, therefore we need to rebuild unbound :(")
        ELSE()
            MESSAGE(WARNING "We will build our own libunbound(stable) library and statically link it.")
        ENDIF()
        SET(BUILD_UNBOUND TRUE)
    ENDIF()
ELSE(FENRIR_UNBOUND_FOUND)
    IF(FENRIR_UNBOUND_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find \"libunbound\" library."
                            "  Please install the \"libunbound\" library"
                            " or at least run:\n"
                            "  cd ${CMAKE_CURRENT_SOURCE_DIR}\n"
                            "  git submodule init\n"
                            "  git submodule update\n"
                            " so that we can build our own\n")
    ENDIF(FENRIR_UNBOUND_FIND_REQUIRED)
ENDIF(FENRIR_UNBOUND_FOUND)

MARK_AS_ADVANCED(
    FENRIR_UNBOUND_INCLUDE_DIR
)

