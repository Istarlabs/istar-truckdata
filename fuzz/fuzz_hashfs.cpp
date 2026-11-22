// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

// HashFS archives, built around what the fuzzer makes of them. zlib's checksum would stop nearly every mutation of a
// real archive before the reader looked at it, so the target compresses the parts itself:
//
// - index: the entry table, the descriptor table and the data behind the descriptors, with a header whose counts are
//   honest or not (the case of the wrapping count);
// - content: a well-formed archive whose directory listings and file are the fuzzer's bytes.

#include "truckdata/gamedata/gamenames.h"
#include "truckdata/gamedata/hashfsarchive.h"

#include "hashfsbuilder.h"

#include <fuzzer/FuzzedDataProvider.h>

#include <cstddef>
#include <cstdint>

using namespace Qt::StringLiterals;
using namespace TestSupport;
using namespace TruckData::GameData;

namespace
{
    QByteArray bytes(FuzzedDataProvider& provider, size_t most)
    {
        const auto taken = provider.ConsumeBytes<char>(provider.ConsumeIntegralInRange<size_t>(0, most));
        return QByteArray(taken.data(), static_cast<qsizetype>(taken.size()));
    }

    /// What the header says a count is: the honest one, one off by a multiple of 2^`wrap`, or any number. The table's
    /// size is the count times 16 (words times 4), so a count that differs by a multiple of 2^28 (2^30) has the same
    /// size in 32 bits: the numbers a wrapping product lets through. Random numbers hit those about once in 10^6 tries.
    quint32 claimed(FuzzedDataProvider& provider, quint32 honest, unsigned wrap)
    {
        const quint32 largest = (1U << (32U - wrap)) - 1U;

        switch (provider.ConsumeIntegralInRange<int>(0, 2))
        {
            case 0:
                return honest;
            case 1:
                return honest + (provider.ConsumeIntegralInRange<quint32>(0, largest) << wrap);
            default:
                return provider.ConsumeIntegral<quint32>();
        }
    }

    QByteArray indexArchive(FuzzedDataProvider& provider)
    {
        constexpr quint64 HeaderSize = 52;
        const QByteArray table = bytes(provider, 2048);
        const QByteArray descriptors = bytes(provider, 2048);
        const quint32 count = claimed(provider, static_cast<quint32>(table.size() / 16), 28);
        const quint32 words = claimed(provider, static_cast<quint32>(descriptors.size() / 4), 30);
        // What the descriptors' offsets point into.
        const QByteArray region = bytes(provider, 4096);

        const QByteArray packedTable = deflate(table);
        const QByteArray packedDescriptors = deflate(descriptors);
        const quint64 tableStart = HeaderSize + static_cast<quint64>(region.size());

        QByteArray header("SCS#");
        put<quint16>(header, 2);
        put<quint16>(header, 0);
        header.append("CITY");
        put<quint32>(header, count);
        put<quint32>(header, static_cast<quint32>(packedTable.size()));
        put<quint32>(header, words);
        put<quint32>(header, static_cast<quint32>(packedDescriptors.size()));
        put<quint64>(header, tableStart);
        put<quint64>(header, tableStart + static_cast<quint64>(packedTable.size()));
        put<quint64>(header, 0);
        return header + region + packedTable + packedDescriptors;
    }

    QByteArray contentArchive(FuzzedDataProvider& provider)
    {
        std::map<QString, std::pair<QByteArray, bool>> entries;
        entries[u""_s] = {bytes(provider, 256), true};
        entries[u"def"_s] = {bytes(provider, 256), true};
        entries[u"def/city"_s] = {bytes(provider, 256), true};
        const auto file = provider.ConsumeRemainingBytes<char>();
        entries[u"def/city/x.sui"_s] = {QByteArray(file.data(), static_cast<qsizetype>(file.size())), false};
        return archive(entries);
    }
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    const bool index = provider.ConsumeBool();
    const auto opened = HashFsArchive::fromData(index ? indexArchive(provider) : contentArchive(provider));

    if (!opened)
    {
        return 0;
    }

    for (const QString& path : {u""_s, u"def"_s, u"def/city"_s, u"def/city/x.sui"_s, u"locale"_s})
    {
        (void)opened->list(path);
        (void)opened->read(path);
    }

    (void)namesFrom({*opened});
    return 0;
}
