// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace TruckData::SaveGame
{
    /// One unit of an SII file ("economy : _nameless.74dbb118 { ... }"): its type, its id, and its values by
    /// name. A value is a string, a number, a bool, or a list of them; ids of other units are strings.
    struct SiiUnit
    {
        QString type;
        QString id;
        QHash<QString, QVariant> values;

        [[nodiscard]] QString text(const QString& key) const;
        /// A whole number; 0 for one that is missing or not a number.
        [[nodiscard]] qint64 number(const QString& key) const;
        /// A number with a fraction (a float of the file); 0 for one that is missing or not a number.
        [[nodiscard]] double real(const QString& key) const;
        [[nodiscard]] QStringList texts(const QString& key) const;
        [[nodiscard]] QList<qint64> numbers(const QString& key) const;
        [[nodiscard]] QList<double> reals(const QString& key) const;
    };

    /// The units of a file, found by id or by type.
    class SiiDocument
    {
    public:
        void add(SiiUnit unit);

        [[nodiscard]] const SiiUnit* unit(const QString& id) const;
        /// The first unit of a type, for those a save has once (economy, player, bank).
        [[nodiscard]] const SiiUnit* first(const QString& type) const;
        [[nodiscard]] QList<const SiiUnit*> ofType(const QString& type) const;
        [[nodiscard]] qsizetype size() const;
        /// Every unit, in the order of the file.
        [[nodiscard]] const QList<SiiUnit>& units() const;

    private:
        QList<SiiUnit> m_units;
        QHash<QString, qsizetype> m_byId;
    };
} // namespace TruckData::SaveGame
