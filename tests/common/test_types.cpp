/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * These are unit tests for the HEMS data types.
 */

#include <boost/date_time/posix_time/posix_time.hpp>
#include "../test.hpp"
#include "hems/common/types.h"

namespace hems { namespace types {

    using boost::posix_time::ptime;
    using boost::posix_time::time_from_string;

    template<typename T> bool test_types(std::vector<T>& items) {
        for (const auto& item : items) {
            if (item != item || !(item == item)) {
                std::cout << "Items were wrongly determined as not equal.\n";
                return false;
            }
        }

        for (unsigned int i = 0; i < items.size(); ++i) {
            for (unsigned int j = i + 1; j < items.size(); ++j) {
                if (items.at(i) == items.at(j) || !(items.at(i) != items.at(j))) {
                    std::cout << "Items were wrongly determined as equal.\n";
                    return false;
                }
            }
        }

        return true;
    }

    bool test_types_settings_t() {
        settings_t settings1 = {
            longitude                   : 1,
            latitude                    : 1,
            timezone                    : 1,
            pv_uri                      : "",
            station_intervals           : { {1, 15}, {2, 30} },
            station_uris                : { {1, ""}, {2, ""} },
            interval_energy_production  : 10,
            interval_energy_consumption : 20,
            interval_automation         : 36
        };

        settings_t settings2 = settings1;
        settings2.longitude = 2;

        settings_t settings3 = settings1;
        settings3.latitude = 2;

        settings_t settings4 = settings1;
        settings4.timezone = 2;

        settings_t settings5 = settings1;
        settings5.pv_uri = "Tomato";

        settings_t settings6 = settings1;
        settings6.station_intervals = { {1, 15}, {2, 0} };

        settings_t settings7 = settings1;
        settings7.station_uris = { {1, "Lorem"}, {2, "ipsum"} };

        settings_t settings8 = settings1;
        settings8.interval_energy_production = 15;

        settings_t settings9 = settings1;
        settings9.interval_energy_consumption = 30;

        settings_t settings10 = settings1;
        settings10.interval_automation = 96;

        std::vector<settings_t> settings = {
            settings1, settings2, settings3, settings4, settings5, settings6, settings7, settings8,
            settings9, settings10
        };
        return test_types(settings);
    }

    bool test_types_appliance_t() {
        appliance_t appliance0 = {
            id                  : 1,
            name                : "appliance",
            uri                 : "",
            rating              : 5.5,
            duty_cycle          : 4,
            schedules_per_week  : 1,
            tasks               : std::set<id_t>({1, 2}),
            auto_profiles       : std::set<id_t>({3, 4})
        };

        appliance_t appliance1 = appliance0;
        appliance1.id = 2;
        appliance1.tasks = std::set<id_t>({2, 1});
        appliance1.auto_profiles = std::set<id_t>({4, 3});

        appliance_t appliance2 = appliance0;
        appliance2.name = "Lorem ipsum";

        appliance_t appliance3 = appliance0;
        appliance3.uri = "Potato";

        appliance_t appliance4 = appliance0;
        appliance4.rating = 10;

        appliance_t appliance5 = appliance0;
        appliance5.duty_cycle = 7.3;

        appliance_t appliance6 = appliance0;
        appliance6.schedules_per_week = 0;

        appliance_t appliance7 = appliance0;
        appliance7.tasks = std::set<id_t>({5, 10});

        appliance_t appliance8 = appliance0;
        appliance8.auto_profiles = std::set<id_t>({5, 10});

        if (appliance0 != appliance1 || !(appliance0 == appliance1)) {
            std::cout << "Items were wrongly determined as not equal.\n";
            return false;
        }

        std::vector<appliance_t> appliances = {
            appliance1, appliance2, appliance3, appliance4, appliance5, appliance6, appliance7,
            appliance8
        };
        return test_types(appliances);
    }

