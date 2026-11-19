// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "truckdata/savegame/siiunit.h"

#include <QByteArray>

namespace TruckData::SaveGame
{
    /// Every unit of the document as readable text, UTF-8: one block per unit in the order of the file,
    ///
    ///     economy : _nameless.74748338 {
    ///       experience_points: 314114
    ///       visited_cities: [hannover, bremen]
    ///     }
    ///
    /// with the values sorted by name. For looking at what a save game holds, not for reading it back.
    QByteArray dumpText(const SiiDocument& document);

    /// The same as JSON, UTF-8: an array with one object per unit, `{"type": ..., "id": ..., "values": {...}}`.
    /// Numbers stay numbers, lists stay arrays. A whole number a double cannot hold exactly (the games write
    /// 64-bit hashes) becomes a string, so no reader rounds it.
    QByteArray dumpJson(const SiiDocument& document);
} // namespace TruckData::SaveGame
