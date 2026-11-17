// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QString>

namespace TruckData::SaveGame
{
    /// One job of a save game's delivery log (a truck) or bus job log (a bus). A save game knows ids only; an
    /// application turns them into names with the game's archives (GameData::readGameNames).
    struct Job
    {
        enum class Vehicle
        {
            Truck,
            Bus,
        };

        Vehicle vehicle = Vehicle::Truck;
        /// In-game minutes since the first in-game day when it was finished: the order of the jobs, and with the
        /// companies and the cargo what tells one apart.
        quint32 gameTime = 0;

        QString sourceCityId;
        QString sourceCompanyId;
        QString destinationCityId;
        QString destinationCompanyId;
        QString cargoId;
        /// "cargo_market", "freight_market", "quick_job", "external_contracts" and so on, as the game names it.
        QString market;
        /// The truck or the bus, as the save game names it ("vehicle.volvo.fh_2024").
        QString truck;

        double drivenKilometres = 0;
        /// The money of the game's currency.
        qint64 revenue = 0;
        int experience = 0;
        /// 0 to 1.
        double cargoDamage = 0;
        /// In-game minutes from taking it to delivering it; known for a bus job only.
        quint32 deliveryMinutes = 0;
        /// Fines paid during the job.
        qint64 fines = 0;

        /// A bus job's passengers over all its stops, the stops, and how content the passengers were (0 to 1).
        int passengers = 0;
        int stops = 0;
        double satisfaction = 0;
    };
} // namespace TruckData::SaveGame
