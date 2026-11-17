# The save game format of Euro Truck Simulator 2 and American Truck Simulator

What `src/truckdata/savegame/` reads, and why it reads it the way it does. SCS publishes no specification; what follows comes
from the community's tools (SII_Decrypt, the ETS2-ATS-Trip-Exporter) and was checked against real saves of 2026
(ETS2 1.5x, BSII version 3).

## Where the saves are

`findProfiles()` (`savegamelocator.cpp`) looks in two places, in every Steam library:

| Where | What |
|---|---|
| `<steam>/userdata/<account>/<app id>/remote/profiles/<profile>/` | Profiles the Steam Cloud keeps |
| `<library>/steamapps/compatdata/<app id>/pfx/drive_c/users/steamuser/Documents/<game>/{profiles,steam_profiles}/<profile>/` | Local profiles, in the Proton prefix |

The app id is 227300 for ETS2 and 270880 for ATS. The libraries come from `steamapps/libraryfolders.vdf`.
`~/.steam/steam` is a link to `~/.local/share/Steam`, so the same folder can turn up under two names; the canonical
path removes the second.

A profile folder is named after the profile in hex: `447269766572` is "Driver". Its `profile.sii` has the
name in `user_profile.profile_name`. Every save is a folder under `save/` (`autosave`, `autosave_drive_2`, `1`, …)
with a `game.sii`; the newest `game.sii` by modification time is the one read.

## The three shapes of an SII file

`readSiiFile()` tells them apart by the first four bytes:

| Magic | Shape | Read by |
|---|---|---|
| `ScsC` | Encrypted and compressed; one of the two below inside | `decrypt()` |
| `BSII` | Binary | `parseBinary()` |
| `SiiN` | Text (`SiiNunit { … }`) | `parseText()` |

The games write `ScsC` with `BSII` inside. The text shape comes from the console's `g_save_format 2`, from tools
that decrypt a save, and from the tests, which write it by hand.

Older games also wrote `3nK`, a plain byte scrambling. It is not read here.

### `ScsC`: the encryption

| Offset | Size | Content |
|---|---|---|
| 0 | 4 | `ScsC` |
| 4 | 32 | An HMAC over the data; not checked, the key it would need is the same public one |
| 36 | 16 | The AES initialisation vector |
| 52 | 4 | The size after inflating, little-endian |
| 56 | … | The data |

The data is AES-256-CBC under a key every copy of the games shares. It is public in SII_Decrypt and is `Key` in
`savegamefile.cpp`. It is decrypted without removing a padding: zlib ignores whatever follows the end of its stream.

What comes out is a zlib stream (deflate with a zlib header). Qt's `qUncompress()` inflates it, but wants the
inflated size in front of the stream as four bytes big-endian; the header has it little-endian, so `decrypt()` turns
it around and puts it in front. A result of another size is an error, not a short save.

### `BSII`: the binary shape

Every number is little-endian. After `BSII` and a `u32` version (3), the file is a row of blocks up to its end,
each beginning with a `u32`:

**`0`: a structure definition.** A `u8` says whether it is valid; one that is not is only a marker, with nothing
after it. A valid one has:

- a `u32` number, the structure's id that data blocks refer to,
- a string, the type of its units: `economy`, `delivery_log_entry`, …,
- its fields, each a `u32` value type and a string name, up to a value type of `0`.

A field of type `0x37` (an ordinal, a kind of enum) also brings its names: a `u32` count, then as many pairs of a
`u32` value and a string name.

A string is always a `u32` length and as many bytes of UTF-8, without a terminator.

**Any other number: a data block** of the structure with that number. The unit's id comes first, then one value per
field, in the order of the definition, with no names and no lengths in between. A value of an unknown type therefore
cannot be skipped; `parseBinary()` stops with an error instead of reading on out of step.

#### Value types

A list is always a `u32` count followed by the elements.

| Type | Value | Read as |
|---|---|---|
| `0x01` / `0x02` | String / list | Text |
| `0x03` / `0x04` | Token / list | Text (see below) |
| `0x05` / `0x06` | `float` / list | Number |
| `0x07` / `0x08` | 2 × `float` / list | Skipped |
| `0x09` / `0x0A` | 3 × `float`, a vector / list | Skipped |
| `0x11` / `0x12` | 3 × `i32` / list | Skipped |
| `0x17` / `0x18` | 4 × `float`, a quaternion / list | Skipped |
| `0x19` / `0x1A` | Placement, 32 bytes / list | Skipped |
| `0x25` / `0x26` | `i32` / list | Number |
| `0x27` / `0x28` | `u32` / list | Number |
| `0x2B` / `0x2C` | `u16` / list | Number |
| `0x2F` | `u32` | Number |
| `0x31` / `0x32` | `i64` / list | Number |
| `0x33` / `0x34` | `u64` / list | Number |
| `0x35` / `0x36` | `bool` as `u8` / list | Truth value |
| `0x37` | Ordinal, `u32` | The name from the definition |
| `0x39`, `0x3B`, `0x3D` | Id | Text |
| `0x3A`, `0x3C`, `0x3E` | List of ids | Texts |

