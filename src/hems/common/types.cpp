/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This contains code for the HEMS data types and structures.
 */

#include <algorithm>
#include <map>
#include <string>
#include <cmath>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "hems/common/types.h"

namespace hems { namespace types {

    bool operator==(const settings_t& lhs, const settings_t& rhs) {
        if (lhs.longitude == rhs.longitude && lhs.latitude == rhs.latitude &&
            lhs.timezone == rhs.timezone && lhs.pv_uri == rhs.pv_uri &&
            lhs.station_intervals == rhs.station_intervals && lhs.station_uris == rhs.station_uris &&
            lhs.interval_energy_production == rhs.interval_energy_production &&
            lhs.interval_energy_consumption == rhs.interval_energy_consumption &&
            lhs.interval_automation == rhs.interval_automation) {
            return true;
        } else {
            return false;
        }
    }

    bool operator!=(const settings_t& lhs, const settings_t& rhs) {
        return !(lhs == rhs);
    }

    std::string to_string(const settings_t& entry) {
        std::string str = "("
            "longitude: " + std::to_string(entry.longitude) + ", " +
            "latitude: " + std::to_string(entry.latitude) + ", " +
            "timezone: " + std::to_string(entry.timezone) + ", " +
            "pv_uri: '" + entry.pv_uri + "', " +
            "station_intervals: (";
        if (entry.station_intervals.empty()) {
            str += "None";
        } else {
            for (const auto& station_interval : entry.station_intervals) {
                str +=
                    std::to_string(station_interval.first) + " -> " +
                    std::to_string(station_interval.second) + ", ";
            }
            str.pop_back();
            str.pop_back();
        }
        str += "), station_uris: (";
        if (entry.station_uris.empty()) {
            str += "None";
        } else {
            for (const auto& station_uris : entry.station_uris) {
                str +=
                    std::to_string(station_uris.first) + " -> " +
                    "'" + station_uris.second + "', ";
            }
            str.pop_back();
            str.pop_back();
        }
        str += "), ";
        str +=
            "interval_energy_production: " + std::to_string(entry.interval_energy_production) + ", " +
            "interval_energy_consumption: " + std::to_string(entry.interval_energy_consumption) + ", " +
            "interval_automation: " + std::to_string(entry.interval_automation);
        str += ")";
        return str;
    }

    const settings_t settings_undefined;

    bool is_undefined(const settings_t& entry) {
        return entry.longitude == 0 && entry.latitude == 0 && entry.timezone == 0;
    }


    bool operator==(const appliance_t& lhs, const appliance_t& rhs) {
        if (lhs.name != rhs.name || lhs.uri != rhs.uri || lhs.rating != rhs.rating ||
            lhs.duty_cycle != rhs.duty_cycle || lhs.schedules_per_week != rhs.schedules_per_week ||
            lhs.tasks.size() != rhs.tasks.size() || lhs.auto_profiles.size() != rhs.auto_profiles.size()) {
            return false;
        } else if (!std::is_permutation(lhs.tasks.begin(), lhs.tasks.end(), rhs.tasks.begin())) {
            return false;
        } else if (
            !std::is_permutation(lhs.auto_profiles.begin(), lhs.auto_profiles.end(), rhs.auto_profiles.begin())
        ) {
            return false;
        } else {
            return true;
        }
    }

    bool operator!=(const appliance_t& lhs, const appliance_t& rhs) {
        return !(lhs == rhs);
    }

    std::string to_string(const appliance_t& entry) {
        std::string str = "("
            "id: " + std::to_string(entry.id) + ", "
            "name: '" + entry.name + "', "
            "uri: '" + entry.uri + "', "
            "rating: " + std::to_string(entry.rating) + ", "
            "duty_cycle: " + std::to_string(entry.duty_cycle) + ", "
            "schedules_per_week: " + std::to_string(entry.schedules_per_week) + ", "
            "tasks: (";
        if (entry.tasks.empty()) {
            str += "None";
        } else {
            for (const auto& task : entry.tasks) {
                str += std::to_string(task) + ", ";
            }
            str.pop_back();
            str.pop_back();
        }
        str += "), auto_profiles: (";
        if (entry.auto_profiles.empty()) {
            str += "None";
        } else {
            for (const auto& auto_profile : entry.auto_profiles) {
                str += std::to_string(auto_profile) + ", ";
            }
            str.pop_back();
            str.pop_back();
        }
        str += "))";
        return str;
    }