    bool test_types_task_t() {
        task_t task0 = {
            id                  : 1,
            name                : "task",
            start_time          : time_from_string("2020-02-20 02:02:02.000"),
            end_time            : time_from_string("2020-02-20 20:20:20.000"),
            auto_profile        : 0,
            is_user_declared    : true,
            appliances          : std::set<id_t>({5, 7})
        };

        task_t task1 = task0;
        task1.id = 2;
        task1.appliances = std::set<id_t>({7, 5});

        task_t task2 = task0;
        task2.name = "Lorem ipsum";

        task_t task3 = task0;
        task3.start_time = time_from_string("2020-02-20 20:20:20.000");

        task_t task4 = task0;
        task4.end_time = time_from_string("2020-02-20 22:22:22.222");

        task_t task5 = task0;
        task5.auto_profile = 1;

        task_t task6 = task0;
        task6.is_user_declared = false;

        task_t task7 = task0;
        task7.appliances = std::set<id_t>({5, 10});

        if (task0 != task1 || !(task0 == task1)) {
            std::cout << "Items were wrongly determined as not equal.\n";
            return false;
        }

        std::vector<task_t> tasks = {
            task1, task2, task3, task4, task5, task6, task7
        };
        return test_types(tasks);
    }

    bool test_types_auto_profile_t() {
        auto_profile_t auto_profile_0 = {
            id          : 1,
            name        : "auto_profile",
            profile     : "I am Iron Man.",
            appliances  : std::set<id_t>({9, 4}),
            tasks       : std::set<id_t>({1, 3})
        };

        auto_profile_t auto_profile_1 = auto_profile_0;
        auto_profile_1.id = 2;
        auto_profile_1.appliances = std::set<id_t>({4, 9});
        auto_profile_1.tasks = std::set<id_t>({3, 1});

        auto_profile_t auto_profile_2 = auto_profile_0;
        auto_profile_2.name = "Lorem ipsum";

        auto_profile_t auto_profile_3 = auto_profile_0;
        auto_profile_3.profile = "Tomato salad";

        auto_profile_t auto_profile_4 = auto_profile_0;
        auto_profile_4.appliances = std::set<id_t>({5, 10});

        auto_profile_t auto_profile_5 = auto_profile_0;
        auto_profile_5.tasks = std::set<id_t>({2, 5});

        if (auto_profile_0 != auto_profile_1 || !(auto_profile_0 == auto_profile_1)) {
            std::cout << "Items were wrongly determined as not equal.\n";
            return false;
        }

        std::vector<auto_profile_t> auto_profiles = {
            auto_profile_1, auto_profile_2, auto_profile_3, auto_profile_4, auto_profile_5
        };
        return test_types(auto_profiles);
    }

    bool test_types_energy_consumption_t() {
        energy_consumption_t energy_consumption_1 = {
            time            : time_from_string("2020-02-20 20:00:00.000"),
            appliance_id    : 0,
            energy          : 6.8
        };

        energy_consumption_t energy_consumption_2 = energy_consumption_1;
        energy_consumption_2.time = time_from_string("2020-02-22 22:22:22.000");

        energy_consumption_t energy_consumption_3 = energy_consumption_1;
        energy_consumption_3.appliance_id = 7;

        energy_consumption_t energy_consumption_4 = energy_consumption_1;
        energy_consumption_4.energy = 15.73;

        std::vector<energy_consumption_t> energy_consumptions = {
            energy_consumption_1, energy_consumption_2, energy_consumption_3, energy_consumption_4
        };
        return test_types(energy_consumptions);
    }

    bool test_types_energy_production_t() {
        energy_production_t energy_production_1 = {
            time    : time_from_string("2020-02-20 20:00:00.000"),
            energy  : 6.8
        };

        energy_production_t energy_production_2 = energy_production_1;
        energy_production_2.time = time_from_string("2020-02-22 22:22:22.000");

        energy_production_t energy_production_3 = energy_production_1;
        energy_production_3.energy = 15.73;

        std::vector<energy_production_t> energy_productions = {
            energy_production_1, energy_production_2, energy_production_3
        };
        return test_types(energy_productions);
    }

