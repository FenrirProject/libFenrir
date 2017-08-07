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

cmake_minimum_required (VERSION 2.8.12)
project (libFenrir)
enable_language(CXX)
enable_language(C)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake; ${CMAKE_MODULE_PATH})
set(FENRIR_VERSION 0.0.1-prealpha)
set(FENRIR_ABI 0)
message(STATUS "libFenrir version ${FENRIR_VERSION}")

# default values
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()


include(GNUInstallDirs)
add_definitions(-DFENRIR_DIR_CONF="${CMAKE_INSTALL_SYSCONFDIR}")
add_definitions(-DFENRIR_DIR_PLG="${CMAKE_INSTALL_LIBDIR}/Fenrir")
add_definitions(-DFENRIR_DIR_DATA="${CMAKE_INSTALL_DATADIR}/Fenrir")
message(STATUS "SysConfDir: ${CMAKE_INSTALL_SYSCONFDIR}")
message(STATUS "Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "LibDir: ${CMAKE_INSTALL_LIBDIR}/Fenrir")
message(STATUS "DataDir: ${CMAKE_INSTALL_DATADIR}/Fenrir")

option(DYNAMIC_LIB "Build dynamic library" ON)
option(STATIC_LIB "Build static library" ON)
option(CLANG_STDLIB "Use clang's libc++" OFF)
option(BUILD_SODIUM "build the bundled libsodium" OFF)
option(CLI "BUild CLI tools" ON)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build Type")
set(FENRIR_LINKER CACHE STRING "linker to use (auto/gold/ld/bsd)")
set_property(CACHE CMAKE_BUILD_TYPE   PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
set_property(CACHE FENRIR_LINKER PROPERTY STRINGS GOLD LD BSD)
set_property(CACHE BUILD_SODIUM PROPERTY STRINGS ON OFF)

# supported linkers: gold, linux standard, bsd
if(NOT FENRIR_LINKER)
    if(CMAKE_SYSTEM_NAME MATCHES "Darwin" OR CMAKE_SYSTEM_NAME MATCHES "FreeBSD" OR CMAKE_SYSTEM_NAME MATCHES "Windows")
        set(FENRIR_LINKER BSD)
    else()
        find_program(FENRIR_GOLD ld.gold)
        if (NOT FENRIR_GOLD MATCHES "FENRIR_GOLD-NOTFOUND")
            set(FENRIR_LINKER GOLD)
        else()
            set(FENRIR_LINKER LD)
        endif()
    endif()
endif()
if(FENRIR_LINKER MATCHES GOLD)
    message(STATUS "Using the GOLD linker")
elseif(FENRIR_LINKER MATCHES LD)
    message(STATUS "Using the LD linker")
elseif(FENRIR_LINKER MATCHES BSD)
    message(STATUS "Using the BSD linker")
else()
    message(WARNING "Linker not found? Default no option.")
    set(FENRIR_LINKER BSD)
endif()

if (CLI MATCHES "ON")
    message(STATUS "Building CLI tools")
else()
    message(STATUS "NOT Building CLI tools")
endif()



# defaults: only enable LTO/PROFILING with known compilers
# exception: osx/BSD. llvm linker/profiling not default?
if ((CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU") AND
        (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin") AND (NOT ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD"))
    option(LTO "Link Time Optimization" ON)
    option(PROFILING "Profiling: speedup library" ON)
else()
    option(LTO "Link Time Optimization" OFF)
    option(PROFILING "Profiling: speedup library" OFF)
endif()

message(STATUS "Build selected: ${CMAKE_BUILD_TYPE}")

if(NOT (STATIC_LIB MATCHES "ON"))
    if (NOT (DYNAMIC_LIB MATCHES "ON"))
        message(FATAL_ERROR "Do you want to actually build the library?")
    endif()
endif()


# with LTO each compiler needs its own AR/NM/RANLIB
if(LTO MATCHES "ON")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            find_program(FENRIR_AR "llvm-ar")
            find_program(FENRIR_NM "llvm-nm")
            find_program(FENRIR_RANLIB "llvm-ranlib")
            find_program(FENRIR_LLVM_PROF llvm-profdata)
            mark_as_advanced(FORCE FENRIR_LLVM_PROF)
            if (FENRIR_LLVM_PROF MATCHES "FENRIR_LLVM_PROF-NOTFOUND")
                message(FATAL_ERROR "Profiling support with clang requires \"llvm-profdata\". Disable profiling with cmake option \"-DPROFILING=OFF\"")
            endif()
        elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            find_program(FENRIR_AR "gcc-ar")
            find_program(FENRIR_NM "gcc-nm")
            find_program(FENRIR_RANLIB "gcc-ranlib")
        endif()
        # check if we found the linker
        if (FENRIR_AR STREQUAL "FENRIR_AR-NOTFOUND" OR FENRIR_NM STREQUAL "FENRIR_NM-NOTFOUND" OR FENRIR_RANLIB STREQUAL "FENRIR_RANLIB-NOTFOUND")
            message(FATAL_ERROR "Can't find the linker programs. Please disable LTO with cmake option \"-DLTO=OFF\"")
        endif()
        set(CMAKE_AR ${FENRIR_AR})
        set(CMAKE_NM ${FENRIR_NM})
        set(CMAKE_RANLIB ${FENRIR_RANLIB})
        mark_as_advanced(FORCE FENRIR_AR FENRIR_NM FENRIR_RANLIB)
        message(STATUS "Link Time Optimization activated")
    else()
        message(FATAL_ERROR "Sorry, don't know how to do LTO with your compiler")
    endif()
else()
    message(STATUS "Link Time Optimization deactivated")
endif()

# Profiling is pretty compiler-specific....
if(PROFILING MATCHES "ON")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        message(STATUS "Profiling activated")
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            message(WARNING "Clang's profile support breaks deterministic builds!")
        endif()
    else()
        message(FATAL_ERROR "Sorry, don't know how to profile with your compiler")
    endif()
else()
    message(STATUS "Profiling deactivated")
endif()

if(CLANG_STDLIB MATCHES "ON")
    if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
        message(FATAL_ERROR "libc++ is only supported by clang")
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(STDLIB stdc++)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(CLANG_STDLIB MATCHES "ON")
        set(FENRIR_STDLIB_FLAG -stdlib=libc++)
        set(STDLIB c++)
    else()
        set(STDLIB stdc++)
    endif()
endif()


find_package(Threads REQUIRED)
find_package(git)
find_package(ev)
include_directories(${EV_INCLUDE_DIR})
find_package(FENRIR_UNBOUND)
include_directories(${FENRIR_UNBOUND_INCLUDE_DIR})
find_package(FENRIR_SODIUM REQUIRED)
include_directories(${FENRIR_SODIUM_INCLUDE_DIR})
find_package(debug_assert) # needed by type_safe
include_directories(${DEBUG_ASSERT_INCLUDE_DIR})
find_package(type_safe)
include_directories(${TYPE_SAFE_INCLUDE_DIR})
find_package(gsl)
include_directories(${GSL_INCLUDE_DIR})
# our include tree
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include(${CMAKE_CURRENT_SOURCE_DIR}/external/build_deps.cmake)

# LATEX documentation
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/doc)


if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/.git/ AND GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --sq HEAD OUTPUT_VARIABLE FENRIR_SEED)
else()
    set(FENRIR_SEED Fenrir_nongit_v${FENRIR_VERSION})
    message(WARNING "Not inside a git repository. Compiler seed is now constant.")
endif()
set(DETERMINISTIC -frandom-seed=${FENRIR_SEED})

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_definitions(-DFENRIR_WINDOWS)
    message(WARNING, "Fenrir builds are not deterministic in Windows yet")
else()
    add_definitions(-DFENRIR_UNIX)
    # used by ftok to get a key. Track both the file and the version
    add_definitions(-DFENRIR_SHMPATH="${CMAKE_INSTALL_LIBDIR}/libFenrir.so-${FENRIR_VERSION}")
endif()




SET(SOURCES src/Fenrir/v1/wrapper/include_all.cpp)

SET(HEADERS	src/Fenrir/v1/API/Report.hpp
            src/Fenrir/v1/auth/Auth.hpp
            src/Fenrir/v1/auth/Token.hpp
            src/Fenrir/v1/crypto/Crypto.hpp
            src/Fenrir/v1/crypto/Crypto_NULL.hpp
            src/Fenrir/v1/crypto/Sodium.hpp
            src/Fenrir/v1/data/AS_list.hpp
            src/Fenrir/v1/data/control/Control.hpp
            src/Fenrir/v1/data/control/Control.ipp
            src/Fenrir/v1/data/Conn0_Type.hpp
            src/Fenrir/v1/data/Conn0.hpp
            src/Fenrir/v1/data/Conn0.ipp
            src/Fenrir/v1/data/Device_ID.hpp
            src/Fenrir/v1/data/IP.hpp
            src/Fenrir/v1/data/packet/Packet.hpp
            src/Fenrir/v1/data/packet/Stream.hpp
            src/Fenrir/v1/data/packet/Stream.ipp
            src/Fenrir/v1/data/Storage_t.hpp
            src/Fenrir/v1/data/Storage_t.ipp
            src/Fenrir/v1/data/Storage.hpp
            src/Fenrir/v1/data/Storage_Raw.hpp
            src/Fenrir/v1/data/Storage_Raw.ipp
            src/Fenrir/v1/data/Token_t.hpp
            src/Fenrir/v1/data/Username.hpp
            src/Fenrir/v1/db/Db.hpp
            src/Fenrir/v1/db/Db_Fake.hpp
            src/Fenrir/v1/event/Events_Base.hpp
            src/Fenrir/v1/event/Events_all.hpp
            src/Fenrir/v1/event/IO.hpp
            src/Fenrir/v1/event/Loop.hpp
            src/Fenrir/v1/event/Loop.ipp
            src/Fenrir/v1/event/Loop_callbacks.hpp
            src/Fenrir/v1/event/Loop_callbacks.ipp
            src/Fenrir/v1/Handler.hpp
            src/Fenrir/v1/Handler.ipp
            src/Fenrir/v1/net/Connection.hpp
            src/Fenrir/v1/net/Connection.ipp
            src/Fenrir/v1/net/Connection_Control.ipp
            src/Fenrir/v1/net/Handshake.hpp
            src/Fenrir/v1/net/Handshake_ID.hpp
            src/Fenrir/v1/net/Handshake.ipp
            src/Fenrir/v1/net/Link.hpp
            src/Fenrir/v1/net/Link.ipp
            src/Fenrir/v1/net/Link_defs.hpp
            src/Fenrir/v1/net/Socket.hpp
            src/Fenrir/v1/plugin/Dynamic.hpp
            src/Fenrir/v1/plugin/Lib.hpp
            src/Fenrir/v1/plugin/Loader.hpp
            src/Fenrir/v1/plugin/Loader.ipp
            src/Fenrir/v1/plugin/Native.hpp
            src/Fenrir/v1/rate/Rate.hpp
            src/Fenrir/v1/rate/Rate.ipp
            src/Fenrir/v1/rate/RR-RR.hpp
            src/Fenrir/v1/recover/Error_Correction.hpp
            src/Fenrir/v1/recover/ECC_NULL.hpp
            src/Fenrir/v1/resolve/DNSSEC.hpp
            src/Fenrir/v1/resolve/DNSSEC.ipp
            src/Fenrir/v1/resolve/Resolver.hpp
            src/Fenrir/v1/service/Service.hpp
            src/Fenrir/v1/service/Service_ID.hpp
            src/Fenrir/v1/util/hash.hpp
            src/Fenrir/v1/util/endian.hpp
            src/Fenrir/v1/util/it_types.hpp
            src/Fenrir/v1/util/math.hpp
            src/Fenrir/v1/util/Random.hpp
            src/Fenrir/v1/util/Shared_Lock.hpp
            src/Fenrir/v1/util/Shared_Lock.ipp
            src/Fenrir/v1/util/span_overlay.hpp
            src/Fenrir/v1/util/Z85.hpp
            src/Fenrir/v1/common.hpp
            )

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    SET(HEADERS ${HEADERS}
                src/Fenrir/v1/net/platforms/Socket_Windows.hpp
                src/Fenrir/v1/plugin/platforms/Lib_Windows.hpp
                src/Fenrir/v1/plugin/platforms/Loader_Windows.cpp)
else()
    SET(PLATFORM_DEPS dl)
    SET(HEADERS ${HEADERS}
                src/Fenrir/v1/net/platforms/Socket_Unix.hpp
                src/Fenrir/v1/plugin/platforms/Lib_Unix.hpp
                src/Fenrir/v1/plugin/platforms/Loader_Unix.hpp
                )
endif()

include(cmake/compiler_flags.cmake)
include(TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if (IS_BIG_ENDIAN)
    add_definitions(-DFENRIR_BIG_ENDIAN)
    message (STATUS "Detected big endian machine")
else()
    add_definitions(-DFENRIR_LITTLE_ENDIAN)
    message (STATUS "Detected little endian machine")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_definitions(-DUSING_CLANG)
    if (LTO MATCHES "ON")
        if (FENRIR_LINKER MATCHES GOLD)
            set(LD_OPT "-flto -fuse-ld=gold" )
        else()
            set(LD_OPT "-flto" )
        endif()
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(LD_OPT "")
endif()


########################################
## TODO: if windows, add notelemetry.obj
########################################


if(PROFILING MATCHES "ON")
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(PROFILE_GET           -fprofile-instr-generate)
        set(PROFILE_GET_FILE      Fenrir.profraw)
        set(PROFILE_SET_FILE      Fenrir.profdata)
        set(PROFILE_SET           "-fprofile-instr-use=${PROFILE_SET_FILE}")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(PROFILE_GET           -fprofile-generate)
        # gcc creates a couple more files than clang for prifiling. Track them.
        set(PROFILE_GET_FILE      "nonexistant.gcda")
        set(PROFILE_SET_FILE      ${PROFILE_GET_FILE})
        set(PROFILE_SET           -fprofile-use)
    endif()


    # PRE-run: build a library, generate the profile.
    add_library(Fenrir_Static_Profiling STATIC ${SOURCES} ${HEADERS})
    add_dependencies(Fenrir_Static_Profiling ${FENRIR_SODIUM_DEP} ${FENRIR_UNBOUND_DEP})
    target_link_libraries(Fenrir_Static_Profiling ${STDLIB} ${FENRIR_UBSAN} ${CMAKE_THREAD_LIBS_INIT} ${PLATFORM_DEPS} ev ${FENRIR_SODIUM_LIB} ${FENRIR_UNBOUND_LIB})
    target_compile_options(
        Fenrir_Static_Profiling PRIVATE
        ${CXX_COMPILER_FLAGS}
        ${PROFILE_GET}
    )
    set_target_properties(Fenrir_Static_Profiling PROPERTIES COMPILER_FLAGS ${PROFILE_GET})

    # build the C example test for profiling
    add_executable(test_cpp_profiled test/test_cpp.cpp)
    add_dependencies(test_cpp_profiled Fenrir_Static_Profiling)
    target_compile_options(
        test_cpp_profiled PRIVATE
        ${C_COMPILER_FLAGS}
        ${PROFILE_GET}
    )
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
        #target_link_libraries(test_cpp_profiled Fenrir_Static_Profiling ${FENRIR_SODIUM_LIB} ${FENRIR_UNBOUND_LIB} ev ${STDLIB} m ${CMAKE_THREAD_LIBS_INIT} ${PLATFORM_DEPS})
        target_link_libraries(test_cpp_profiled ${FENRIR_SODIUM_LIB} ${FENRIR_UNBOUND_LIB} ev ${STDLIB} m ${CMAKE_THREAD_LIBS_INIT} ${PLATFORM_DEPS})
    else()
        #target_link_libraries(test_cpp_profiled Fenrir_Static_Profiling ${FENRIR_SODIUM_LIB} ${FENRIR_UNBOUND_LIB} ev ${STDLIB} ${CMAKE_THREAD_LIBS_INIT} ${PLATFORM_DEPS})
        target_link_libraries(test_cpp_profiled ${FENRIR_SODIUM_LIB} ${FENRIR_UNBOUND_LIB} ev ${STDLIB} ${CMAKE_THREAD_LIBS_INIT} ${PLATFORM_DEPS})
    endif()
    set_target_properties(test_cpp_profiled PROPERTIES LINK_FLAGS ${PROFILE_GET})

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_custom_command(
            OUTPUT ${PROFILE_GET_FILE}
            COMMAND LLVM_PROFILE_FILE=${PROFILE_GET_FILE} ./test_cpp_profiled
            DEPENDS test_cpp_profiled
            COMMENT "Running profiling test..."
            VERBATIM
        )
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_custom_command(
            OUTPUT ${PROFILE_GET_FILE}
            COMMAND ./test_cpp_profiled
            DEPENDS test_cpp_profiled
            COMMENT "Running profiling test..."
            VERBATIM
        )
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # the profile must be translated into readable form
        add_custom_command(
            OUTPUT ${PROFILE_SET_FILE}
            COMMAND llvm-profdata merge -output=${PROFILE_SET_FILE} ${PROFILE_GET_FILE}
            DEPENDS ${PROFILE_GET_FILE}
            COMMENT "Creating profile data..."
            VERBATIM
        )
    endif()
    add_custom_target(
        profile ALL
        DEPENDS ${PROFILE_SET_FILE}
    )
endif()



# build the static library
if(STATIC_LIB MATCHES "ON")
    add_library(Fenrir_Static STATIC ${SOURCES} ${HEADERS})
    if(NOT CMAKE_SYSTEM_NAME MATCHES "Windows")
        set_target_properties(Fenrir_Static PROPERTIES OUTPUT_NAME Fenrir.${FENRIR_ABI})
    endif()
    target_compile_options(
        Fenrir_Static PRIVATE
        ${CXX_COMPILER_FLAGS}
    )
    if(LTO MATCHES "ON")
        target_compile_options(Fenrir_Static PRIVATE -flto)
    endif()
    if(PROFILING MATCHES "ON")
        add_dependencies(Fenrir_Static profile)
        target_compile_options(Fenrir_Static PRIVATE ${PROFILE_SET})
    endif()
    if (NOT FENRIR_LINKER MATCHES BSD)
        set_target_properties (
            Fenrir_Static
            PROPERTIES
            LINK_FLAGS "-Wl,-z,now,-z,relro${FENRIR_EXCLUDE_SYM} -pie ${LD_OPT}"
        )
    endif()
    target_link_libraries(Fenrir_Static ${STDLIB} ${FENRIR_UBSAN} ${CMAKE_THREAD_LIBS_INIT} ${PLATFORM_DEPS} ev ${FENRIR_SODIUM_LIB} ${FENRIR_UNBOUND_LIB})
    set_property(TARGET Fenrir_Static PROPERTY ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
    if(CMAKE_SYSTEM_NAME MATCHES "Windows")
        # not really deterministic?
        # don't know how to do that in Windows.
        add_custom_target(
            make_static_deterministic ALL
            DEPENDS Fenrir_Static
        )
    else()
        # make the library deterministic (only on *nix)
        add_executable(make_deterministic src/deterministic.cpp)
        target_compile_options(
            make_deterministic PRIVATE
            ${CXX_COMPILER_FLAGS}
        )
        target_link_libraries(make_deterministic ${STDLIB} ${FENRIR_UBSAN})
        add_custom_command(
            OUTPUT deterministic.run
            COMMAND make_deterministic ${CMAKE_CURRENT_BINARY_DIR}/lib/libFenrir.${FENRIR_ABI}.a
            DEPENDS Fenrir_Static
            COMMENT "Removing creation date from library..."
            VERBATIM
        )
        add_custom_target(
            make_static_deterministic ALL
            DEPENDS deterministic.run
        )
    endif()

endif()

#build dynamic library
if(DYNAMIC_LIB MATCHES "ON")
    add_definitions(-DFENRIR_DLL)
    add_definitions(-DFENRIR_DLL_EXPORTS)

    add_library(Fenrir SHARED ${SOURCES} ${HEADERS})
    target_compile_options(
        Fenrir PRIVATE
        ${CXX_COMPILER_FLAGS}
    )
    if(LTO MATCHES "ON")
        target_compile_options(Fenrir PRIVATE -flto)
    endif()
    if(PROFILING MATCHES "ON")
        add_dependencies(Fenrir profile)
        target_compile_options(Fenrir PRIVATE ${PROFILE_SET})
    endif()
    if (FENRIR_LINKER MATCHES BSD)
        # keep the last sace at the end here, otherwise cmake gets called
        # with the wrong number of parameters in the next set_target_properties
        set(FENRIR_LINK_LIB "${LD_OPT} ")
    else()
        set(FENRIR_LINK_LIB "-Wl,-z,now,-z,relro${FENRIR_LZ4_EXCLUDE_SYM} ${LD_OPT}")
    endif()
    set_target_properties (
        Fenrir
        PROPERTIES
        LINK_FLAGS ${FENRIR_LINK_LIB}
        SOVERSION ${FENRIR_ABI}
        VERSION ${FENRIR_VERSION}
    )
    set_property(TARGET Fenrir PROPERTY LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
endif()

# Authentication server serializer
set(Fenrir_AS_Serializer_sources src/cli/Fenrir_AS_Serializer.cpp
                                external/optionparser-1.4/optionparser.h
                                src/Fenrir/v1/common.hpp
                                src/Fenrir/v1/util/Z85.hpp)
if(CLI MATCHES "ON")
    add_executable(Fenrir_AS_Serializer ${Fenrir_AS_Serializer_sources})
else()
    add_executable(Fenrir_AS_Serializer EXCLUDE_FROM_ALL ${Fenrir_AS_Serializer_sources})
endif()

target_compile_options(
    Fenrir_AS_Serializer PRIVATE
    ${CXX_COMPILER_FLAGS}
)

if(CMAKE_SYSTEM_NAME MATCHES "Windows" AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set_target_properties (
        Fenrir_AS_Serializer
        PROPERTIES
        LINK_FLAGS "notelemetry.obj"
    )
endif()
target_link_libraries(Fenrir_AS_Serializer ${FENRIR_UBSAN} ${STDLIB})

set_target_properties(Fenrir_AS_Serializer PROPERTIES OUTPUT_NAME Fenrir_AS_Serializer
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

# easy base85 converter.
set(Fenrir_base85_sources src/cli/Fenrir_base85.cpp
                                        external/optionparser-1.4/optionparser.h
                                        src/Fenrir/v1/common.hpp
                                        src/Fenrir/v1/util/Z85.hpp)
if(CLI MATCHES "ON")
    add_executable(Fenrir_base85 ${Fenrir_base85_sources})
else()
    add_executable(Fenrir_base85 EXCLUDE_FROM_ALL ${Fenrir_base85_sources})
endif()

target_compile_options(
    Fenrir_base85 PRIVATE
    ${CXX_COMPILER_FLAGS}
)

if(CMAKE_SYSTEM_NAME MATCHES "Windows" AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set_target_properties (
        Fenrir_base85
        PROPERTIES
        LINK_FLAGS "notelemetry.obj"
    )
endif()
target_link_libraries(Fenrir_base85 ${FENRIR_UBSAN} ${STDLIB})

set_target_properties(Fenrir_base85 PROPERTIES OUTPUT_NAME Fenrir_base85
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

# test server
set(Fenrir_AS_sources src/cli/Fenrir_AS.cpp
                                        external/optionparser-1.4/optionparser.h
                                        ${HEADERS})
if(CLI MATCHES "ON")
    add_executable(Fenrir_AS ${Fenrir_AS_sources})
else()
    add_executable(Fenrir_AS EXCLUDE_FROM_ALL ${Fenrir_AS_sources})
endif()

target_compile_options(
    Fenrir_AS PRIVATE
    ${CXX_COMPILER_FLAGS}
)

if(CMAKE_SYSTEM_NAME MATCHES "Windows" AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set_target_properties (
        Fenrir_AS
        PROPERTIES
        LINK_FLAGS "notelemetry.obj"
    )
endif()
add_dependencies(Fenrir_AS ${FENRIR_SODIUM_DEP} ${FENRIR_UNBOUND_DEP})
target_link_libraries(Fenrir_AS ${FENRIR_UBSAN} ${STDLIB} dl ${CMAKE_THREAD_LIBS_INIT} ${PLATFORM_DEPS} ev ${FENRIR_SODIUM_LIB} ${FENRIR_UNBOUND_LIB})

set_target_properties(Fenrir_AS PROPERTIES OUTPUT_NAME Fenrir_AS
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")


# test client
set(Fenrir_Client_sources src/cli/Fenrir_Client.cpp
                                        external/optionparser-1.4/optionparser.h
                                        ${HEADERS})
if(CLI MATCHES "ON")
    add_executable(Fenrir_Client ${Fenrir_Client_sources})
else()
    add_executable(Fenrir_Client EXCLUDE_FROM_ALL ${Fenrir_Client_sources})
endif()

target_compile_options(
    Fenrir_Client PRIVATE
    ${CXX_COMPILER_FLAGS}
)

if(CMAKE_SYSTEM_NAME MATCHES "Windows" AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set_target_properties (
        Fenrir_Client
        PROPERTIES
        LINK_FLAGS "notelemetry.obj"
    )
endif()
add_dependencies(Fenrir_Client ${FENRIR_SODIUM_DEP} ${FENRIR_UNBOUND_DEP})
target_link_libraries(Fenrir_Client ${FENRIR_UBSAN} ${STDLIB} dl ${CMAKE_THREAD_LIBS_INIT} ${PLATFORM_DEPS} ev ${FENRIR_SODIUM_LIB} ${FENRIR_UNBOUND_LIB})

set_target_properties(Fenrir_Client PROPERTIES OUTPUT_NAME Fenrir_Client
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")

add_custom_target(cli DEPENDS Fenrir_AS_Serializer Fenrir_base85 Fenrir_AS Fenrir_Client)



if(CLI MATCHES "ON")
    add_custom_target(everything DEPENDS make_static_deterministic docs cli)
else()
    add_custom_target(everything DEPENDS make_static_deterministic docs)
endif()

install(FILES ${HEADERS} DESTINATION include/Fenrir)
install(TARGETS Fenrir Fenrir_Static
     LIBRARY DESTINATION lib COMPONENT libraries
     ARCHIVE DESTINATION lib COMPONENT libraries)


