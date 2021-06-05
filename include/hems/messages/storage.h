/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This header defines the interface for the Data Storage Module.
 * The interface consists of message types for messages that are put in the module's message queue.
 */

#ifndef HEMS_MESSAGES_STORAGE_H
#define HEMS_MESSAGES_STORAGE_H

#include <map>
#include <set>
#include <vector>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include "hems/common/modules.h"
#include "hems/common/types.h"


namespace hems { namespace messages { namespace storage {

    using namespace hems::types;

    using boost::posix_time::ptime;

    /**
     * @brief       Identifiers for message types of the Data Storage Module.
     */
    enum msg_type {
        MSG_SET_APPLIANCE,
        MSG_SET_TASK,
        MSG_SET_AUTO_PROFILE,
        MSG_SET_ENERGY_CONSUMPTION,
        MSG_SET_ENERGY_PRODUCTION,
        MSG_SET_WEATHER,
        MSG_DEL_APPLIANCE,
        MSG_DEL_TASK,
        MSG_DEL_AUTO_PROFILE,
        MSG_DEL_ENERGY_CONSUMPTION,
        MSG_DEL_ENERGY_PRODUCTION,
        MSG_DEL_WEATHER,
        MSG_GET_SETTINGS,
        MSG_GET_APPLIANCES,
        MSG_GET_APPLIANCES_ALL,
        MSG_GET_TASKS_BY_ID,
        MSG_GET_TASKS_BY_TIME,
        MSG_GET_TASKS_ALL,
        MSG_GET_AUTO_PROFILES,
        MSG_GET_AUTO_PROFILES_ALL,
        MSG_GET_ENERGY_PRODUCTION,
        MSG_GET_ENERGY_PRODUCTION_ALL,
        MSG_GET_ENERGY_CONSUMPTION,
        MSG_GET_ENERGY_CONSUMPTION_ALL,
        MSG_GET_WEATHER
    };

    /**
     * @brief       Response codes for the messages of the Data Storage Module.
     */
    enum response_code {
        SUCCESS                         = 0x00,
        MSG_SET_CONSTRAINT_VIOLATION    = 0x10,
        MSG_SET_REPLACE_NON_EXISTING    = 0x11,
        MSG_SET_SQL_ERROR               = 0x12,
        MSG_SET_FATAL_DUPLICATE         = 0x13,
        MSG_DEL_CONSTRAINT_VIOLATION    = 0x20,
        MSG_DEL_DELETE_NON_EXISTING     = 0x21,
        MSG_DEL_SQL_ERROR               = 0x22,
        MSG_GET_NONE_AVAILABLE          = 0x30,
        MSG_GET_SQL_ERROR               = 0x31
    };

    /**
     * @brief       A tribool enum used by several get messages.
     */
    enum tribool {
        TRUE, FALSE, INDETERMINATE
    };


    /* BEGIN SET messages. */

    /**
     * @brief       Use this message to insert a new entry of an appliance profile or replace an
     *              existing one.
     * 
     *              This message can return the following response codes:
     * 
     *              SUCCESS                         if success.
     *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a field
     *                                              that violated a constraint.
     *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to be
     *                                              replaced.
     *              MSG_SET_SQL_ERROR               if an SQL query returns an error.
     *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type entry's
     *                                              primary key already exist, which should
     *                                              absolutely not happen.
     */
    typedef struct {
        appliance_t appliance;  /** An appliance profile to be inserted or replaced. If an entry
                                    with the provided unique identifiers already exists, the
                                    existing entry is replaced with the values provided here. */
    } msg_set_appliance_request;

