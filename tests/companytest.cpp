// SPDX-FileCopyrightText: 2026 Pascal Klose
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "truckdata/savegame/career.h"
#include "truckdata/savegame/company.h"
#include "truckdata/savegame/savegamefile.h"

#include <QTest>

using namespace TruckData::SaveGame;

namespace
{
    /// A company as a save game of ETS2 1.5x holds it, cut down: a small garage in Hannover with the player in slot 0,
    /// an empty slot 1 and a hired driver in slot 2; a garage in Berlin the player does not own.
    constexpr const char* CompanySave = R"(SiiNunit
{
economy : _nameless.1 {
 bank: _nameless.2
 player: _nameless.3
 garages: 2
 garages[0]: garage.hannover
 garages[1]: garage.berlin
}

bank : _nameless.2 {
 money_account: 9612549
 coinsurance_fixed: 1000
 coinsurance_ratio: &3dcccccd
 loans: 0
 loan_limit: 500000
 overdraft: false
}

player : _nameless.3 {
 hq_city: hannover
 drivers: 2
 drivers[0]: driver.57
 drivers[1]: driver.5
 trucks: 2
 trucks[0]: _nameless.10
 trucks[1]: _nameless.11
 trailers: 1
 trailers[0]: _nameless.20
 buses: 1
 buses[0]: _nameless.12
}

garage : garage.hannover {
 vehicles: 3
 vehicles[0]: _nameless.10
 vehicles[1]: nil
 vehicles[2]: _nameless.11
 drivers: 3
 drivers[0]: driver.57
 drivers[1]: nil
 drivers[2]: driver.5
 trailers: 1
 trailers[0]: _nameless.20
 status: 2
 profit_log: _nameless.30
 productivity: &3f000000
}

garage : garage.berlin {
 vehicles: 0
 drivers: 0
 trailers: 0
 status: 0
 profit_log: _nameless.33
 productivity: 0
}

driver_player : driver.57 {
 profit_log: _nameless.31
}

driver_ai : driver.5 {
 adr: 7
 long_dist: 3
 heavy: 2
 fragile: 1
 urgent: 4
 mechanical: 5
 hometown: bremen
 current_city: kiel
 driver_job: _nameless.40
 experience_points: 1234
 profit_log: _nameless.32
}

job_info : _nameless.40 {
 cargo: cargo.fish_chips
 source_company: company.volatile.posped.bremen
 target_company: company.volatile.tradeaux.kiel
 planned_distance_km: 150
}

vehicle : _nameless.10 {
 accessories: 1
 accessories[0]: _nameless.50
 license_plate: "H<offset hshift=4 vshift=-5><img src=/material/ui/lp/germany/b.mat color=FFFFFFFF><offset hshift=4 vshift=5>JV 5319|germany"
 odometer: 115515
 engine_wear: &3e800000
 transmission_wear: 0
 cabin_wear: 0
 chassis_wear: 0
 wheels_wear: 2
 wheels_wear[0]: &3f000000
 wheels_wear[1]: 0
 fuel_relative: &3f800000
}

vehicle_accessory : _nameless.50 {
 data_path: "/def/vehicle/truck/scania.r_2016/data.sii"
}

vehicle : _nameless.12 {
 accessories: 1
 accessories[0]: _nameless.52
 license_plate: "H<offset hshift=4><img src=/material/ui/lp/germany/b.mat><offset hshift=4>VF 695|germany"
 odometer: 5495
 engine_wear: &3e800000
}

vehicle_accessory : _nameless.52 {
 data_path: "/def/vehicle/bus/volvo.9700_23/data.sii"
}

vehicle : _nameless.11 {
 accessories: 0
 license_plate: ""
 odometer: 42
}

trailer : _nameless.20 {
 trailer_definition: _nameless.60
 trailer_body_wear: &3e800000
 odometer: 7
 accessories: 1
 accessories[0]: _nameless.51
}

vehicle_accessory : _nameless.51 {
 data_path: "/def/vehicle/trailer_owned/scs.box/data.sii"
}

trailer_def : _nameless.60 {
 body_type: curtainside
}

profit_log : _nameless.30 {
 stats_data: 1
 stats_data[0]: _nameless.72
}

profit_log : _nameless.31 {
 stats_data: 2
 stats_data[0]: _nameless.70
 stats_data[1]: _nameless.71
}

profit_log : _nameless.32 {
 stats_data: 0
}

profit_log_entry : _nameless.70 {
 revenue: 53230
 wage: 0
 maintenance: 0
 fuel: 0
 distance: 1086
 distance_on_job: true
 cargo_count: 1
 cargo: fish_chips
 source_city: bremen
 source_company: posped
 destination_city: kiel
 destination_company: tradeaux
 timestamp_day: 275
}

profit_log_entry : _nameless.71 {
 revenue: 0
 wage: 0
 maintenance: 53457
 fuel: 0
 distance: 0
 distance_on_job: false
 cargo_count: 0
 cargo: ""
 timestamp_day: 275
}

profit_log_entry : _nameless.72 {
 revenue: 80097
 wage: 52813
 maintenance: 6931
 fuel: 4038
 distance: 2847
 distance_on_job: true
 timestamp_day: 276
}
}
)";

    class CompanyTest : public QObject
    {
        Q_OBJECT

    private:
        Company m_company;

    private Q_SLOTS:
        void initTestCase()
        {
            const auto document = parseText(CompanySave);
            QVERIFY2(document.has_value(), qPrintable(document.error_or(SiiError{}).message()));
            m_company = companyFrom(*document);
            // The career counts the garages the player owns, not every city's.
            QCOMPARE(careerFrom(*document).garages, 1);
        }

        void theOwnedGaragesWithTheirSlots()
        {
            QCOMPARE(m_company.headquartersCityId, QStringLiteral("hannover"));
            QCOMPARE(m_company.garages.size(), 1);

            const Garage& garage = m_company.garages.first();
            QCOMPARE(garage.cityId, QStringLiteral("hannover"));
            QCOMPARE(garage.slotCount, 3);
            QCOMPARE(garage.truckIds, QStringList({QStringLiteral("_nameless.10"), QStringLiteral("_nameless.11")}));
            QCOMPARE(garage.driverIds, QStringList({QStringLiteral("driver.57"), QStringLiteral("driver.5")}));
            QCOMPARE(garage.trailers, 1);
            QCOMPARE(garage.productivity, 0.5);
            QCOMPARE(garage.log.size(), 1);
            QCOMPARE(garage.log.first().wage, 52813);
        }

        void aDriverWithSkillsJobAndTruck()
        {
            // The player is not among the hired drivers.
            QCOMPARE(m_company.drivers.size(), 1);
            const Driver& driver = m_company.drivers.first();
            QCOMPARE(driver.id, QStringLiteral("driver.5"));
            QCOMPARE(driver.garageCityId, QStringLiteral("hannover"));
            QCOMPARE(driver.hometownId, QStringLiteral("bremen"));
            QCOMPARE(driver.currentCityId, QStringLiteral("kiel"));
            QCOMPARE(driver.experience, 1234);
            // ADR 7 is three classes, one bit each.
            QCOMPARE(driver.adr, 3);
            QCOMPARE(driver.longDistance, 3);
            QCOMPARE(driver.highValue, 2);
            QCOMPARE(driver.fragile, 1);
            QCOMPARE(driver.justInTime, 4);
            QCOMPARE(driver.ecoDriving, 5);
            // The truck in slot 2, the same as the driver's.
            QCOMPARE(driver.truckId, QStringLiteral("_nameless.11"));
            QVERIFY(driver.job.has_value());
            QCOMPARE(driver.job.value_or(DriverJob{}).cargoId, QStringLiteral("fish_chips"));
            QCOMPARE(driver.job.value_or(DriverJob{}).sourceCompanyId, QStringLiteral("posped"));
            QCOMPARE(driver.job.value_or(DriverJob{}).destinationCityId, QStringLiteral("kiel"));
            QCOMPARE(driver.job.value_or(DriverJob{}).plannedKilometres, 150);
        }

        void theTrucksAndTrailers()
        {
            QCOMPARE(m_company.trucks.size(), 2);
            const Truck& truck = m_company.trucks.first();
            QCOMPARE(truck.model, QStringLiteral("scania.r_2016"));
            QCOMPARE(truck.licensePlate, QStringLiteral("H JV 5319"));
            QCOMPARE(truck.plateCountry, QStringLiteral("germany"));
            QCOMPARE(truck.odometer, 115515);
            QCOMPARE(truck.engineWear, 0.25);
            QCOMPARE(truck.wheelsWear, 0.25);
            QCOMPARE(truck.fuel, 1.0);
            QCOMPARE(truck.garageCityId, QStringLiteral("hannover"));
            // The player's own truck is driven by the player.
            QCOMPARE(truck.driverId, QStringLiteral("driver.57"));
            // A truck without a data accessory has no model; it is still there.
            QCOMPARE(m_company.trucks.last().model, QString());

            QCOMPARE(m_company.trailers.size(), 1);
            const Trailer& trailer = m_company.trailers.first();
            QCOMPARE(trailer.model, QStringLiteral("scs.box"));
            QCOMPARE(trailer.bodyType, QStringLiteral("curtainside"));
            QCOMPARE(trailer.bodyWear, 0.25);
            QCOMPARE(trailer.garageCityId, QStringLiteral("hannover"));
        }

        void theBusesAsTheTrucks()
        {
            // A bus is no truck, and stands in no garage.
            QCOMPARE(m_company.trucks.size(), 2);
            QCOMPARE(m_company.buses.size(), 1);
            const Truck& bus = m_company.buses.first();
            QCOMPARE(bus.model, QStringLiteral("volvo.9700_23"));
            QCOMPARE(bus.licensePlate, QStringLiteral("H VF 695"));
            QCOMPARE(bus.odometer, 5495);
            QCOMPARE(bus.engineWear, 0.25);
            QVERIFY(bus.garageCityId.isEmpty());
            QVERIFY(bus.driverId.isEmpty());
        }

        void thePlayersLogHasTheRepairs()
        {
            QCOMPARE(m_company.playerId, QStringLiteral("driver.57"));
            QCOMPARE(m_company.playerLog.size(), 2);

            const ProfitEntry& delivery = m_company.playerLog.first();
            QCOMPARE(delivery.revenue, 53230);
            QCOMPARE(delivery.kilometres, 1086);
            QVERIFY(delivery.onJob);
            QCOMPARE(delivery.sourceCompanyId, QStringLiteral("posped"));

            // A bill without kilometres: the repair.
            const ProfitEntry& repair = m_company.playerLog.last();
            QCOMPARE(repair.maintenance, 53457);
            QCOMPARE(repair.kilometres, 0);
            QCOMPARE(repair.day, 275);
        }

        void theBank()
        {
            QCOMPARE(m_company.bank.money, 9612549);
            QCOMPARE(m_company.bank.loanLimit, 500000);
            QCOMPARE(m_company.bank.loans, 0);
            QCOMPARE(m_company.bank.coinsuranceFixed, 1000);
            QVERIFY(qAbs(m_company.bank.coinsuranceRatio - 0.1) < 1e-6);
            QVERIFY(!m_company.bank.overdraft);
        }

        void aPlateWithoutMarkupOrCountry()
        {
            QCOMPARE(licensePlate(QStringLiteral("AB 123")), std::make_pair(QStringLiteral("AB 123"), QString()));
            QCOMPARE(licensePlate(QString()), std::make_pair(QString(), QString()));
        }

        void noEconomyNoCompany()
        {
            const auto document = parseText("SiiNunit\n{\n}\n");
            QVERIFY(document.has_value());
            QVERIFY(companyFrom(*document).garages.isEmpty());
        }
    };
} // namespace

QTEST_GUILESS_MAIN(CompanyTest)
#include "companytest.moc"
