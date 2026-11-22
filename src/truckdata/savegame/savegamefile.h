// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "truckdata/savegame/siiunit.h"

#include <QByteArray>
#include <QIODevice>
#include <QString>

#include <expected>

namespace TruckData::SaveGame
{
    /// Why an SII file cannot be read. An application that translates its messages switches over the kind;
    /// message() is the English text.
    struct SiiError
    {
        enum class Kind
        {
            /// Shorter than the "ScsC" header.
            CutShort,
            CannotDecrypt,
            CannotUnpack,
            NotBinary,
            /// A data block names a structure no definition before it declared.
            UndefinedStructure,
            /// The data stops in the middle of a value.
            EndsInValue,
            NotText,
            /// A value type this reader does not know; valueType and field say which.
            UnknownValueType,
            /// Neither "ScsC", "BSII" nor "SiiNunit".
            NotASaveGame,
            /// The file, or what it says it inflates to, is above MaxFileSize or MaxInflatedSize.
            TooLarge,
        };

        Kind kind = Kind::NotASaveGame;
        quint32 valueType = 0;
        // Initialized, so `SiiError{.kind = ...}` leaves no member out (-Wmissing-field-initializers).
        QString field = {}; // NOLINT(readability-redundant-member-init)

        [[nodiscard]] QString message() const;
    };

    /// The SII files of Euro Truck Simulator 2 and American Truck Simulator come in three shapes:
    ///
    /// - "ScsC": encrypted with AES-256-CBC under a key every copy of the games shares (the one the community's
    ///   SII_Decrypt uses), then zlib-compressed. Inside is one of the two below.
    /// - "BSII": binary. Blocks of structure definitions (a type and its fields with their value types), each
    ///   followed by data blocks that fill one in. Read as the games 1.62 write it, version 3.
    /// - "SiiN": text, which the games write with `g_save_format "2"` in config.cfg.
    ///
    /// Reads any of them into its units, or says why it cannot. docs/save-game-format.md describes the format byte
    /// by byte, and what is read from which unit.
    ///
    /// A save game can come from anywhere, so the file and what it inflates to are limited: a long career is a few
    /// tens of megabytes, and these leave room for a hundred times that.
    std::expected<SiiDocument, SiiError> readSiiFile(const QByteArray& file);

    /// The same for a file, which is not read at all once it is over MaxFileSize.
    std::expected<SiiDocument, SiiError> readSiiFile(QIODevice& device);

    /// The largest file readSiiFile takes, in bytes.
    inline constexpr qint64 MaxFileSize = 256LL * 1024 * 1024;

    /// The largest size a file may say it inflates to, in bytes.
    inline constexpr quint64 MaxInflatedSize = 512ULL * 1024 * 1024;

    /// The steps of readSiiFile, for the tests.
    std::expected<QByteArray, SiiError> decrypt(const QByteArray& file);
    std::expected<SiiDocument, SiiError> parseBinary(const QByteArray& data);
    std::expected<SiiDocument, SiiError> parseText(const QByteArray& data);
} // namespace TruckData::SaveGame