    /**
     * @brief       Use this message to insert a new entry of a task or replace an existing one.
     * 
     *              This message can return the following response codes:
     * 
     *              SUCCESS                         if success.
     *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a field
     *                                              that violated a constraint.
     *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to be
     *                                              replaced.
     *              MSG_SET_SQL_ERROR               if an SQL query returns an error.
     *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type entry's
     *                                              primary key already exist, which should
     *                                              absolutely not happen.
     */
    typedef struct {
        task_t task;    /** A task to be inserted or replaced. If an entry with the provided unique
                            identifiers already exists, the existing entry is replaced with the
                            values provided here. */
    } msg_set_task_request;

    /**
     * @brief       Use this message to insert a new entry of an automation profile or replace an
     *              existing one.
     * 
     *              This message can return the following response codes:
     * 
     *              SUCCESS                         if success.
     *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a field
     *                                              that violated a constraint.
     *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to be
     *                                              replaced.
     *              MSG_SET_SQL_ERROR               if an SQL query returns an error.
     *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type entry's
     *                                              primary key already exist, which should
     *                                              absolutely not happen.
     */
    typedef struct {
        auto_profile_t auto_profile;    /** An automation profile to be inserted or replaced. If an
                                            entry with the provided unique identifiers already
                                            exists, the existing entry is replaced with the values
                                            provided here. */
    } msg_set_auto_profile_request;

    /**
     * @brief       Use this message to insert a new entry of energy consumption or replace an
     *              existing one.
     * 
     *              This message can return the following response codes:
     * 
     *              SUCCESS                         if success.
     *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a field
     *                                              that violated a constraint.
     *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to be
     *                                              replaced.
     *              MSG_SET_SQL_ERROR               if an SQL query returns an error.
     *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type entry's
     *                                              primary key already exist, which should
     *                                              absolutely not happen.
     */
    typedef struct {
        energy_consumption_t energy_consumption;    /** Energy consumption to be inserted or
                                                        replaced. If an entry with the provided
                                                        unique identifiers already exists, the
                                                        existing entry is replaced with the values
                                                        provided here. */
    } msg_set_energy_consumption_request;

    /**
     * @brief       Use this message to insert a new entry of energy production or replace an
     *              existing one.
     * 
     *              This message can return the following response codes:
     * 
     *              SUCCESS                         if success.
     *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a field
     *                                              that violated a constraint.
     *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to be
     *                                              replaced.
     *              MSG_SET_SQL_ERROR               if an SQL query returns an error.
     *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type entry's
     *                                              primary key already exist, which should
     *                                              absolutely not happen.
     */
    typedef struct {
        energy_production_t energy_production;  /** Energy consumption to be inserted or replaced.
                                                    If an entry with the provided unique identifiers
                                                    already exists, the existing entry is replaced
                                                    with the values provided here. */
    } msg_set_energy_production_request;

    /**
     * @brief       Use this message to insert a new entry of weather data or replace an existing
     *              one.
     * 
     *              This message can return the following response codes:
     * 
     *              SUCCESS                         if success.
     *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a field
     *                                              that violated a constraint.
     *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to be
     *                                              replaced.
     *              MSG_SET_SQL_ERROR               if an SQL query returns an error.
     *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type entry's
     *                                              primary key already exist, which should
     *                                              absolutely not happen.
     */
    typedef struct {
        weather_t weather;  /** Weather data to be inserted or replaced. If an entry with the
                                provided unique identifiers already exists, the existing entry is
                                replaced with the values provided here. */
    } msg_set_weather_request;

    /**
     * @brief       This message delivers the response to `MSG_SET_APPLIANCE`, `MSG_SET_TASK` and
     *              `MSG_SET_AUTO_PROFILE` requests.
     */
    typedef struct {
        id_t id;    /** The id of the newly inserted entry. */
    } msg_set_response;

    /* END SET messages. */


    /* BEGIN DEL messages. */

    /**
     * @brief       Use this message to delete an existing entry of an appliance profile.
     *              This message can return the following response codes:
     * 
     *              SUCCESS                         if success.
     *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
     *                                              field that violated a constraint.
     *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
     *                                              be replaced.
     *              MSG_DEL_SQL_ERROR               if an SQL query returns an error.
     */
    typedef struct {
        id_t id;    /** The id of the appliance profile to be deleted. */
    } msg_del_appliance_request;

