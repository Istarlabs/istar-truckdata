// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

// The save game readers and what is read out of their units. The first byte chooses the reader: 0 the binary
// shape, 1 the text shape, 2 any file (the "ScsC" header, the size checks, the decryption), the rest is the file.
// Nothing a damaged file holds may throw, run out of memory or read outside a buffer.

#include "truckdata/savegame/career.h"
#include "truckdata/savegame/company.h"
#include "truckdata/savegame/savegamefile.h"
#include "truckdata/savegame/siidump.h"

#include <cstddef>
#include <cstdint>

using namespace TruckData::SaveGame;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size == 0)
    {
        return 0;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast): the fuzzer hands over bytes as unsigned char
    const QByteArray file(reinterpret_cast<const char*>(data) + 1, static_cast<qsizetype>(size - 1));
    std::expected<SiiDocument, SiiError> document;

    switch (data[0] % 3)
    {
        case 0:
            document = parseBinary(file);
            break;
        case 1:
            document = parseText(file);
            break;
        default:
            document = readSiiFile(file);
            break;
    }

    if (!document)
    {
        (void)document.error().message();
        return 0;
    }

    // What the application does with a document it was given.
    (void)careerFrom(*document);
    (void)deliveriesFrom(*document);
    (void)companyFrom(*document);
    (void)dumpText(*document);
    (void)dumpJson(*document);
    return 0;
}
