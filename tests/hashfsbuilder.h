// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "truckdata/gamedata/cityhash.h"

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QtEndian>

#include <array>
#include <bit>
#include <map>

/// What the tests and the fuzz targets build archives from. Not part of the library.
namespace TestSupport
{
    template <typename T>
    void put(QByteArray& data, T value)
    {
        const auto bytes = std::bit_cast<std::array<char, sizeof(T)>>(qToLittleEndian(value));
        data.append(bytes.data(), sizeof(T));
    }

    /// A 24-bit number and a byte of flags, as the descriptors store sizes and offsets.
    inline void putThreeBytes(QByteArray& data, quint32 value, quint8 flags)
    {
        put<quint16>(data, static_cast<quint16>(value & 0xFFFFU));
        put<quint8>(data, static_cast<quint8>(value >> 16U));
        put<quint8>(data, flags);
    }

    inline QByteArray deflate(const QByteArray& data)
    {
        // qCompress puts the size in front, which the archive keeps in the descriptor instead.
        return qCompress(data).mid(4);
    }

    /// A directory listing: a count, a byte per name with its length, the names.
    inline QByteArray listing(const QStringList& names)
    {
        QByteArray data;
        put<quint32>(data, static_cast<quint32>(names.size()));

        for (const QString& name : names)
        {
            data.append(static_cast<char>(name.toUtf8().size()));
        }

        for (const QString& name : names)
        {
            data.append(name.toUtf8());
        }

        return data;
    }

    /// An archive in HashFS version 2 as the games write it: a header, the data of each entry (zlib, 16-byte
    /// aligned), the entry table and the descriptor table, both zlib.
    inline QByteArray archive(const std::map<QString, std::pair<QByteArray, bool>>& entries)
    {
        constexpr qsizetype DataStart = 0x60;
        QByteArray data(DataStart, '\0');
        QByteArray table;
        QByteArray descriptors;
        QByteArray dataHeaders;
        const auto count = static_cast<quint32>(entries.size());
        quint32 index = 0;

        for (const auto& [path, entry] : entries)
        {
            const auto& [content, directory] = entry;
            const QByteArray stored = deflate(content);
            const auto offset = static_cast<quint32>(data.size() / 16);
            data.append(stored);
            data.append(QByteArray((16 - (data.size() % 16)) % 16, '\0'));

            put<quint64>(table, TruckData::GameData::cityHash64(path.toUtf8()));
            put<quint32>(table, index);
            put<quint16>(table, 1);
            put<quint16>(table, directory ? 1 : 0);

            // The descriptors come first, one word per entry; the data headers after them, four words each.
            putThreeBytes(descriptors, count + (index * 4), 0x80);
            putThreeBytes(dataHeaders, static_cast<quint32>(stored.size()), 0x10);
            putThreeBytes(dataHeaders, static_cast<quint32>(content.size()), 0);
            put<quint32>(dataHeaders, 0);
            put<quint32>(dataHeaders, offset);
            ++index;
        }

        descriptors.append(dataHeaders);
        const QByteArray packedTable = deflate(table);
        const QByteArray packedDescriptors = deflate(descriptors);
        const auto tableStart = static_cast<quint64>(data.size());
        data.append(packedTable);
        const auto descriptorsStart = static_cast<quint64>(data.size());
        data.append(packedDescriptors);

        QByteArray header("SCS#");
        put<quint16>(header, 2);
        put<quint16>(header, 0);
        header.append("CITY");
        put<quint32>(header, count);
        put<quint32>(header, static_cast<quint32>(packedTable.size()));
        put<quint32>(header, static_cast<quint32>(descriptors.size() / 4));
        put<quint32>(header, static_cast<quint32>(packedDescriptors.size()));
        put<quint64>(header, tableStart);
        put<quint64>(header, descriptorsStart);
        put<quint64>(header, 0);
        data.replace(0, header.size(), header);
        return data;
    }
} // namespace TestSupport
