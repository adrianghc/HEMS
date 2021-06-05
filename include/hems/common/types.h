/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This header declares the HEMS data types and structures.
 */

#ifndef HEMS_COMMON_TYPES_H
#define HEMS_COMMON_TYPES_H

#include <functional>
#include <map>
#include <set>
#include <string>
#include <cstdint>
#include <mqueue.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/string.hpp>

namespace hems { namespace types {

    using boost::posix_time::ptime;

    /* BEGIN HEMS data types. */

    /**
     * @brief       Identifiers for HEMS data types.
     */
    enum hems_data {
        APPLIANCE,
        TASK,
        AUTO_PROFILE,
        ENERGY_CONSUMPTION,
        ENERGY_PRODUCTION,
        WEATHER,
        SUNLIGHT
    };

    typedef uint64_t id_t;  /** A type for ids. */

    /**
     * @brief       A struct that compiles the HEMS settings.
     */
    typedef struct {
        double longitude    = 0;        /** The user's geographical longitude. */
        double latitude     = 0;        /** The user's geographical latitude. */
        double timezone     = 0;        /** The user's timezone. */
        std::string pv_uri  = "";       /** A string containing an URI to read energy production
                                            data from the PV system. */
        std::map<id_t, unsigned int> station_intervals  = {};   /** Each weather station's interval
                                                                    in minutes at which they release
                                                                    new data. Must be between 1 and
                                                                    60 and a divisor of 60. */
        std::map<id_t, std::string> station_uris        = {};   /** Each weather station's URI. */
        unsigned int interval_energy_production         = 60;   /** The interval in minutes to
                                                                    collect new energy production
                                                                    data. Must be between 1 and 60
                                                                    and a divisor of 60. */
        unsigned int interval_energy_consumption        = 60;   /** The interval in minutes to
                                                                    collect new energy consumption
                                                                    data. Must be between 1 and 60
                                                                    and a divisor of 60. */
        unsigned int interval_automation                = 0;    /** The interval in hours to
                                                                    automate tasks. A value of 0
                                                                    means that no automation takes
                                                                    place and the HEMS relies on
                                                                    user commits entirely. */
    } settings_t;

    /**
     * @brief       Compares two settings_t.
     * @return      True if equal, false otherwise.
     */
    bool operator==(const settings_t& lhs, const settings_t& rhs);

    /**
     * @brief       Compares two settings_t.
     * @return      True if not equal, false otherwise.
     */
    bool operator!=(const settings_t& lhs, const settings_t& rhs);

    /**
     * @brief       Returns a string representation of HEMS settings.
     * @return      The string representation.
     */
    std::string to_string(const settings_t& entry);

    /**
     * @brief       An exemplary settings struct that represents an undefined state.
     * 
     *              { Since (0,0) at timezone 0 is in the sea, we can use the default values for a
     *              settings struct as the undefined state - nobody is going to use this software
     *              there. :) Using NaN values would be more clean, but text archives cannot handle
     *              those by default, so serialization and deserialization of the undefined state
     *              would fail with an exception. }
     */
    extern const settings_t settings_undefined;

    /**
     * @brief       Whether the given settings are in an undefined state.
     * 
     * @param[in]   settings    The settings to check.
     */
    bool is_undefined(const settings_t& entry);


    /**
     * @brief       A struct that defines an appliance profile.
     *              `id` is the unique identifier.
     */
    typedef struct {
        id_t        id = 0;                 /** The appliance's id. Must be greater than 0. If it is
                                                0, it indicates a new entry to be stored. */
        std::string name;                   /** A string containing the appliance's name. */
        std::string uri;                    /** A string containing an URI to automate an appliance
                                                and/or read energy consumption data off it. */
        double      rating = 0;             /** The power rating of the appliance. */
        uint32_t    duty_cycle = 0;         /** The average duty cycle of the appliance in hours. */
        uint8_t     schedules_per_week = 0; /** How often an appliance should be scheduled in the
                                                timeframe of a week. A value of 0 means that the
                                                appliance is not schedulable at all. This describes
                                                appliances that run without input from the HEMS
                                                (either constantly, intermittently or recurringly,
                                                as specified through tasks and automation profiles)
                                                but add to the total energy consumption. */
        std::set<id_t>  tasks;              /** A set of ids for tasks attached to this appliance. */
        std::set<id_t>  auto_profiles;      /** A set of ids for automation profiles attached to
                                                this appliance. */
    } appliance_t;

    /**
     * @brief       Compares two appliance_t. The `id` field is excluded from the comparison.
     * @return      True if equal, false otherwise.
     */
    bool operator==(const appliance_t& lhs, const appliance_t& rhs);

    /**
     * @brief       Compares two appliance_t. The `id` field is excluded from the comparison.
     * @return      True if not equal, false otherwise.
     */
    bool operator!=(const appliance_t& lhs, const appliance_t& rhs);

    /**
     * @brief       Returns a string representation of an appliance profile.
     * @return      The string representation.
     */
    std::string to_string(const appliance_t& entry);


