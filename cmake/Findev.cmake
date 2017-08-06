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

FIND_PATH(EV_INCLUDE_DIR
  NAMES ev.h
  PATH_SUFFIXES include/
  PATHS
  ${EV_ROOT}
  $ENV{EV_ROOT}
  /usr/
)

IF(EV_INCLUDE_DIR)
  SET(EV_FOUND TRUE)
ELSE(EV_INCLUDE_DIR)
  SET(EV_FOUND FALSE)
ENDIF(EV_INCLUDE_DIR)

FIND_LIBRARY(EV_LIB
    NAMES ev
    PATH_SUFFIXES lib/
    PATHS
    ${EV_ROOT}
    $ENV{EV_ROOT}
    /usr/
    /usr/local/
)


IF(EV_FOUND)
  MESSAGE(STATUS "Found libev in ${EV_INCLUDE_DIR}")
ELSE(EV_FOUND)
  IF(EV_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find \"libev\"")
  ENDIF(EV_FIND_REQUIRED)
ENDIF(EV_FOUND)

MARK_AS_ADVANCED(
    EV_LIB
)
MARK_AS_ADVANCED(
  EV_INCLUDE_DIR
)

