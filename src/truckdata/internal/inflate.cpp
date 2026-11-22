// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "truckdata/internal/inflate.h"

#include <zlib.h>

namespace TruckData::Internal
{
    std::expected<QByteArray, InflateError> inflate(QByteArrayView deflated, quint64 size, quint64 limit)
    {
        if (size > limit)
        {
            return std::unexpected(InflateError::TooLarge);
        }

        // Deflate cannot make more of its input than about a thousand times as much, so a stream that claims more
        // is no stream of that size, and no buffer is made for it. (The 6 bytes of the zlib wrapper count too.)
        constexpr quint64 LargestRatio = 1100;

        if (size > (static_cast<quint64>(deflated.size()) + 6) * LargestRatio)
        {
            return std::unexpected(InflateError::Corrupt);
        }

        QByteArray inflated(static_cast<qsizetype>(size), Qt::Uninitialized);
        auto inflatedSize = static_cast<uLongf>(size);
        auto deflatedSize = static_cast<uLong>(deflated.size());

        // uncompress2 stops at the end of the stream and says Z_BUF_ERROR for one that does not fit, which is what
        // keeps the buffer at the size the file claimed.
        // NOLINTBEGIN(cppcoreguidelines-pro-type-reinterpret-cast): zlib takes bytes as unsigned char
        const int result = uncompress2(reinterpret_cast<Bytef*>(inflated.data()), &inflatedSize,
                                       reinterpret_cast<const Bytef*>(deflated.data()), &deflatedSize);
        // NOLINTEND(cppcoreguidelines-pro-type-reinterpret-cast)

        if (result != Z_OK || inflatedSize != size)
        {
            return std::unexpected(InflateError::Corrupt);
        }

        return inflated;
    }
} // namespace TruckData::Internal
