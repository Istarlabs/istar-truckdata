// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <cstdint>

namespace TruckData
{
    /// One of the two games. The numbers are fixed: applications store them.
    enum class Game : std::uint8_t
    {
        Unknown = 0,
        EuroTruckSimulator2 = 1,
        AmericanTruckSimulator = 2,
    };
} // namespace TruckData
