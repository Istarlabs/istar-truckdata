# istar-truckdata

A Qt 6 library that reads what **Euro Truck Simulator 2** and **American Truck Simulator** keep on disk:

- **Save games** (`game.sii`, `profile.sii`) in all three shapes the games write: encrypted (`ScsC`), binary (`BSII`)
  and text (`SiiN`). Every unit with every value, and on top of that what a player cares about: the career, the
  delivery and bus job logs, the offences, and the company - garages, hired drivers with their skills and jobs,
  trucks, buses and trailers with their wear, the bank, the profit logs and the repairs paid on the road.
- **The game's archives** (`.scs`, HashFS version 2), for the names of cities, countries, companies and cargoes in
  every language the game has.
- **Where they are**: the profiles in Steam's cloud folder and in the Proton prefix, in every Steam library, and the
  installed games.

It only reads. It never writes a save game or an archive.

## Using it

As a git submodule, added with CMake:

```bash
git submodule add https://github.com/Istarlabs/istar-truckdata.git external/istar-truckdata
```

```cmake
add_subdirectory(external/istar-truckdata)
target_link_libraries(your-app PRIVATE IstarTruckData::TruckData)
```

It needs Qt 6.8 or newer (Core only), OpenSSL's libcrypto and a C++23 compiler. Fedora:
`sudo dnf install cmake gcc-c++ qt6-qtbase-devel openssl-devel`.

```cpp
#include "truckdata/savegame/career.h"
#include "truckdata/savegame/savegamefile.h"
#include "truckdata/savegame/savegamelocator.h"

using namespace TruckData::SaveGame;

for (const SaveGameProfile& profile : findProfiles())
{
    QFile file(profile.newestSave);

    if (!file.open(QIODevice::ReadOnly))
    {
        continue;
    }

    const auto document = readSiiFile(file.readAll());

    if (!document)
    {
        qWarning() << document.error().message();
        continue;
    }

    const Career career = careerFrom(*document);
    qInfo() << profile.name << career.experience << "XP," << career.money << "in the bank";

    for (const Job& job : deliveriesFrom(*document))
    {
        qInfo() << job.sourceCityId << "->" << job.destinationCityId << job.cargoId << job.revenue;
    }
}
```

`SiiDocument` gives every unit of the file by id or by type, for what the typed functions do not cover yet.
`GameData::readGameNames()` reads the names of an installed game; `findGames()` says where it is installed.

Errors are `SiiError`: a `Kind` to switch over, and `message()` in English. A translated application maps the kinds
to its own texts.

## Building on its own

```bash
cmake -B build && cmake --build build && ctest --test-dir build
```

The tests are built when the library is the top-level project (`ISTAR_TRUCKDATA_TESTS`), not when an application adds
it. The `Scripts/` files are the maintainer's shims into a shared tool checkout; plain CMake is all a user needs.

## Fuzzing

A save game or an archive can come from anywhere, so the readers are meant to answer a damaged or hostile file with an
error and nothing else: no exception, no read outside a buffer, no allocation the file chose the size of. The tests
cover the cases that were found by hand; `fuzz/` has libFuzzer targets for the rest (`fuzz_sii`, `fuzz_inflate`,
`fuzz_hashfs`), with seeds in `fuzz/seeds/`. They need clang:

```bash
cmake -B build-fuzz -DCMAKE_CXX_COMPILER=clang++ -DISTAR_TRUCKDATA_FUZZ=ON \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined,fuzzer-no-link" -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-fuzz --target fuzz_sii && build-fuzz/fuzz/fuzz_sii corpus fuzz/seeds/fuzz_sii -max_total_time=60
```

The maintainer's `python3 Scripts/cpp.py fuzz` does the same, and `cpp.py verify` runs the tests under ASan, UBSan and
LeakSanitizer as well (`[cpp] sanitize`).

## The formats

SCS Software documents the telemetry SDK, but not its save games or archives. What this library knows about them comes
from the community's work, checked against real files of ETS2 1.5x:

- [docs/save-game-format.md](docs/save-game-format.md): the three shapes of an SII file byte by byte, and what is read
  from which unit
- [docs/game-archives.md](docs/game-archives.md): HashFS version 2, CityHash 1.0.3, the definitions and translations

| What | Where from | Used as |
|---|---|---|
| The save game's encryption (`ScsC`: AES-256-CBC, the shared key, zlib) and the binary format `BSII` | [SII_Decrypt](https://github.com/TheLazyTomcat/SII_Decrypt) by František Milt (MPL-2.0) | Documentation; `savegamefile.cpp` is our own code |
| What the positions of a `delivery_log_entry`'s `params` mean | [ETS2-ATS-Trip-Exporter](https://github.com/ChristianVassallo/ETS2-ATS-Trip-Exporter) (GPL-3.0) | Documentation; `career.cpp` is our own code |
| The archive format HashFS version 2 (`.scs`) | [Archive::SCS::HashFS2](https://metacpan.org/pod/Archive::SCS::HashFS2) (Perl, CPAN) | Documentation; `hashfsarchive.cpp` is our own code |
| CityHash64, version 1.0.3, which the archives look their files up by | [CityHash](https://github.com/google/cityhash) by Geoff Pike and Jyrki Alakuijala, Google (MIT) | **Ported code**: `cityhash.cpp` carries its MIT notice |

No game data is shipped: the names are read from the player's own installation.

## Licence

LGPL-3.0-or-later (`LICENSES/LGPL-3.0-or-later.txt`, which builds on `LICENSES/GPL-3.0-or-later.txt`): any application
may use the library, also one under another licence; changes to the library itself stay open.
`src/truckdata/gamedata/cityhash.cpp` and `cityhash.h` are MIT (`LICENSES/MIT.txt`), as Google's CityHash they are
ported from.

Used by [istar-wheelstar](https://github.com/Istarlabs/istar-wheelstar).