    bool operator==(const task_t& lhs, const task_t& rhs) {
        if (lhs.name != rhs.name || lhs.start_time != rhs.start_time || lhs.end_time != rhs.end_time ||
            lhs.auto_profile != rhs.auto_profile || lhs.is_user_declared != rhs.is_user_declared ||
            lhs.appliances.size() != rhs.appliances.size()) {
            return false;
        } else if (!std::is_permutation(lhs.appliances.begin(), lhs.appliances.end(), rhs.appliances.begin())) {
            return false;
        } else {
            return true;
        }
    }

    bool operator!=(const task_t& lhs, const task_t& rhs) {
        return !(lhs == rhs);
    }

    std::string to_string(const task_t& entry) {
        std::string str = "("
            "id: " + std::to_string(entry.id) + ", "
            "name: '" + entry.name + "', "
            "start_time: " + boost::posix_time::to_simple_string(entry.start_time) + ", "
            "end_time: " + boost::posix_time::to_simple_string(entry.end_time) + ", "
            "auto_profile: " + std::to_string(entry.auto_profile) + ", "
            "is_user_declared: " + (entry.is_user_declared ? "true" : "false") + ", "
            "appliances: (";
        if (entry.appliances.empty()) {
            str += "None";
        } else {
            for (const auto& appliance : entry.appliances) {
                str += std::to_string(appliance) + ", ";
            }
            str.pop_back();
            str.pop_back();
        }
        str += "))";
        return str;
    }


    bool operator==(const auto_profile_t& lhs, const auto_profile_t& rhs) {
        if (lhs.name != rhs.name || lhs.profile != rhs.profile ||
            lhs.appliances.size() != rhs.appliances.size() || lhs.tasks.size() != rhs.tasks.size()) {
            return false;
        } else if (!std::is_permutation(lhs.appliances.begin(), lhs.appliances.end(), rhs.appliances.begin())) {
            return false;
        } else if (!std::is_permutation(lhs.tasks.begin(), lhs.tasks.end(), rhs.tasks.begin())) {
            return false;
        } else {
            return true;
        }
    }

    bool operator!=(const auto_profile_t& lhs, const auto_profile_t& rhs) {
        return !(lhs == rhs);
    }

    std::string to_string(const auto_profile_t& entry) {
        std::string str = "("
            "id: " + std::to_string(entry.id) + ", "
            "name: '" + entry.name + "', "
            "profile: '" + entry.profile + "', "
            "appliances: (";
        if (entry.appliances.empty()) {
            str += "None";
        } else {
            for (const auto& appliance : entry.appliances) {
                str += std::to_string(appliance) + ", ";
            }
            str.pop_back();
            str.pop_back();
        }
        str += "), tasks: (";
        if (entry.tasks.empty()) {
            str += "None";
        } else {
            for (const auto& task : entry.tasks) {
                str += std::to_string(task) + ", ";
            }
            str.pop_back();
            str.pop_back();
        }
        str += "))";
        return str;
    }


    bool operator==(const energy_consumption_t& lhs, const energy_consumption_t& rhs) {
        if (lhs.time == rhs.time && lhs.appliance_id == rhs.appliance_id && lhs.energy == rhs.energy) {
            return true;
        } else {
            return false;
        }
    }

    bool operator!=(const energy_consumption_t& lhs, const energy_consumption_t& rhs) {
        return !(lhs == rhs);
    }

    std::string to_string(const energy_consumption_t& entry) {
        std::string str = "("
            "time: " + boost::posix_time::to_simple_string(entry.time) + ", "
            "appliance_id: " + std::to_string(entry.appliance_id) + ", "
            "energy: " + std::to_string(entry.energy) +
        ")";
        return str;
    }


    bool operator==(const energy_production_t& lhs, const energy_production_t& rhs) {
        if (lhs.time == rhs.time && lhs.energy == rhs.energy) {
            return true;
        } else {
            return false;
        }
    }

    bool operator!=(const energy_production_t& lhs, const energy_production_t& rhs) {
        return !(lhs == rhs);
    }

    std::string to_string(const energy_production_t& entry) {
        std::string str = "("
            "time: " + boost::posix_time::to_simple_string(entry.time) + ", "
            "energy: " + std::to_string(entry.energy) +
        ")";
        return str;
    }


    bool operator==(const weather_t& lhs, const weather_t& rhs) {
        if (lhs.time == rhs.time && lhs.station == rhs.station && lhs.temperature == rhs.temperature &&
            lhs.humidity == rhs.humidity && lhs.pressure == rhs.pressure &&
            lhs.cloud_cover == rhs.cloud_cover && lhs.radiation == rhs.radiation) {
            return true;
        } else {
            return false;
        }
    }

