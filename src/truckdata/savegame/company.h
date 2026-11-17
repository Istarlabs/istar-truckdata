// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "truckdata/savegame/siiunit.h"

#include <QList>
#include <QString>

#include <optional>

namespace TruckData::SaveGame
{
    /// One entry of a profit log: a delivery, a drive without a job, or a bill on its own - fuel, or a repair. The
    /// game keeps only the last few in-game days of each log.
    struct ProfitEntry
    {
        /// The in-game day it was booked on (day 0 is the first).
        qint64 day = 0;
        qint64 revenue = 0;
        /// What the driver was paid; 0 for the player's own entries.
        qint64 wage = 0;
        /// The upkeep of the truck on a delivery; on an entry without distance of the player's own log, a repair.
        qint64 maintenance = 0;
        qint64 fuel = 0;
        qint64 kilometres = 0;
        /// Whether the kilometres were driven on a job.
        bool onJob = false;
        int cargoCount = 0;
        /// The game's ids; empty for an entry that is no delivery.
        QString cargoId;
        QString sourceCityId;
        QString sourceCompanyId;
        QString destinationCityId;
        QString destinationCompanyId;
    };

    /// The job a hired driver is on.
    struct DriverJob
    {
        QString cargoId;
        QString sourceCityId;
        QString sourceCompanyId;
        QString destinationCityId;
        QString destinationCompanyId;
        qint64 plannedKilometres = 0;
    };

    /// A hired driver. The save game knows the driver by an id ("driver.136"); the name is in the game's archives.
    struct Driver
    {
        QString id;
        /// The garage the driver works from, by its city.
        QString garageCityId;
        QString hometownId;
        /// Where the driver is now.
        QString currentCityId;
        qint64 experience = 0;
        /// The skills, 0 to 6 each, in the game's order. ADR is the number of dangerous goods classes (the save game
        /// keeps one bit each); the save game calls high-value cargo "heavy", just-in-time delivery "urgent" and
        /// ecodriving "mechanical".
        int adr = 0;
        int longDistance = 0;
        int highValue = 0;
        int fragile = 0;
        int justInTime = 0;
        int ecoDriving = 0;
        std::optional<DriverJob> job;
        /// The truck in the driver's slot of the garage; empty when the slot has none.
        QString truckId;
        QList<ProfitEntry> log;
    };

    /// A truck of the company.
    struct Truck
    {
        QString id;
        /// The truck's definition: "scania.r_2016", "volvo.fh_2024" - the brand before the dot.
        QString model;
        /// The plate without its markup ("H JV 5319"), and the country it is from ("germany").
        QString licensePlate;
        QString plateCountry;
        qint64 odometer = 0;
        /// Wear of each part, 0 (new) to 1; the wheels averaged.
        double engineWear = 0;
        double transmissionWear = 0;
        double cabinWear = 0;
        double chassisWear = 0;
        double wheelsWear = 0;
        /// The tank, 0 to 1.
        double fuel = 0;
        /// The garage it stands in, by its city, and the driver in its slot (the player's own id for the player's
        /// truck).
        QString garageCityId;
        QString driverId;
    };

    /// A trailer the company owns.
    struct Trailer
    {
        QString id;
        /// The trailer's definition: "scs.box", "schwmuller.lowloader".
        QString model;
        /// "curtainside", "refrigerated", … as the game names the body.
        QString bodyType;
        double bodyWear = 0;
        qint64 odometer = 0;
        QString garageCityId;
    };

    /// A garage the company owns.
    struct Garage
    {
        QString cityId;
        /// How many trucks fit: 3 in a small garage, 5 in a large one. (Not "slots": that is a Qt keyword.)
        int slotCount = 0;
        /// The ids of the trucks and drivers in it, slot by slot; empty slots left out.
        QStringList truckIds;
        QStringList driverIds;
        int trailers = 0;
        /// How busy it is, 0 to 1, as the game shows it.
        double productivity = 0;
        QList<ProfitEntry> log;
    };

    struct Bank
    {
        qint64 money = 0;
        qint64 loanLimit = 0;
        /// How many loans run.
        int loans = 0;
        /// What the player pays of an accident's damage: a fixed sum, and a share of the rest.
        qint64 coinsuranceFixed = 0;
        double coinsuranceRatio = 0;
        bool overdraft = false;
    };

    /// The player's company, as far as the save game knows it.
    struct Company
    {
        QString headquartersCityId;
        QList<Garage> garages;
        /// The hired drivers, not the player.
        QList<Driver> drivers;
        QList<Truck> trucks;
        /// The buses, kept like the trucks: the model is a bus's definition ("volvo.9700_23"), and a bus stands in
        /// no garage and has no driver of its own.
        QList<Truck> buses;
        QList<Trailer> trailers;
        Bank bank;
        /// The player's own id among the drivers of the garages, and the player's profit log: deliveries, and the
        /// bills paid on the road - an entry with maintenance and no kilometres is a repair.
        QString playerId;
        QList<ProfitEntry> playerLog;
    };

    /// The company of the save game; an empty one when it has no economy (not a game save).
    Company companyFrom(const SiiDocument& document);

    /// "H<offset …><img …><offset …>JV 5319|germany": the plate as text ("H JV 5319") and its country ("germany").
    std::pair<QString, QString> licensePlate(const QString& raw);
} // namespace TruckData::SaveGame
