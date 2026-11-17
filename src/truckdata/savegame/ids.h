// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>

#include <utility>

/// How the save game writes the ids of things, and how to take them apart. The library's own helpers, not part of
/// what it offers.
namespace TruckData::SaveGame::Ids
{
    /// "company.volatile.posped.bremen" is the company "posped" in the city "bremen"; anything else is neither.
    inline std::pair<QString, QString> companyAndCity(const QString& reference)
    {
        const QStringList parts = reference.split(u'.');

        if (parts.size() < 4)
        {
            return {};
        }

        return {parts.at(2), parts.at(3)};
    }

    /// The part after the first dot: "cargo.sandwch_pnls" is "sandwch_pnls", "garage.hannover" is "hannover".
    inline QString afterKind(const QString& id)
    {
        return id.section(u'.', 1);
    }
} // namespace TruckData::SaveGame::Ids
