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

FIND_PATH(GSL_INCLUDE_DIR
    NAMES gsl/span
    PATH_SUFFIXES include/ gsl
    PATHS
    ${GSL_ROOT}
    $ENV{GSL_ROOT}
    /usr/
    /usr/local/
    ${PROJECT_SOURCE_DIR}/external/gsl/include
)

IF(GSL_INCLUDE_DIR)
    SET(GSL_FOUND TRUE)
ELSE(GSL_INCLUDE_DIR)
    SET(GSL_FOUND FALSE)
ENDIF(GSL_INCLUDE_DIR)

IF(GSL_FOUND)
    MESSAGE(STATUS "Found gsl in ${GSL_INCLUDE_DIR}")
ELSE(GSL_FOUND)
    IF(GSL_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find \"gsl\" library. please run\n"
                            "\tgit submodule init\n"
                            "\tgit submodule update\n")
    ELSE()
        add_definitions(-DGSL_TERMINATE_ON_CONTRACT_VIOLATION)
    ENDIF(GSL_FIND_REQUIRED)
ENDIF(GSL_FOUND)

MARK_AS_ADVANCED(
    GSL_INCLUDE_DIR
)

