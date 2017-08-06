#
# Copyright (c) 2016-2017, Luca Fulchir<luca@fulchir.it>, All rights reserved.
#
# This file is part of "libFenrir".
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

PROJECT(libFenrir_Doc)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

INCLUDE(../cmake/UseLATEX.cmake)

IF (LATEX_COMPILER AND PDFLATEX_COMPILER AND MAKEINDEX_COMPILER)
ADD_LATEX_DOCUMENT(libFenrir.tex
	INPUTS GNU_FDL.tex
	USE_INDEX
	MANGLE_TARGET_NAMES
	NO_DEFAULT
	)

ADD_CUSTOM_TARGET(docs DEPENDS libFenrir_pdf)
ELSE()
ADD_CUSTOM_TARGET(docs)
ENDIF()

INSTALL(FILES ${CMAKE_BINARY_DIR}/doc/libFenrir.pdf DESTINATION share/doc/ OPTIONAL)

