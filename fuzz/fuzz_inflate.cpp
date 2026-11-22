// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

// The bounded inflate with a size and a stream that need not agree: the first four bytes are the size the "file"
// claims, the rest is the stream.

#include "truckdata/internal/inflate.h"

#include <QByteArrayView>
#include <QtEndian>

#include <cstddef>
#include <cstdint>
#include <utility>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    constexpr quint64 Limit = 64ULL * 1024 * 1024;

    if (size < 4)
    {
        return 0;
    }

    const auto claimed = qFromLittleEndian<quint32>(data);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): the fuzzer hands over bytes as unsigned char
    const QByteArrayView stream(reinterpret_cast<const char*>(data) + 4, static_cast<qsizetype>(size - 4));
    const auto inflated = TruckData::Internal::inflate(stream, claimed, Limit);

    // What it promises: exactly the size that was claimed, or an error.
    if (inflated && std::cmp_not_equal(inflated->size(), claimed))
    {
        __builtin_trap();
    }

    return 0;
}
