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

FIND_LIBRARY(UBSAN_LIB
    NAMES ubsan
    PATH_SUFFIXES lib/
    PATHS
    ${UBSAN_ROOT}
    $ENV{UBSAN_ROOT}
    /usr/
    /usr/local/
)

IF(UBSAN_LIB)
    SET(UBSAN_FOUND TRUE)
ELSE(UBSAN_LIB)
    SET(UBSAN_FOUND FALSE)
ENDIF(UBSAN_LIB)

IF(UBSAN_FOUND)
    MESSAGE(STATUS "Found UBSAN in ${UBSAN_LIB}")
ELSE(UBSAN_FOUND)
    IF(UBSAN_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find \"UBSAN\" library")
    ENDIF(UBSAN_FIND_REQUIRED)
ENDIF(UBSAN_FOUND)

MARK_AS_ADVANCED(
    UBSAN_LIB
)

