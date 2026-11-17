# SPDX-FileCopyrightText: 2026 Pascal Klose
# SPDX-License-Identifier: LGPL-3.0-or-later

find_package(Qt6 REQUIRED COMPONENTS Test)
# The save game test writes its own ScsC files, so it encrypts with libcrypto too.
find_package(OpenSSL REQUIRED COMPONENTS Crypto)

foreach(test gamedatatest savegametest)
    add_executable(${test} ${test}.cpp)
    set_target_properties(${test} PROPERTIES AUTOMOC ON)
    target_link_libraries(${test} PRIVATE istar-truckdata Qt6::Test OpenSSL::Crypto)
    add_test(NAME ${test} COMMAND ${test})
endforeach()