    /**
     * @brief       Use this message to delete an existing entry of a task.
     *              This message can return the following response codes:
     * 
     *              SUCCESS                         if success.
     *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
     *                                              field that violated a constraint.
     *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
     *                                              be replaced.
     *              MSG_DEL_SQL_ERROR               if an SQL query returns an error.
     */
    typedef struct {
        id_t id;    /** The id of the task to be deleted. */
    } msg_del_task_request;

    /**
     * @brief       Use this message to delete an existing entry of an automation profile.
     *              This message can return the following response codes:
     * 
     *              SUCCESS                         if success.
     *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
     *                                              field that violated a constraint.
     *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
     *                                              be replaced.
     *              MSG_DEL_SQL_ERROR               if an SQL query returns an error.
     */
    typedef struct {
        id_t id;    /** The id of the automation profile to be deleted. */
    } msg_del_auto_profile_request;

    /**
     * @brief       Use this message to delete an existing entry of energy consumption.
     *              This message can return the following response codes:
     * 
     *              SUCCESS                         if success.
     *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
     *                                              field that violated a constraint.
     *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
     *                                              be replaced.
     *              MSG_DEL_SQL_ERROR               if an SQL query returns an error.
     */
    typedef struct {
        ptime   time;           /** The end of the 15-minute interval. */
        id_t    appliance_id;   /** The id of the appliance that has consumed energy, or 0 if this
                                    is consumption data from appliances for which no individual
                                    measurements exist. */
    } msg_del_energy_consumption_request;

    /**
     * @brief       Use this message to delete an existing entry of energy production.
     *              This message can return the following response codes:
     * 
     *              SUCCESS                         if success.
     *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
     *                                              field that violated a constraint.
     *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
     *                                              be replaced.
     *              MSG_DEL_SQL_ERROR               if an SQL query returns an error.
     */
    typedef struct {
        ptime time; /** The end of the 15-minute interval. */
    } msg_del_energy_production_request;

    /**
     * @brief       Use this message to delete an existing entry of weather data.
     *              This message can return the following response codes:
     * 
     *              SUCCESS                         if success.
     *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
     *                                              field that violated a constraint.
     *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
     *                                              be replaced.
     *              MSG_DEL_SQL_ERROR               if an SQL query returns an error.
     */
    typedef struct {
        ptime   time;       /** The time for which to delete the weather data.. */
        id_t    station;    /** The station for which to delete the weather data. */
    } msg_del_weather_request;

    /* END DEL messages. */


    /* BEGIN Messages to get appliances. */

    /**
     * @brief       Use this message to get one or several appliance profiles by id.
     */
    typedef struct {
        std::set<id_t> ids; /** A set of ids of the requested appliances. */
    } msg_get_appliances_request;

    /**
     * @brief       This message delivers the response to a `MSG_GET_APPLIANCES` request.
     */
    typedef struct {
        std::map<id_t, appliance_t> appliances; /** A map from appliance ids to appliance structs. */
    } msg_get_appliances_response;

    /**
     * @brief       Use this message to get all appliance profiles, or by whether they are
     *              schedulable or not.
     */
    typedef struct {
        tribool is_schedulable; /** Whether appliances are to be filtered by being schedulable or
                                    not (i.e. whether `schedules_per_week` > 0 or = 0). If
                                    indeterminate, get all appliance profiles regardless of their
                                    value for `schedules_per_week`. */
    } msg_get_appliances_all_request;

    /**
     * @brief       This message delivers the response to a `MSG_GET_APPLIANCES_ALL` request.
     */
    typedef struct {
        std::vector<appliance_t> appliances;    /** A vector of appliance structs. */
    } msg_get_appliances_all_response;

    /* END Messages to get appliances. */


