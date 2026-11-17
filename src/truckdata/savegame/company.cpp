// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "truckdata/savegame/company.h"
#include "truckdata/savegame/ids.h"

#include <QHash>
#include <QRegularExpression>

#include <bit>
#include <numeric>

using namespace Qt::StringLiterals;

namespace TruckData::SaveGame
{
    namespace
    {
        using Ids::afterKind;
        using Ids::companyAndCity;

        /// A profit log's entries, oldest first.
        QList<ProfitEntry> profitLog(const SiiDocument& document, const QString& logId)
        {
            QList<ProfitEntry> entries;
            const SiiUnit* log = document.unit(logId);

            for (const QString& id : log != nullptr ? log->texts(u"stats_data"_s) : QStringList())
            {
                const SiiUnit* unit = document.unit(id);

                if (unit == nullptr)
                {
                    continue;
                }

                entries.append({.day = unit->number(u"timestamp_day"_s),
                                .revenue = unit->number(u"revenue"_s),
                                .wage = unit->number(u"wage"_s),
                                .maintenance = unit->number(u"maintenance"_s),
                                .fuel = unit->number(u"fuel"_s),
                                .kilometres = unit->number(u"distance"_s),
                                .onJob = unit->number(u"distance_on_job"_s) != 0,
                                .cargoCount = static_cast<int>(unit->number(u"cargo_count"_s)),
                                .cargoId = unit->text(u"cargo"_s),
                                .sourceCityId = unit->text(u"source_city"_s),
                                .sourceCompanyId = unit->text(u"source_company"_s),
                                .destinationCityId = unit->text(u"destination_city"_s),
                                .destinationCompanyId = unit->text(u"destination_company"_s)});
            }

            return entries;
        }

        /// A vehicle's definition, from the accessory that is its data: "/def/vehicle/truck/scania.r_2016/data.sii"
        /// is "scania.r_2016" of the kind "truck".
        QString modelOf(const SiiDocument& document, const SiiUnit& vehicle, const QString& kind)
        {
            const QString prefix = u"/def/vehicle/"_s + kind + u'/';

            for (const QString& id : vehicle.texts(u"accessories"_s))
            {
                const SiiUnit* accessory = document.unit(id);
                const QString path = accessory != nullptr ? accessory->text(u"data_path"_s) : QString();

                if (path.startsWith(prefix) && path.endsWith(u"/data.sii"_s))
                {
                    return path.sliced(prefix.size(), path.size() - prefix.size() - 9);
                }
            }

            return {};
        }

        std::optional<DriverJob> jobOf(const SiiDocument& document, const SiiUnit& driver)
        {
            const SiiUnit* job = document.unit(driver.text(u"driver_job"_s));

            if (job == nullptr || job->text(u"cargo"_s).isEmpty())
            {
                return std::nullopt;
            }

            DriverJob result;
            result.cargoId = afterKind(job->text(u"cargo"_s));
            std::tie(result.sourceCompanyId, result.sourceCityId) = companyAndCity(job->text(u"source_company"_s));
            std::tie(result.destinationCompanyId, result.destinationCityId) =
                companyAndCity(job->text(u"target_company"_s));
            result.plannedKilometres = job->number(u"planned_distance_km"_s);
            return result;
        }

        /// What belongs to which garage: a truck and a driver by sharing a slot, a trailer by being in its list.
        struct Places
        {
            QHash<QString, QString> garageOf;
            QHash<QString, QString> driverOfTruck;
            QHash<QString, QString> truckOfDriver;
        };

        Garage garageFrom(const SiiDocument& document, const QString& id, const SiiUnit& unit, Places& places)
        {
            Garage garage;
            garage.cityId = afterKind(id);
            const QStringList trucks = unit.texts(u"vehicles"_s);
            const QStringList drivers = unit.texts(u"drivers"_s);
            garage.slotCount = static_cast<int>(trucks.size());

            for (qsizetype slot = 0; slot < trucks.size(); ++slot)
            {
                const QString& truck = trucks.at(slot);
                const QString driver = drivers.value(slot);

                if (!truck.isEmpty())
                {
                    garage.truckIds.append(truck);
                    places.garageOf.insert(truck, garage.cityId);
                    places.driverOfTruck.insert(truck, driver);
                }

                if (!driver.isEmpty())
                {
                    garage.driverIds.append(driver);
                    places.garageOf.insert(driver, garage.cityId);
                    places.truckOfDriver.insert(driver, truck);
                }
            }

            for (const QString& trailer : unit.texts(u"trailers"_s))
            {
                if (!trailer.isEmpty())
                {
                    ++garage.trailers;
                    places.garageOf.insert(trailer, garage.cityId);
                }
            }

            garage.productivity = unit.real(u"productivity"_s);
            garage.log = profitLog(document, unit.text(u"profit_log"_s));
            return garage;
        }

        Driver driverFrom(const SiiDocument& document, const QString& id, const SiiUnit& unit, const Places& places)
        {
            Driver driver;
            driver.id = id;
            driver.garageCityId = places.garageOf.value(id);
            driver.hometownId = unit.text(u"hometown"_s);
            driver.currentCityId = unit.text(u"current_city"_s);
            driver.experience = unit.number(u"experience_points"_s);
            driver.adr = std::popcount(static_cast<quint64>(unit.number(u"adr"_s)));
            driver.longDistance = static_cast<int>(unit.number(u"long_dist"_s));
            driver.highValue = static_cast<int>(unit.number(u"heavy"_s));
            driver.fragile = static_cast<int>(unit.number(u"fragile"_s));
            driver.justInTime = static_cast<int>(unit.number(u"urgent"_s));
            driver.ecoDriving = static_cast<int>(unit.number(u"mechanical"_s));
            driver.job = jobOf(document, unit);
            driver.truckId = places.truckOfDriver.value(id);
            driver.log = profitLog(document, unit.text(u"profit_log"_s));
            return driver;
        }

