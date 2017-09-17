#
# Copyright (c) 2015-2016-2017, Luca Fulchir<luca@fulchir.it>, All rights reserved.
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
#

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

#string of multiple flags to list
string(REPLACE " " ";" FENRIR_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE " " ";" FENRIR_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

##############################################
#### check for UBSAN sanitizing support ######
##############################################
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    find_package(ubsan)
    if (UBSAN_FOUND)
        check_c_compiler_flag("-fsanitize=undefined" FENRIR_FLAG_C_UBSAN)
        check_cxx_compiler_flag("-fsanitize=undefined" FENRIR_FLAG_CXX_UBSAN)
        if (FENRIR_FLAG_C_UBSAN AND FENRIR_FLAG_CXX_UBSAN)
            set(FENRIR_ENABLE_UBSAN TRUE)
            set(FENRIR_UBSAN ubsan)
            message(STATUS "UBSAN sanitizing support enabled")
        else()
            message(STATUS "UBSAN sanitizing support disabled")
        endif()
    else()
        message(STATUS "UBSAN sanitizing support disabled")
    endif()
    add_definitions(-DFENRIR_DEBUG=true)
else()
    add_definitions(-DFENRIR_DEBUG=false)
endif()

###################
## Compiler options
###################

#gnu options
set(FENRIR_GNU_C_OPTIONS ${FENRIR_DETERMINISTIC} -std=c11 -ffast-math
            -Wno-unknown-pragmas -Wall -Wextra -pedantic -Wno-padded
            -fstack-protector-all -fstrict-aliasing -fwrapv -fvisibility=hidden
            -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict
            -Wnull-dereference -Wdouble-promotion -Wshadow -Wformat=2)
set(FENRIR_GNU_CXX_OPTIONS ${FENRIR_DETERMINISTIC} -std=c++14 -ffast-math
            -fno-rtti -fno-exceptions -Wno-unknown-pragmas -Wall -Wextra
            -pedantic -Wno-padded -Wno-unknown-pragmas -Wno-weak-vtables
            -fstack-protector-all -fstrict-aliasing -fwrapv -fvisibility=hidden
            -fvisibility-inlines-hidden
            -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict
            -Wnull-dereference -Wuseless-cast -Wdouble-promotion -Wshadow -Wformat=2)

# GCC internal compiler errors with:
# -fsanitize=undefined
# -fsanitize=null
# -fsanitize=signed-integer-overflow
set(FENRIR_GNU_C_DEBUG   -O0 -g -Wno-aggressive-loop-optimizations -ftrapv )
set(FENRIR_GNU_CXX_DEBUG -O0 -g -Wno-aggressive-loop-optimizations -ftrapv )
set(FENRIR_GNU_C_DEBUG_SANITIZE   -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=vla-bound -fsanitize=return -fisolate-erroneous-paths-dereference -fisolate-erroneous-paths-attribute )
set(FENRIR_GNU_CXX_DEBUG_SANITIZE -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=vla-bound -fsanitize=return -fisolate-erroneous-paths-dereference -fisolate-erroneous-paths-attribute )
set(FENRIR_GNU_C_MINSIZEREL   -Os )
set(FENRIR_GNU_CXX_MINSIZEREL -Os )
set(FENRIR_GNU_C_RELEASE   -Ofast -DNDEBUG -fwrapv -ftree-loop-distribution -funroll-loops )
set(FENRIR_GNU_CXX_RELEASE -Ofast -DNDEBUG -fwrapv -ftree-loop-distribution -funroll-loops )
set(FENRIR_GNU_C_RELWITHDEBINFO   -g -Ofast -fwrapv -ftree-loop-distribution -funroll-loops )
set(FENRIR_GNU_CXX_RELWITHDEBINFO -g -Ofast -fwrapv -ftree-loop-distribution -funroll-loops )

# clang options
set(FENRIR_CLANG_C_OPTIONS ${FENRIR_DETERMINISTIC} -std=c11 -ffast-math
    -fno-math-errno -Wall -Wextra -pedantic -Weverything -Wno-padded
            -fstack-protector-all -fstrict-aliasing -Wformat -Wformat-security
            -Wno-disabled-macro-expansion -fvisibility=hidden
            -fvisibility-inlines-hidden
            -Wdouble-promotion -Wshadow -Wformat=2 -Wnull-dereference)
set(FENRIR_CLANG_CXX_OPTIONS ${FENRIR_STDLIB_FLAG} ${FENRIR_DETERMINISTIC}
            -std=c++14 -Wno-c++14-extensions -fno-rtti -fno-exceptions
            -ffast-math -fno-math-errno -Wall -pedantic -Weverything
            -Wno-c++98-compat-pedantic -Wno-c++98-compat -Wno-padded
            -Wno-unknown-pragmas -Wno-weak-vtables -fstack-protector-all
            -fstrict-aliasing -Wformat -Wformat-security -fvisibility=hidden
            -fvisibility-inlines-hidden
            -Wno-documentation -Wno-documentation-unknown-command
            -Wdouble-promotion -Wshadow -Wformat=2 -Wnull-dereference)

set(FENRIR_CLANG_C_DEBUG   -O0 -g )
set(FENRIR_CLANG_CXX_DEBUG -O0 -g )
set(FENRIR_CLANG_C_DEBUG_SANITIZE   -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=vla-bound -fsanitize=return )
set(FENRIR_CLANG_CXX_DEBUG_SANITIZE -fsanitize=shift -fsanitize=integer-divide-by-zero -fsanitize=vla-bound -fsanitize=return )
set(FENRIR_CLANG_C_MINSIZEREL   -Os )
set(FENRIR_CLANG_CXX_MINSIZEREL -Os )
set(FENRIR_CLANG_C_RELEASE   -Ofast -DNDEBUG -fwrapv )
set(FENRIR_CLANG_CXX_RELEASE -Ofast -DNDEBUG -fwrapv )
set(FENRIR_CLANG_C_RELWITHDEBINFO   -g -Ofast -fwrapv )
set(FENRIR_CLANG_CXX_RELWITHDEBINFO -g -Ofast -fwrapv )

# msvc flags. todo?
set(FENRIR_MSVC_C_OPTIONS /wd4068)
set(FENRIR_MSVC_CXX_OPTIONS /wd4068)


###################
# combine the above
###################
# note: generator expansions do not work on "set(..)"
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(FENRIR_LIST_CXX_COMPILER_FLAGS ${FENRIR_CXX_FLAGS} ${FENRIR_CLANG_CXX_OPTIONS})
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        set(FENRIR_LIST_CXX_COMPILER_FLAGS ${FENRIR_LIST_CXX_COMPILER_FLAGS} ${FENRIR_CLANG_CXX_DEBUG})
        if (FENRIR_ENABLE_UBSAN)
            set(FENRIR_LIST_CXX_COMPILER_FLAGS ${FENRIR_LIST_CXX_COMPILER_FLAGS} ${FENRIR_CLANG_CXX_DEBUG_SANITIZE})
        endif()
    elseif(CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
        set(FENRIR_LIST_CXX_COMPILER_FLAGS ${FENRIR_LIST_CXX_COMPILER_FLAGS} ${FENRIR_CLANG_CXX_MINSIZEREL})
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        set(FENRIR_LIST_CXX_COMPILER_FLAGS ${FENRIR_LIST_CXX_COMPILER_FLAGS} ${FENRIR_CLANG_CXX_RELEASE})
    elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
        set(FENRIR_LIST_CXX_COMPILER_FLAGS ${FENRIR_LIST_CXX_COMPILER_FLAGS} ${FENRIR_CLANG_CXX_RELWITHDEBINFO})
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(FENRIR_LIST_CXX_COMPILER_FLAGS ${FENRIR_CXX_FLAGS} ${FENRIR_GNU_CXX_OPTIONS})
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        set(FENRIR_LIST_CXX_COMPILER_FLAGS ${FENRIR_LIST_CXX_COMPILER_FLAGS} ${FENRIR_GNU_CXX_DEBUG})
        if (FENRIR_ENABLE_UBSAN)
            set(FENRIR_LIST_CXX_COMPILER_FLAGS ${FENRIR_LIST_CXX_COMPILER_FLAGS} ${FENRIR_GNU_CXX_DEBUG_SANITIZE})
        endif()
    elseif(CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
        set(FENRIR_LIST_CXX_COMPILER_FLAGS ${FENRIR_LIST_CXX_COMPILER_FLAGS} ${FENRIR_GNU_CXX_MINSIZEREL})
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        set(FENRIR_LIST_CXX_COMPILER_FLAGS ${FENRIR_LIST_CXX_COMPILER_FLAGS} ${FENRIR_GNU_CXX_RELEASE})
    elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
        set(FENRIR_LIST_CXX_COMPILER_FLAGS ${FENRIR_LIST_CXX_COMPILER_FLAGS} ${FENRIR_GNU_CXX_RELWITHDEBINFO})
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(FENRIR_LIST_CXX_COMPILER_FLAGS ${FENRIR_CXX_FLAGS} ${FENRIR_MSVC_CXX_OPTIONS})
endif()


if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(FENRIR_LIST_C_COMPILER_FLAGS ${FENRIR_C_FLAGS} ${FENRIR_CLANG_C_OPTIONS})
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        set(FENRIR_LIST_C_COMPILER_FLAGS ${FENRIR_LIST_C_COMPILER_FLAGS} ${FENRIR_CLANG_C_DEBUG})
        if (FENRIR_ENABLE_UBSAN)
            set(FENRIR_LIST_C_COMPILER_FLAGS ${FENRIR_LIST_C_COMPILER_FLAGS} ${FENRIR_CLANG_C_DEBUG_SANITIZE})
        endif()
    elseif(CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
        set(FENRIR_LIST_C_COMPILER_FLAGS ${FENRIR_LIST_C_COMPILER_FLAGS} ${FENRIR_CLANG_C_MINSIZEREL})
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        set(FENRIR_LIST_C_COMPILER_FLAGS ${FENRIR_LIST_C_COMPILER_FLAGS} ${FENRIR_CLANG_C_RELEASE})
    elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
        set(FENRIR_LIST_C_COMPILER_FLAGS ${FENRIR_LIST_C_COMPILER_FLAGS} ${FENRIR_CLANG_C_RELWITHDEBINFO})
    endif()
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU")
    set(FENRIR_LIST_C_COMPILER_FLAGS ${FENRIR_C_FLAGS} ${FENRIR_GNU_C_OPTIONS})
    if(CMAKE_BUILD_TYPE MATCHES "Debug")
        set(FENRIR_LIST_C_COMPILER_FLAGS ${FENRIR_LIST_C_COMPILER_FLAGS} ${FENRIR_GNU_C_DEBUG})
        if (FENRIR_ENABLE_UBSAN)
            set(FENRIR_LIST_C_COMPILER_FLAGS ${FENRIR_LIST_C_COMPILER_FLAGS} ${FENRIR_GNU_C_DEBUG_SANITIZE})
        endif()
    elseif(CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
        set(FENRIR_LIST_C_COMPILER_FLAGS ${FENRIR_LIST_C_COMPILER_FLAGS} ${FENRIR_GNU_C_MINSIZEREL})
    elseif(CMAKE_BUILD_TYPE MATCHES "Release")
        set(FENRIR_LIST_C_COMPILER_FLAGS ${FENRIR_LIST_C_COMPILER_FLAGS} ${FENRIR_GNU_C_RELEASE})
    elseif(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
        set(FENRIR_LIST_C_COMPILER_FLAGS ${FENRIR_LIST_C_COMPILER_FLAGS} ${FENRIR_GNU_C_RELWITHDEBINFO})
    endif()
elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    set(FENRIR_LIST_C_COMPILER_FLAGS ${FENRIR_C_FLAGS} ${FENRIR_MSVC_C_OPTIONS})
endif()


########################################
## Test all the previously enabled flags
########################################

set(FENRIR_C_COMPILER_FLAGS "")
set(FENRIR_CXX_COMPILER_FLAGS "")

# check each C flag
foreach(flag ${FENRIR_LIST_C_COMPILER_FLAGS})
    string(REPLACE "-" "_" FENRIR_FLAG2 ${flag})
    string(REPLACE "+" "_" FENRIR_FLAG1 ${FENRIR_FLAG2})
    string(REPLACE "=" "_" FENRIR_FLAG  ${FENRIR_FLAG1})
    check_c_compiler_flag(${flag} FENRIR_FLAG_C_${FENRIR_FLAG})
    if(FENRIR_FLAG_C_${FENRIR_FLAG})
        set(C_COMPILER_FLAGS ${C_COMPILER_FLAGS} ${flag})
    else()
        message(WARNING "flag \"${flag}\" can't be used in your C compiler")
    endif()
endforeach()

# check each c++ flag
foreach(flag ${FENRIR_LIST_CXX_COMPILER_FLAGS})
    string(REPLACE "-" "_" FENRIR_FLAG2 ${flag})
    string(REPLACE "+" "_" FENRIR_FLAG1 ${FENRIR_FLAG2})
    string(REPLACE "=" "_" FENRIR_FLAG  ${FENRIR_FLAG1})
    check_cxx_compiler_flag(${flag} FENRIR_FLAG_CXX_${FENRIR_FLAG})
    if(FENRIR_FLAG_CXX_${FENRIR_FLAG})
        set(CXX_COMPILER_FLAGS ${CXX_COMPILER_FLAGS} ${flag})
    else()
        message(WARNING "flag \"${flag}\" can't be used in your CXX compiler")
    endif()
endforeach()