A placement (`0x19`) is a position of three floats, four bytes of padding and a quaternion: 3 × 4 + 4 + 4 × 4 = 32
bytes. Nothing here needs vectors, quaternions or placements, so they are skipped by their size.

#### Tokens and ids

A **token** packs up to twelve characters of `0-9a-z_` into a `u64`: base 38, the lowest digit first, 0 meaning
"no more characters". A character's digit is its position in `0123456789abcdefghijklmnopqrstuvwxyz_` plus 1.
Reading one: subtract 1, take the remainder by 38 as the character, divide by 38, repeat until 0.

An **id** names a unit and is how units refer to each other:

- a `u8` count of parts, then as many tokens, joined by dots: two parts are `company.volatile`, …
- a count of `0xFF` is a nameless unit: a `u64` follows, written `_nameless.<hex>` the way the text shape writes it.

### `SiiN`: the text shape

```
SiiNunit
{
economy : _nameless.74dbb118 {
 experience_points: 314114
 visited_cities: 2
 visited_cities[0]: hannover
 visited_cities[1]: bremen
 satisfaction: &3f3b7fd0
}
}
```

- A unit begins with `type : id {` and ends with `}`, each on a line of its own.
- An array gives its length first (`key: 2`), then its elements (`key[0]: …`), sometimes as `key[]: …` without an
  index. `parseText()` collects the elements and ignores the length.
- `&3f3b7fd0` is a `float` given by its bits in hex, exactly as it is in memory; decimal digits would round it.
- Text that can hold spaces is quoted. Everything else (tokens, ids, vectors such as `(1, 2, 3)`) stays text.
  `nil` is an empty reference.

## What is read from it

The result is an `SiiDocument`: units by id and by type. Everything starts at the one `economy` unit, which refers
to the rest by id.

### The career (`careerFrom()`)

| Value | From |
|---|---|
| Experience, game time | `economy.experience_points`, `economy.game_time` (in-game minutes since day 1) |
| Money | `bank.money_account`, the unit `economy.bank` refers to |
| Kilometres, real and game time by vehicle | `economy.total_distances_by_mode`, `total_real_times_by_mode`, `total_game_times_by_mode` |
| Cancelled jobs, cities | `cancelled_job_count`, the length of `visited_cities` |
| Garages | The garages of `economy.garages` with a `status` other than 0 (see *The company*) |
| Fuel, visits | `total_fuel_litres`, `total_fuel_price`, `gas_station_visit_count`, `service_visit_count` |
| Records | `clean_distance_max`/`_counter`, `no_cargo_damage_distance_max`, `no_violation_distance_max`/`_counter` |
| Ferry crossings | The sum of `use_count` over the entries of `economy.ferry_log` |
| Offences noticed | `police_ctrl.offence_counter` |
| Offences fined, fines paid | `police_offence_log.offence_total_counts`, `offence_total_fines` |
| Truck deliveries, bus jobs | The jobs read below, counted per vehicle |

**The "by mode" lists** have one entry per kind of vehicle: truck, bus, car. Nothing says so; a real save
confirms it. Its five bus jobs (`bus_job_log`) add up to 3,099 km and the second entry is 3,849 km: those jobs plus
driving a bus without one.

**The offence lists** have 14 entries in the order of the SDK's `fine_offence`: crash, avoid_sleeping, wrong_way,
speeding_camera, no_lights, red_signal, speeding, avoid_weighing, illegal_trailer, avoid_inspection,
illegal_border_crossing, hard_shoulder_violation, damaged_vehicle_usage, generic. `perOffence()` pads or cuts a list
to that length, so one from another version of the game does not shift the offences. `offence_counter` counts every
offence the game noticed, also with fines turned off; the two `offence_total_*` lists count only the fined ones.

### The company (`companyFrom()`)

What follows was checked against a save of ETS2 1.5x with 58 garages, 57 hired drivers, 58 trucks and 24 trailers.

**Garages.** `economy.garages` lists a `garage` unit for every city of the map, `garage.<city>`, owned or not: 237 in
that save. One the player owns has a `status` other than 0; seen were 2 with three slots and 3 with five (the
headquarters is one of them). `vehicles` and `drivers` are the slots, as long as each other: the truck and the driver
of slot *n* belong together. An empty slot is an empty id. `trailers` lists the garage's trailers, `productivity` (a
float, 0 to 1) is how busy it is, `profit_log` its log.

**Drivers.** `player.drivers` lists every driver of the company: one `driver_player` (the player, in a slot of one
garage like anyone else) and the `driver_ai` units. A hired driver has

