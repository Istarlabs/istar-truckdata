# SPDX-FileCopyrightText: 2026 Pascal Klose
# SPDX-License-Identifier: LGPL-3.0-or-later

cmake_minimum_required(VERSION 3.25)

project(istar-truckdata VERSION 0.1.0 LANGUAGES CXX)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)

    # At least as strict as the applications that add the library, so a warning shows up here first. An application
    # brings its own flags; these are not forced on it.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-Wall -Wextra -Wpedantic)
    endif()
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS Core)
# The games encrypt their save files with AES; OpenSSL's libcrypto decrypts them.
find_package(OpenSSL REQUIRED COMPONENTS Crypto)
# And zlib inflates them, with the size of the output under the library's control (Qt's qUncompress has none).
find_package(ZLIB REQUIRED)

add_library(istar-truckdata STATIC
    src/truckdata/game.h
    src/truckdata/logging.cpp
    src/truckdata/logging.h
    src/truckdata/internal/inflate.cpp
    src/truckdata/internal/inflate.h
    src/truckdata/gamedata/cityhash.cpp
    src/truckdata/gamedata/cityhash.h
    src/truckdata/gamedata/gamenames.cpp
    src/truckdata/gamedata/gamenames.h
    src/truckdata/gamedata/hashfsarchive.cpp
    src/truckdata/gamedata/hashfsarchive.h
    src/truckdata/savegame/career.cpp
    src/truckdata/savegame/career.h
    src/truckdata/savegame/company.cpp
    src/truckdata/savegame/company.h
    src/truckdata/savegame/ids.h
    src/truckdata/savegame/job.h
    src/truckdata/savegame/savegamefile.cpp
    src/truckdata/savegame/savegamefile.h
    src/truckdata/savegame/savegamelocator.cpp
    src/truckdata/savegame/savegamelocator.h
    src/truckdata/savegame/siidump.cpp
    src/truckdata/savegame/siidump.h
    src/truckdata/savegame/siiunit.cpp
    src/truckdata/savegame/siiunit.h
)
add_library(IstarTruckData::TruckData ALIAS istar-truckdata)

# std::expected
target_compile_features(istar-truckdata PUBLIC cxx_std_23)
target_include_directories(istar-truckdata PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(istar-truckdata PUBLIC Qt6::Core PRIVATE OpenSSL::Crypto ZLIB::ZLIB)
# Linked into a shared library or a position-independent executable as well.
set_target_properties(istar-truckdata PROPERTIES POSITION_INDEPENDENT_CODE ON)

# The tests come with the library on its own, not when an application adds it with add_subdirectory().
option(ISTAR_TRUCKDATA_TESTS "Build the tests of istar-truckdata" ${PROJECT_IS_TOP_LEVEL})

if(ISTAR_TRUCKDATA_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# The libFuzzer targets of the readers, which `python3 Scripts/cpp.py fuzz` builds with clang and the sanitizers.
option(ISTAR_TRUCKDATA_FUZZ "Build the fuzz targets of istar-truckdata" OFF)

if(ISTAR_TRUCKDATA_FUZZ)
    add_subdirectory(fuzz)
endif()
