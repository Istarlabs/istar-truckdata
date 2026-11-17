# Claude Code Instructions

istar-truckdata reads what Euro Truck Simulator 2 and American Truck Simulator keep on disk: the save games
(SII files: encrypted `ScsC`, binary `BSII`, text `SiiN`) and the game's `.scs` archives (HashFS v2) for the names
of cities, companies and cargoes. It is a Qt 6 Core library under LGPL-3.0-or-later; applications (istar-wheelstar
first) add it as a git submodule with `add_subdirectory()`. It only reads - it never writes a save game or an archive.

## Shared guidelines

Imported from the shared devscripts checkout, so a rule is changed there once and reaches every
project. `install.py --claude` adds the line for a block that `Scripts/project.toml` enables later.
Claude Code asks once per project whether these files outside the repository may be loaded.

@~/dev/tools/devscripts/claude/base.md
@~/dev/tools/devscripts/claude/cpp.md

## This project

- **Qt Core only.** No Gui, no KDE Frameworks, no i18n: the library's texts are English (`SiiError::message()`), and
  an application translates by switching over the error's `Kind`. A KDE dependency here would force it on every user.
- **Public API = everything under `src/truckdata/`**, included as `"truckdata/<area>/<file>.h"`. Applications rely on
  the names; a rename is a breaking change and goes into the commit message as such.
- **`Game`'s numbers are fixed** (0 unknown, 1 ETS2, 2 ATS): applications store them.
- **Nothing application-specific**: no database, no telemetry, no deduplication against what an application already
  knows. That lives in the application (Wheelstar's `newDeliveries`).
- `docs/save-game-format.md` and `docs/game-archives.md` describe the formats byte by byte. A new field read from a save
  game is documented there in the same change, with what it was checked against.
- `cityhash.*` is a port of Google's CityHash 1.0.3 and stays MIT; every other file is LGPL-3.0-or-later.
- clang-tidy is clean (`run-clang-tidy -p build "$PWD/(src|tests)/"`); a `NOLINT` names its check and says why.