    /* BEGIN Messages to get tasks. */

    /**
     * @brief       Use this message to get one or several tasks by id.
     */
    typedef struct {
        std::set<id_t> ids; /** A set of ids of the requested tasks. */
    } msg_get_tasks_by_id_request;

    /**
     * @brief       This message delivers the response to a `MSG_GET_TASKS_BY_ID` request.
     */
    typedef struct {
        std::map<id_t, task_t> tasks;   /** A map from task ids to task structs. */
    } msg_get_tasks_by_id_response;

    /**
     * @brief       Use this message to get tasks in a given timeframe, and filter by whether
     *              `is_user_declared` is true, false or indeterminate.
     */
    typedef struct {
        ptime start_time;           /** The beginning of the timeframe for which energy consumption
                                        data is requested. */
        ptime end_time;             /** The end of the timeframe for which energy consumption data
                                        is requested. */
        tribool is_user_declared;   /** Whether tasks are to be filtered if `is_user_declared` is
                                        true or false. If indeterminate, get tasks regardless of
                                        their value for `is_user_declared`. */
    } msg_get_tasks_by_time_request;

    /**
     * @brief       This message delivers the response to a `MSG_GET_TASKS_BY_TIME` request.
     */
    typedef struct {
        std::map<ptime, std::vector<task_t>> tasks; /** A map from beginnings of 15-minute intervals
                                                        to a vector of tasks in that interval.
                                                        The first key of the map is the beginning of
                                                        the interval that is the closest in the past
                                                        to `msg_get_tasks_by_time_request.start_time`. */
    } msg_get_tasks_by_time_response;

    /**
     * @brief       Use this message to get all tasks, and filter by whether `is_user_declared` is
     *              true, false or indeterminate.
     */
    typedef struct {
        tribool is_user_declared;   /** Whether tasks are to be filtered if `is_user_declared` is
                                        true or false. If indeterminate, get tasks regardless of
                                        their value for `is_user_declared`. */
    } msg_get_tasks_all_request;

    /**
     * @brief       This message delivers the response to a `MSG_GET_TASKS_ALL` request.
     */
    typedef struct {
        std::vector<task_t> tasks;  /** A vector of task structs. */
    } msg_get_tasks_all_response;

    /* END Messages to get tasks. */


    /* BEGIN Messages to get automation profiles. */

    /**
     * @brief       Use this message to get one or several automation profiles by id.
     */
    typedef struct {
        std::set<id_t> ids; /** A set of ids of the requested automation profiles. */
    } msg_get_auto_profiles_request;

    /**
     * @brief       This message delivers the response to a `MSG_GET_AUTO_PROFILES` request.
     */
    typedef struct {
        std::map<id_t, auto_profile_t> auto_profiles;   /** A map from task ids to task structs. */
    } msg_get_auto_profiles_response;

    /**
     * @brief       This message delivers the response to a `MSG_GET_AUTO_PROFILES_ALL` request.
     */
    typedef struct {
        std::vector<auto_profile_t> auto_profiles;  /** A vector of automation profile structs. */
    } msg_get_auto_profiles_all_response;

    /* END Messages to get automation profiles. */


    /* BEGIN Messages to get energy production. */

    /**
     * @brief       Use this message to get the amount of energy produced in a given timeframe.
     */
    typedef struct {
        ptime start_time;   /** The beginning of the timeframe for which energy production data is
                                requested. */
        ptime end_time;     /** The end of the timeframe for which energy production data is
                                requested. */
    } msg_get_energy_production_request;

    /**
     * @brief       This message delivers the response to a `MSG_GET_ENERGY_PRODUCTION` request.
     */
    typedef struct {
        std::map<ptime, energy_production_t> energy;    /** A map from beginnings of 15-minute intervals to the
                                                            respective amount of energy produced in that interval.
                                                            The first key of the map is the beginning of the
                                                            interval that is the closest in the past to
                                                            `msg_get_energy_production_request.start_time`. */
    } msg_get_energy_production_response;