    /**
     * @brief       A struct that defines a task.
     *              `id` is the unique identifier.
     */
    typedef struct {
        id_t        id = 0;                     /** The tasks's id. Must be greater than 0. If it is
                                                    0, it indicates a new entry to be stored. */
        std::string name;                       /** A string containing the tasks's name. */
        ptime       start_time;                 /** The time when the task's execution starts. */
        ptime       end_time;                   /** The time when the task's execution ends. */
        id_t        auto_profile = 0;           /** The id of the automation profile this task
                                                    belongs to, or 0 if none. */
        bool        is_user_declared = false;   /** This states whether the task has been explicitly
                                                    declared or confirmed/committed (upon
                                                    recommendation) by the user. These tasks are
                                                    respected by the HEMS, whereas automatically
                                                    generated/committed tasks can be replaced if
                                                    deemed sensible by the Automation and
                                                    Recommendation Module in the face of updated
                                                    weather forecasts, and thus new predictions.
                                                    Additionally, user-declared tasks can have
                                                    effects on the average runtime of an appliance. */
        std::set<id_t> appliances;              /** A set of ids for appliances attached to this
                                                    task. */
    } task_t;

    /**
     * @brief       Compares two task_t. The `id` field is excluded from the comparison.
     * @return      True if equal, false otherwise.
     */
    bool operator==(const task_t& lhs, const task_t& rhs);

    /**
     * @brief       Compares two task_t. The `id` field is excluded from the comparison.
     * @return      True if not equal, false otherwise.
     */
    bool operator!=(const task_t& lhs, const task_t& rhs);

    /**
     * @brief       Returns a string representation of a task.
     * @return      The string representation.
     */
    std::string to_string(const task_t& entry);


    /**
     * @brief       A struct that defines an automation profile.
     *              `id` is the unique identifier.
     */
    typedef struct {
        id_t            id = 0;     /** The automation profile's id. Must be greater than 0. If it
                                        is 0, it indicates a new entry to be stored. */
        std::string     name;       /** A string containing the automation profile's name. */
        std::string     profile;    /** A string containing the automation profile in iCalendar
                                        syntax. */
        std::set<id_t>  appliances; /** A set of ids for appliances attached to this automation
                                        profile. */
        std::set<id_t>  tasks;      /** A set of task ids spawned from this automation profile. */
    } auto_profile_t;

    /**
     * @brief       Compares two auto_profile_t. The `id` field is excluded from the comparison.
     * @return      True if equal, false otherwise.
     */
    bool operator==(const auto_profile_t& lhs, const auto_profile_t& rhs);

    /**
     * @brief       Compares two auto_profile_t. The `id` field is excluded from the comparison.
     * @return      True if not equal, false otherwise.
     */
    bool operator!=(const auto_profile_t& lhs, const auto_profile_t& rhs);

    /**
     * @brief       Returns a string representation of an automation profile.
     * @return      The string representation.
     */
    std::string to_string(const auto_profile_t& entry);


    /**
     * @brief       A struct that defines energy consumption data of an appliance in a 15-minute
     *              interval.
     *              `(time, appliance_id)` is the unique identifier.
     */
    typedef struct {
        ptime   time;               /** The end of the 15-minute interval. */
        id_t    appliance_id = 0;   /** The id of the appliance that has consumed energy, or 0 if
                                        this is consumption data from appliances for which no
                                        individual measurements exist. */
        double  energy;             /** The amount of energy consumed. */
    } energy_consumption_t;

    /**
     * @brief       Compares two energy_consumption_t.
     * @return      True if equal, false otherwise.
     */
    bool operator==(const energy_consumption_t& lhs, const energy_consumption_t& rhs);

    /**
     * @brief       Compares two energy_consumption_t.
     * @return      True if not equal, false otherwise.
     */
    bool operator!=(const energy_consumption_t& lhs, const energy_consumption_t& rhs);

    /**
     * @brief       Returns a string representation of energy consumption data.
     * @return      The string representation.
     */
    std::string to_string(const energy_consumption_t& entry);


    /**
     * @brief       A struct that defines energy production data in a 15-minute interval.
     *              `time` is the unique identifier.
     */
    typedef struct {
        ptime       time;       /** The end of the 15-minute interval. */
        double      energy = 0; /** The amount of energy produced. */
    } energy_production_t;

    /**
     * @brief       Compares two energy_production_t.
     * @return      True if equal, false otherwise.
     */
    bool operator==(const energy_production_t& lhs, const energy_production_t& rhs);

    /**
     * @brief       Compares two energy_production_t.
     * @return      True if not equal, false otherwise.
     */
    bool operator!=(const energy_production_t& lhs, const energy_production_t& rhs);

    /**
     * @brief       Returns a string representation of energy production data.
     * @return      The string representation.
     */
    std::string to_string(const energy_production_t& entry);