| Field | Content |
|---|---|
| `hometown`, `current_city` | City ids |
| `experience_points` | Experience |
| `adr` | The ADR classes as bits, one each: 1, 3, 7, 15, 31, 63 |
| `long_dist`, `heavy`, `fragile`, `urgent`, `mechanical` | The other skills, 0 to 6, in the game's order: long distance, high-value cargo, fragile cargo, just-in-time delivery, ecodriving |
| `driver_job` | A `job_info`: `cargo`, `source_company`, `target_company` (`company.volatile.<company>.<city>`), `planned_distance_km` |
| `state` | 1, 2 or 4; what they stand for is not known, every driver of that save had a job |
| `profit_log` | The driver's log |

The save holds no names of drivers, only their ids (`driver.136`).

**Trucks and trailers.** `player.trucks` and `player.trailers` list them. A vehicle's model is the accessory whose
`data_path` is its definition, `/def/vehicle/truck/<model>/data.sii` (`trailer_owned` for a trailer).
`license_plate` is markup for the game's font, `H<offset …><img …><offset …>JV 5319|germany`: each run of tags stands
for a gap, the country follows the bar. Wear is per part, 0 to 1: `engine_wear`, `transmission_wear`, `cabin_wear`,
`chassis_wear`, and `wheels_wear` per wheel; each has an `_unfixable` share as well, not read. `odometer` is
kilometres, `fuel_relative` the tank. A trailer's body type is `body_type` of its `trailer_definition`.

**Profit logs.** A `profit_log` has `stats_data`, a list of `profit_log_entry`: `revenue`, `wage`, `maintenance`,
`fuel`, `distance` (`distance_on_job` whether on a job), `cargo_count`, `cargo`, `source_city`, `source_company`,
`destination_city`, `destination_company` and `timestamp_day`, the in-game day. The game keeps only the last days: in
that save the player's log reached back seven in-game days, twelve entries. In a hired driver's entries
`maintenance` is the truck's upkeep on the job. In the player's own log the bills come as entries of their own,
without distance: one with only `fuel`, one with only `maintenance` - a repair (53,457 on one day of that save).

**The bank.** `money_account`, `loan_limit`, `loans` (a list; that save had none, so a loan's fields are not known),
`coinsurance_fixed` and `coinsurance_ratio` (what the player pays of an accident: a fixed sum and a share), and
`overdraft`.

### Truck jobs: `delivery_log`

`economy.delivery_log` has `entries`, each a `delivery_log_entry` with nothing but `params`: a list of texts whose
meaning is their position. The positions are those the ETS2-ATS-Trip-Exporter reads, checked against a real save:

| Index | Content | Example |
|---|---|---|
| 0 | Game time at the end (in-game minutes) | `397719` |
| 1 | Source, `company.volatile.<company>.<city>` | `company.volatile.polarislines.isafjordur` |
| 2 | Destination, the same way | `company.volatile.blatt_voruhu.thorshofn` |
| 3 | Cargo, `cargo.<id>` | `cargo.sandwch_pnls` |
| 4 | Experience | `1162` |
| 5 | Revenue | `35851` |
| 6 | Kilometres driven | `1002` |
| 7 | Cargo damage, 0 to 1 | `0.016` |
| 14 | Fines during the job | `250` |
| 16 | The truck | `vehicle.volvo.fh_2024` |
| 18 | The job market | `on_compn` |

The other positions are not read; what they mean is not certain. A save of 2026 has 24 per entry;
`deliveriesFrom()` takes any list of at least 19.

Entries without a source and destination are **free driving without a job** (market `freerm`). They are no jobs and
do not count as deliveries.

The markets: `quick` quick job, `compn` freight market, `cargo` cargo market, `on_compn` external contract (World of
Trucks), `spec_oversize` special transport.

### Bus jobs: `bus_job_log`

Bus jobs are not in the delivery log. `economy.bus_job_log` has `entries`, each a `bus_job_log_entry`, and these,
unlike the delivery log's, have named values:

| Field | Content | Example |
|---|---|---|
| `type` | The market | `quick_job`, `company_job` |
| `assigned_bus` | The bus | `vehicle.volvo.9700_23` |
| `source_bus_stop_name`, `target_bus_stop_name` | First and last stop, `city.<id>` | `city.zurich` |
| `stage_count` | Stops | `2` |
| `total_passengers` | Passengers over all stops | `84` |
| `exp_delta` | Experience | `438` |
| `reward` | Revenue | `4515` |
| `travelled_distance` | Kilometres | `265` |
| `start_time`, `end_time` | Game time at the start and the end | `400620`, `400862` |
| `satisfaction` | The passengers' satisfaction, 0 to 1 (`float`) | `0.878` |
| `flags` | 1 so far | |

What `flags` means is not known. Every entry seen so far has 1 and was finished, so every one is read as delivered;
a bus job given up may look different there.

`economy.car_job_log` is there too, and so far always empty, apparently for a car mode to come. It is not read.