    /**
     * @brief       Use this message to get the amount of energy produced throughout the HEMS'
     *              entire time of operation.
     *              No data needs to be transmitted for this request so its struct its empty and can
     *              be omitted.
     */
    typedef struct {} msg_get_energy_production_all_request;

    /**
     * @brief       This message delivers the response to a `MSG_GET_ENERGY_PRODUCTION_ALL` request.
     */
    typedef struct {
        std::map<ptime, energy_production_t> energy;    /** A map from beginnings of 15-minute intervals to the
                                                            respective amount of energy produced in that interval. */
    } msg_get_energy_production_all_response;

    /* END Messages to get energy production. */


    /* BEGIN Messages to get energy consumption. */

    /**
     * @brief       Use this message to get the amount of energy consumed by one, several or all
     *              appliances in a given timeframe.
     */
    typedef struct {
        std::set<id_t> appliance_ids;       /** A set of appliance ids for which energy consumption
                                                data is requested.
                                                An appliance id of 0 represents energy consumption
                                                data from appliances for which no individual
                                                measurements exist.
                                                An empty set means that energy consumption data is
                                                requested for all appliances. */
        ptime start_time;                   /** The beginning of the timeframe for which energy
                                                consumption data is requested. */
        ptime end_time;                     /** The end of the timeframe for which energy
                                                consumption data is requested. */
    } msg_get_energy_consumption_request;

    /**
     * @brief       This message delivers the response to a `MSG_GET_ENERGY_CONSUMPTION` request.
     */
    typedef struct {
        std::map<id_t, std::map<ptime, double>> energy; /** A map from appliance ids to time-to-energy-maps, the latter
                                                            meaning that the beginning of a 15-minute interval is mapped
                                                            to the respective amount of energy consumed by the appliance
                                                            in that interval.
                                                            The first key of the time-to-energy-map is the beginning of
                                                            the interval that is the closest in the past to
                                                            `msg_get_energy_consumption_request.start_time`. */
    } msg_get_energy_consumption_response;

    /**
     * @brief       Use this message to get the amount of energy consumed by one, several or all
     *              appliances throughout the HEMS' entire time of operation.
     */
    typedef struct {
        std::set<id_t> appliance_ids;       /** A set of appliance ids for which energy consumption
                                                data is requested.
                                                An appliance id of 0 represents energy consumption
                                                data from appliances for which no individual
                                                measurements exist.
                                                An empty set means that energy consumption data is
                                                requested for all appliances. */
    } msg_get_energy_consumption_all_request;

    /**
     * @brief       This message delivers the response to a `MSG_GET_ENERGY_CONSUMPTION_ALL` request.
     */
    typedef struct {
        std::map<id_t, std::map<ptime, double>> energy; /** A map from appliance ids to time-to-energy-maps, the latter
                                                            meaning that the beginning of a 15-minute interval is mapped
                                                            to the respective amount of energy consumed by the appliance
                                                            in that interval. */
    } msg_get_energy_consumption_all_response;

    /* END Messages to get energy consumption. */


    /* BEGIN Messages to get weather. */

    /**
     * @brief       Use this message to get the weather data for a given timeframe and weather
     *              stations.
     */
    typedef struct {
        ptime start_time;           /** The beginning of the timeframe for which weather data is
                                        requested. */
        ptime end_time;             /** The beginning of the timeframe for which weather data is
                                        requested. */
        std::set<id_t> stations;    /** The weather stations for which the weather data is
                                        requested. An empty set means that weather data is
                                        requested for all weather stations. */
    } msg_get_weather_request;

    /**
     * @brief       This message delivers the response to a `MSG_GET_WEATHER` request.
     */
    typedef struct {
        std::map<ptime, std::vector<weather_t>> time_to_weather;    /** A map from time points to
                                                                        weather data. */
        std::map<id_t, std::vector<weather_t>> station_to_weather;  /** A map from weather station
                                                                        id's to weather data. */
    } msg_get_weather_response;