    bool test_types_weather_t() {
        weather_t weather1 = {
            time        : time_from_string("2020-02-20 20:30:00.000"),
            station     : 1,
            temperature : 17.65,
            humidity    : 90,
            pressure    : 976,
            cloud_cover : 56,
            radiation   : 1000,
        };

        weather_t weather2 = weather1;
        weather2.time = time_from_string("2020-02-22 22:22:22.000");

        weather_t weather3 = weather1;
        weather3.station = 2;

        weather_t weather4 = weather1;
        weather4.temperature = -5.2;

        weather_t weather5 = weather1;
        weather5.humidity = 45;

        weather_t weather6 = weather1;
        weather6.pressure = 899;

        weather_t weather7 = weather1;
        weather7.cloud_cover = 12;

        weather_t weather8 = weather1;
        weather8.radiation = 1500;

        std::vector<weather_t> weathers = {
            weather1, weather2, weather3, weather4, weather5, weather6, weather7, weather8
        };
        return test_types(weathers);
    }

    bool test_types_sunlight_t() {
        sunlight_t sunlight1 = {
            time    : time_from_string("2020-02-20 20:30:00.000"),
            angle   : 0
        };

        sunlight_t sunlight2 = sunlight1;
        sunlight2.time = time_from_string("2020-02-22 22:22:22.000");

        sunlight_t sunlight3 = sunlight1;
        sunlight3.angle = 6.967;

        std::vector<sunlight_t> sunlights = {
            sunlight1, sunlight2, sunlight3
        };

        if (!test_types(sunlights)) {
            return false;
        }

        /* Test the get_angle() function. */

        ptime time4 = time_from_string("2020-06-21 11:00:00.000");
        double latitude4 = 52.4652025;
        double longitude4 = 13.3412466;
        double angle4_lb = 60.923997;
        double angle4_ub = 60.923998;

        sunlight_t sunlight4 = get_angle(time4, latitude4, longitude4);
        if (sunlight4.time != time4 || sunlight4.angle < angle4_lb || sunlight4.angle > angle4_ub) {
            std::cout <<
                "Wrong angle was returned, or wrong time was put into sunlight_t object:" +
                to_string(sunlight4) + "\n";
            return false;
        }

        ptime time5 = time_from_string("2006-08-06 06:00:00.000");
        double latitude5 = 48.1;
        double longitude5 = 11.6;
        double angle5_lb = 19.061638;
        double angle5_ub = 19.061639;

        sunlight_t sunlight5 = get_angle(time5, latitude5, longitude5);
        if (sunlight5.time != time5 || sunlight5.angle < angle5_lb || sunlight5.angle > angle5_ub) {
            std::cout <<
                "Wrong angle was returned, or wrong time was put into sunlight_t object:" +
                to_string(sunlight5) + "\n";
            return false;
        }

        return true;
    }

}}


int main() {
    return run_tests({
        { "01 Common: Types test (settings_t)", &hems::types::test_types_settings_t },
        { "02 Common: Types test (appliance_t)", &hems::types::test_types_appliance_t },
        { "03 Common: Types test (task_t)", &hems::types::test_types_task_t },
        { "04 Common: Types test (auto_profile_t)", &hems::types::test_types_auto_profile_t },
        { "05 Common: Types test (energy_consumption_t)", &hems::types::test_types_energy_consumption_t },
        { "06 Common: Types test (energy_production_t)", &hems::types::test_types_energy_production_t },
        { "07 Common: Types test (weather_t)", &hems::types::test_types_weather_t },
        { "08 Common: Types test (sunlight_t)", &hems::types::test_types_sunlight_t }
    });
}