    bool operator!=(const weather_t& lhs, const weather_t& rhs) {
        return !(lhs == rhs);
    }

    std::string to_string(const weather_t& entry) {
        std::string str = "("
            "time: " + boost::posix_time::to_simple_string(entry.time) + ", "
            "temperature: " + std::to_string(entry.temperature) + ", "
            "humidity: " + std::to_string(entry.humidity) + ", "
            "pressure: " + std::to_string(entry.pressure) + ", "
            "cloud cover: " + std::to_string(entry.cloud_cover) + ", "
            "radiation: " + std::to_string(entry.radiation) +
        ")";
        return str;
    }


    bool operator==(const sunlight_t& lhs, const sunlight_t& rhs) {
        if (lhs.time == rhs.time && lhs.angle == rhs.angle) {
            return true;
        } else {
            return false;
        }
    }

    bool operator!=(const sunlight_t& lhs, const sunlight_t& rhs) {
        return !(lhs == rhs);
    }

    std::string to_string(const sunlight_t& entry) {
        std::string str = "("
            "time: " + boost::posix_time::to_simple_string(entry.time) + ", "
            "angle: " + std::to_string(entry.angle) +
        ")";
        return str;
    }

    sunlight_t get_angle(ptime time, double latitude, double longitude) {
        auto get_radians = [](double degrees) {
            return (degrees * M_PI) / 180;
        };

        auto get_degrees = [](double radians) {
            return (radians * 180) / M_PI;
        };

        auto get_julian_day = [](ptime time) {
            auto y = 0;
            auto m = 0;
            if (time.date().month() > 2) {
                y = time.date().year();
                m = time.date().month();
            } else {
                y = time.date().year() - 1;
                m = time.date().month() + 12;
            }
            auto d =
                time.date().day() + time.time_of_day().hours() / 24.0 +
                time.time_of_day().minutes() / 1440.0 + time.time_of_day().seconds() / 86400.0;
            auto b = 2 - std::floor(y / 100) + std::floor(y / 400);

            auto jd = std::floor(365.25 * (y + 4716)) + std::floor(30.6001 * (m + 1)) + d + b - 1524.5;
            return jd;
        };


        /* Source: https://de.wikipedia.org/wiki/Sonnenstand#Genauere_Ermittlung_des_Sonnenstandes_f%C3%BCr_einen_Zeitpunkt */

        /* 1. Eclipctical coordinates of the sun */

        /* Julian day */
        auto jd = get_julian_day(time);

        auto n = jd - 2451545;

        /* Median ecliptic longitude of the sun */
        auto l = std::fmod(280.46 + 0.9856474 * n, 360);

        /* Median anomaly */
        auto g = std::fmod(357.528 + 0.9856003 * n, 360);

        /* Ecliptic longitude of the sun */
        auto lbd = l + 1.915 * std::sin(get_radians(g)) + 0.01997 * std::sin(get_radians(2*g));


        /* 2. Equatorial coordinates of the sun */

        /* Ecliptic */
        auto eps = 23.439 - 0.0000004 * n;

        /* Right ascension */
        auto alpha = get_degrees(std::atan(std::cos(get_radians(eps)) * std::tan(get_radians(lbd))));
        if (std::cos(get_radians(lbd)) < 0) {
            alpha += 180;
        }

        /* Declination */
        auto delta = get_degrees(std::asin(std::sin(get_radians(eps)) * std::sin(get_radians(lbd))));


        /* 3. Horizontal coordinates of the sun */

        ptime time_(time.date(), boost::posix_time::time_duration(0, 0, 0));
        auto t0 = (get_julian_day(time_) - 2451545) / 36525;

        /* Median sidereal time */
        auto theta_hg = std::fmod(
            6.697376 + 2400.05134 * t0 + 1.002738 * (time.time_of_day().hours() + time.time_of_day().minutes() / 60.0),
            24
        );

        auto theta_g = theta_hg * 15;
        auto theta = theta_g + longitude;

        /* Hour angle of the sun */
        auto tau = theta - alpha;

        /* Elevation angle */
        auto h = std::cos(get_radians(delta)) * std::cos(get_radians(tau)) * std::cos(get_radians(latitude));
        h += std::sin(get_radians(delta)) * std::sin(get_radians(latitude));
        h = get_degrees(std::asin(h));

        sunlight_t angle = {
            time    : time,
            angle   : h > 0 ? h : 0
        };
        return angle;
    }

}}