    /* END Messages to get weather. */

}}}


namespace boost { namespace serialization {

    using namespace hems::messages::storage;

    template<typename Archive>
    void serialize(Archive& ar, msg_set_appliance_request& msg, const unsigned int version) {
        ar & msg.appliance;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_set_task_request& msg, const unsigned int version) {
        ar & msg.task;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_set_auto_profile_request& msg, const unsigned int version) {
        ar & msg.auto_profile;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_set_energy_consumption_request& msg, const unsigned int version) {
        ar & msg.energy_consumption;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_set_energy_production_request& msg, const unsigned int version) {
        ar & msg.energy_production;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_set_weather_request& msg, const unsigned int version) {
        ar & msg.weather;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_set_response& msg, const unsigned int version) {
        ar & msg.id;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_del_appliance_request& msg, const unsigned int version) {
        ar & msg.id;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_del_task_request& msg, const unsigned int version) {
        ar & msg.id;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_del_auto_profile_request& msg, const unsigned int version) {
        ar & msg.id;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_del_energy_consumption_request& msg, const unsigned int version) {
        ar & msg.time;
        ar & msg.appliance_id;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_del_energy_production_request& msg, const unsigned int version) {
        ar & msg.time;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_del_weather_request& msg, const unsigned int version) {
        ar & msg.time;
        ar & msg.station;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_appliances_request& msg, const unsigned int version) {
        ar & msg.ids;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_appliances_response& msg, const unsigned int version) {
        ar & msg.appliances;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_appliances_all_request& msg, const unsigned int version) {
        ar & msg.is_schedulable;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_appliances_all_response& msg, const unsigned int version) {
        ar & msg.appliances;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_tasks_by_id_request& msg, const unsigned int version) {
        ar & msg.ids;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_tasks_by_id_response& msg, const unsigned int version) {
        ar & msg.tasks;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_tasks_by_time_request& msg, const unsigned int version) {
        ar & msg.start_time;
        ar & msg.end_time;
        ar & msg.is_user_declared;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_tasks_by_time_response& msg, const unsigned int version) {
        ar & msg.tasks;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_tasks_all_request& msg, const unsigned int version) {
        ar & msg.is_user_declared;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_tasks_all_response& msg, const unsigned int version) {
        ar & msg.tasks;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_auto_profiles_request& msg, const unsigned int version) {
        ar & msg.ids;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_auto_profiles_response& msg, const unsigned int version) {
        ar & msg.auto_profiles;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_auto_profiles_all_response& msg, const unsigned int version) {
        ar & msg.auto_profiles;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_energy_production_request& msg, const unsigned int version) {
        ar & msg.start_time;
        ar & msg.end_time;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_energy_production_response& msg, const unsigned int version) {
        ar & msg.energy;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_energy_production_all_request& msg, const unsigned int version) {}

    template<typename Archive>
    void serialize(Archive& ar, msg_get_energy_production_all_response& msg, const unsigned int version) {
        ar & msg.energy;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_energy_consumption_request& msg, const unsigned int version) {
        ar & msg.appliance_ids;
        ar & msg.start_time;
        ar & msg.end_time;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_energy_consumption_response& msg, const unsigned int version) {
        ar & msg.energy;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_energy_consumption_all_request& msg, const unsigned int version) {
        ar & msg.appliance_ids;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_energy_consumption_all_response& msg, const unsigned int version) {
        ar & msg.energy;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_weather_request& msg, const unsigned int version) {
        ar & msg.start_time;
        ar & msg.end_time;
        ar & msg.stations;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_get_weather_response& msg, const unsigned int version) {
        ar & msg.time_to_weather;
        ar & msg.station_to_weather;
    }

}}

#endif /* HEMS_MESSAGES_STORAGE_H */
