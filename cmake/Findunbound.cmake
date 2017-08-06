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

FIND_PATH(UNBOUND_INCLUDE_DIR
  NAMES unbound.h
  PATH_SUFFIXES include/
  PATHS
  ${UNBOUND_ROOT}
  $ENV{UNBOUND_ROOT}
  /usr/
)

IF(UNBOUND_INCLUDE_DIR)
  SET(UNBOUND_FOUND TRUE)
ELSE(UNBOUND_INCLUDE_DIR)
  SET(UNBOUND_FOUND FALSE)
ENDIF(UNBOUND_INCLUDE_DIR)

IF(UNBOUND_FOUND)
  MESSAGE(STATUS "Found libunbound in ${UNBOUND_INCLUDE_DIR}")
ELSE(UNBOUND_FOUND)
  IF(UNBOUND_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find \"libunbound\"")
  ENDIF(UNBOUND_FIND_REQUIRED)
ENDIF(UNBOUND_FOUND)

MARK_AS_ADVANCED(
  UNBOUND_INCLUDE_DIR
)

