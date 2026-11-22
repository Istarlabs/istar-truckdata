// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "truckdata/savegame/siidump.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaType>
#include <QStringList>

#include <algorithm>

namespace TruckData::SaveGame
{
    namespace
    {
        /// The largest whole number a double holds exactly: 2^53.
        constexpr qint64 LargestExactInteger = 9007199254740992;

        // NOLINTNEXTLINE(misc-no-recursion): a list may hold lists; the depth is the file's, a few levels at most
        QString textOf(const QVariant& value)
        {
            if (value.metaType().id() != QMetaType::QVariantList)
            {
                return value.toString();
            }

            QStringList parts;

            for (const QVariant& element : value.toList())
            {
                parts.append(textOf(element));
            }

            return QLatin1Char('[') + parts.join(QStringLiteral(", ")) + QLatin1Char(']');
        }

        // NOLINTNEXTLINE(misc-no-recursion): same as textOf
        QJsonValue jsonOf(const QVariant& value)
        {
            switch (value.metaType().id())
            {
                case QMetaType::QVariantList:
                {
                    QJsonArray array;

                    for (const QVariant& element : value.toList())
                    {
                        array.append(jsonOf(element));
                    }

                    return array;
                }
                case QMetaType::ULongLong:
                    return value.toULongLong() > static_cast<quint64>(LargestExactInteger)
                        ? QJsonValue(value.toString())
                        : QJsonValue(value.toLongLong());
                case QMetaType::LongLong:
                {
                    // Not std::llabs: it is undefined for the smallest number, which a damaged file can hold.
                    const qint64 number = value.toLongLong();
                    return number < -LargestExactInteger || number > LargestExactInteger ? QJsonValue(value.toString())
                                                                                         : QJsonValue(number);
                }
                default:
                    return QJsonValue::fromVariant(value);
            }
        }
    } // namespace

    QByteArray dumpText(const SiiDocument& document)
    {
        QString result;

        for (const SiiUnit& unit : document.units())
        {
            result += unit.type + QStringLiteral(" : ") + unit.id + QStringLiteral(" {\n");

            QStringList keys = unit.values.keys();
            std::ranges::sort(keys);

            for (const QString& key : keys)
            {
                result += QStringLiteral("  ") + key + QStringLiteral(": ") + textOf(unit.values.value(key)) +
                    QLatin1Char('\n');
            }

            result += QStringLiteral("}\n\n");
        }

        return result.toUtf8();
    }

    QByteArray dumpJson(const SiiDocument& document)
    {
        QJsonArray units;

        for (const SiiUnit& unit : document.units())
        {
            QJsonObject values;

            for (auto it = unit.values.cbegin(); it != unit.values.cend(); ++it)
            {
                values.insert(it.key(), jsonOf(it.value()));
            }

            units.append(QJsonObject{
                {QStringLiteral("type"), unit.type},
                {QStringLiteral("id"), unit.id},
                {QStringLiteral("values"), values},
            });
        }

        return QJsonDocument(units).toJson(QJsonDocument::Indented);
    }
} // namespace TruckData::SaveGame
