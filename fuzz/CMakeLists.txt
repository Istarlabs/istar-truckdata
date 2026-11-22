# SPDX-FileCopyrightText: 2026 Pascal Klose
# SPDX-License-Identifier: LGPL-3.0-or-later

# libFuzzer comes with clang. `python3 Scripts/cpp.py fuzz` configures this tree with the compiler and the
# sanitizer flags (-fsanitize=fuzzer-no-link for the library, below -fsanitize=fuzzer for the targets).
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "The fuzz targets need clang (libFuzzer): python3 Scripts/cpp.py fuzz")
endif()

foreach(target fuzz_hashfs fuzz_inflate fuzz_sii)
    add_executable(${target} ${target}.cpp)
    target_link_libraries(${target} PRIVATE istar-truckdata)
    target_link_options(${target} PRIVATE -fsanitize=fuzzer)
endforeach()

# The archives the tests build are the ones the fuzz target builds its own around.
target_include_directories(fuzz_hashfs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../tests)
