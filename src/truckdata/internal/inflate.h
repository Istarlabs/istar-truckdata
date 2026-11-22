// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QByteArrayView>

#include <expected>

/// Not part of the library's API: what the readers share, free to change without a word in a commit message.
namespace TruckData::Internal
{
    enum class InflateError
    {
        /// The inflated size the file states is above the limit.
        TooLarge,
        /// Not zlib, or it does not inflate to exactly the size the file states.
        Corrupt,
    };

    /// zlib data whose inflated size the file states.
    ///
    /// That size is a claim of the file, not a fact: it is refused above `limit`, or as corrupt when the stream is too
    /// short to ever make that much, before anything is allocated; the output buffer is exactly that big, and a
    /// stream that wants more room or ends short of it is corrupt. What follows the end of the stream in `deflated`
    /// (the padding of an encrypted block) is ignored.
    ///
    /// qUncompress does none of this: it takes the size as a hint and doubles its buffer for as long as the stream
    /// goes on, so a few hundred kilobytes of zlib can make it hold gigabytes.
    std::expected<QByteArray, InflateError> inflate(QByteArrayView deflated, quint64 size, quint64 limit);
} // namespace TruckData::Internal