        /// A truck, or a bus: the same unit, its definition under /def/vehicle/<kind>/.
        Truck truckFrom(const SiiDocument& document, const QString& id, const SiiUnit& unit, const Places& places,
                        const QString& kind)
        {
            Truck truck;
            truck.id = id;
            truck.model = modelOf(document, unit, kind);
            std::tie(truck.licensePlate, truck.plateCountry) = licensePlate(unit.text(u"license_plate"_s));
            truck.odometer = unit.number(u"odometer"_s);
            truck.engineWear = unit.real(u"engine_wear"_s);
            truck.transmissionWear = unit.real(u"transmission_wear"_s);
            truck.cabinWear = unit.real(u"cabin_wear"_s);
            truck.chassisWear = unit.real(u"chassis_wear"_s);

            if (const QList<double> wheels = unit.reals(u"wheels_wear"_s); !wheels.isEmpty())
            {
                truck.wheelsWear =
                    std::accumulate(wheels.cbegin(), wheels.cend(), 0.0) / static_cast<double>(wheels.size());
            }

            truck.fuel = unit.real(u"fuel_relative"_s);
            truck.garageCityId = places.garageOf.value(id);
            truck.driverId = places.driverOfTruck.value(id);
            return truck;
        }

        Trailer trailerFrom(const SiiDocument& document, const QString& id, const SiiUnit& unit, const Places& places)
        {
            const SiiUnit* definition = document.unit(unit.text(u"trailer_definition"_s));

            return {.id = id,
                    .model = modelOf(document, unit, u"trailer_owned"_s),
                    .bodyType = definition != nullptr ? definition->text(u"body_type"_s) : QString(),
                    .bodyWear = unit.real(u"trailer_body_wear"_s),
                    .odometer = unit.number(u"odometer"_s),
                    .garageCityId = places.garageOf.value(id)};
        }
    } // namespace

    std::pair<QString, QString> licensePlate(const QString& raw)
    {
        // The plate is markup for the game's font: an offset, a picture (the country's emblem), an offset. Each run of
        // it stands where the plate has a gap.
        static const QRegularExpression Markup(uR"((<[^>]*>)+)"_s);
        const qsizetype bar = raw.lastIndexOf(u'|');
        QString text = (bar < 0 ? raw : raw.left(bar));
        text.replace(Markup, u" "_s);

        return {text.simplified(), bar < 0 ? QString() : raw.sliced(bar + 1)};
    }

    Company companyFrom(const SiiDocument& document)
    {
        Company company;
        const SiiUnit* economy = document.first(u"economy"_s);
        const SiiUnit* player = economy != nullptr ? document.unit(economy->text(u"player"_s)) : nullptr;

        if (player == nullptr)
        {
            return company;
        }

        company.headquartersCityId = player->text(u"hq_city"_s);
        Places places;

        // The economy lists a garage for every city; only those the player owns have a status.
        for (const QString& id : economy->texts(u"garages"_s))
        {
            if (const SiiUnit* garage = document.unit(id); garage != nullptr && garage->number(u"status"_s) != 0)
            {
                company.garages.append(garageFrom(document, id, *garage, places));
            }
        }

        for (const QString& id : player->texts(u"drivers"_s))
        {
            const SiiUnit* unit = document.unit(id);

            if (unit != nullptr && unit->type == u"driver_player"_s)
            {
                company.playerId = id;
                company.playerLog = profitLog(document, unit->text(u"profit_log"_s));
            }
            else if (unit != nullptr)
            {
                company.drivers.append(driverFrom(document, id, *unit, places));
            }
        }

        for (const QString& id : player->texts(u"trucks"_s))
        {
            if (const SiiUnit* unit = document.unit(id))
            {
                company.trucks.append(truckFrom(document, id, *unit, places, u"truck"_s));
            }
        }

        for (const QString& id : player->texts(u"buses"_s))
        {
            if (const SiiUnit* unit = document.unit(id))
            {
                company.buses.append(truckFrom(document, id, *unit, places, u"bus"_s));
            }
        }

        for (const QString& id : player->texts(u"trailers"_s))
        {
            if (const SiiUnit* unit = document.unit(id))
            {
                company.trailers.append(trailerFrom(document, id, *unit, places));
            }
        }

        if (const SiiUnit* bank = document.unit(economy->text(u"bank"_s)))
        {
            company.bank = {.money = bank->number(u"money_account"_s),
                            .loanLimit = bank->number(u"loan_limit"_s),
                            .loans = static_cast<int>(bank->texts(u"loans"_s).size()),
                            .coinsuranceFixed = bank->number(u"coinsurance_fixed"_s),
                            .coinsuranceRatio = bank->real(u"coinsurance_ratio"_s),
                            .overdraft = bank->number(u"overdraft"_s) != 0};
        }

        return company;
    }
} // namespace TruckData::SaveGame