    /**
     * @brief       A struct that defines weather data for a given point in time.
     *              This weather data is used to predict energy production and to train the
     *              corresponding model.
     *              `(time, station)` is the unique identifier.
     */
    typedef struct {
        ptime           time;               /** The point in time. */
        id_t            station;            /** The weather station's id. */
        double          temperature = 0;    /** The temperature. */
        unsigned int    humidity = 0;       /** The humidity in percent. */
        double          pressure = 0;       /** The air pressure. */
        unsigned int    cloud_cover = 0;    /** The cloud cover in percent. */
        double          radiation = 0;      /** The global radiation in kJ per square meter. */
    } weather_t;

    /**
     * @brief       Compares two weather_t.
     * @return      True if equal, false otherwise.
     */
    bool operator==(const weather_t& lhs, const weather_t& rhs);

    /**
     * @brief       Compares two weather_t.
     * @return      True if not equal, false otherwise.
     */
    bool operator!=(const weather_t& lhs, const weather_t& rhs);

    /**
     * @brief       Returns a string representation of weather data.
     * @return      The string representation.
     */
    std::string to_string(const weather_t& entry);


    /**
     * @brief       A struct that defines sunlight angle data for a given time.
     *              This sunlight angle data is used to infer the maximum possible sunlight
     *              intensity for a given geographical location and time of the day / day of the
     *              year.
     *              `time` is the unique identifier.
     */
    typedef struct {
        ptime       time;       /** The time. */
        double      angle = 0;  /** The sunlight angle in degrees at that time. */
    } sunlight_t;

    /**
     * @brief       Compares two sunlight_t.
     * @return      True if equal, false otherwise.
     */
    bool operator==(const sunlight_t& lhs, const sunlight_t& rhs);

    /**
     * @brief       Compares two sunlight_t.
     * @return      True if not equal, false otherwise.
     */
    bool operator!=(const sunlight_t& lhs, const sunlight_t& rhs);

    /**
     * @brief       Returns a string representation of sunlight data.
     * @return      The string representation.
     */
    std::string to_string(const sunlight_t& entry);

    /**
     * @brief       Gets the sunlight angle for a given time and location.
     * 
     * @param[in]   time        The time for which to get the sunlight angle.
     * @param[in]   latitude    The latitude for which to get the sunlight angle.
     * @param[in]   longitude   The longitude for which to get the sunlight angle.
     * 
     * @return      The sunlight angle.
     */
    sunlight_t get_angle(ptime time, double latitude, double longitude);

    /* END HEMS data types. */

}}


namespace boost { namespace serialization {

    using namespace hems::types;

    template<typename Archive>
    void serialize(Archive& ar, settings_t& msg, const unsigned int version) {
        ar & msg.longitude;
        ar & msg.latitude;
        ar & msg.timezone;
        ar & msg.pv_uri;
        ar & msg.station_intervals;
        ar & msg.station_uris;
        ar & msg.interval_energy_production;
        ar & msg.interval_energy_consumption;
        ar & msg.interval_automation;
    }

    template<typename Archive>
    void serialize(Archive& ar, appliance_t& msg, const unsigned int version) {
        ar & msg.id;
        ar & msg.name;
        ar & msg.uri;
        ar & msg.rating;
        ar & msg.duty_cycle;
        ar & msg.schedules_per_week;
        ar & msg.tasks;
        ar & msg.auto_profiles;
    }

    template<typename Archive>
    void serialize(Archive& ar, task_t& msg, const unsigned int version) {
        ar & msg.id;
        ar & msg.name;
        ar & msg.start_time;
        ar & msg.end_time;
        ar & msg.auto_profile;
        ar & msg.is_user_declared;
        ar & msg.appliances;
    }

    template<typename Archive>
    void serialize(Archive& ar, auto_profile_t& msg, const unsigned int version) {
        ar & msg.id;
        ar & msg.name;
        ar & msg.profile;
        ar & msg.appliances;
    }

    template<typename Archive>
    void serialize(Archive& ar, energy_consumption_t& msg, const unsigned int version) {
        ar & msg.time;
        ar & msg.appliance_id;
        ar & msg.energy;
    }

    template<typename Archive>
    void serialize(Archive& ar, energy_production_t& msg, const unsigned int version) {
        ar & BOOST_SERIALIZATION_NVP(msg.time);
        ar & BOOST_SERIALIZATION_NVP(msg.energy);
    }

    template<typename Archive>
    void serialize(Archive& ar, weather_t& msg, const unsigned int version) {
        ar & BOOST_SERIALIZATION_NVP(msg.time);
        ar & BOOST_SERIALIZATION_NVP(msg.station);
        ar & BOOST_SERIALIZATION_NVP(msg.temperature);
        ar & BOOST_SERIALIZATION_NVP(msg.humidity);
        ar & BOOST_SERIALIZATION_NVP(msg.pressure);
        ar & BOOST_SERIALIZATION_NVP(msg.cloud_cover);
        ar & BOOST_SERIALIZATION_NVP(msg.radiation);
    }

    template<typename Archive>
    void serialize(Archive& ar, sunlight_t& msg, const unsigned int version) {
        ar & msg.time;
        ar & msg.angle;
    }

}}

#endif /* HEMS_COMMON_TYPES_H */
