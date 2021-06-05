/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This header defines the interface for the Measurement Collection Module.
 * The interface consists of message types for messages that are put in the module's message queue.
 */

#ifndef HEMS_MESSAGES_COLLECTION_H
#define HEMS_MESSAGES_COLLECTION_H

#include <utility>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/utility.hpp>

#include "hems/common/types.h"

namespace hems { namespace messages { namespace collection {

    using boost::posix_time::ptime;

    /**
     * @brief       Identifiers for message types of the Measurement Collection Module.
     */
    enum msg_type {
        MSG_DOWNLOAD_ENERGY_PRODUCTION,
        MSG_DOWNLOAD_ENERGY_CONSUMPTION,
        MSG_DOWNLOAD_WEATHER_DATA
    };

    /**
     * @brief       Response codes for the messages of the Measurement Collection Module and its
     *              data download methods.
     */
    enum response_code {
        SUCCESS = 0x00,
        INVALID_DATA,
        UNREACHABLE_SOURCE,
        INVALID_RESPONSE_SOURCE,
        DATA_STORAGE_MODULE_ERR
    };


    /**
     * @brief       Use this message to request download of energy production data for a given
     *              15-minute interval.
     */
    typedef struct {
        ptime time; /** The end of the 15-minute interval for which to download the energy
                        production data. */
    } msg_download_energy_production_request;

    /**
     * @brief       Use this message to request download of energy consumption data for a given
     *              15-minute interval and a given appliance.
     */
    typedef struct {
        types::id_t appliance;  /** The id of the appliance for which to download the energy
                                    consumption data. */
        ptime       time;       /** The end of the 15-minute interval for which to download the
                                    energy consumption data. */
    } msg_download_energy_consumption_request;

    /**
     * @brief       Use this message to request download of weather data for a given time and
     *              weather station.
     */
    typedef struct {
        ptime       time;       /** The time for which to download the weather data. */
        types::id_t station;    /** The weather station for which to download the weather data. */
    } msg_download_weather_data_request;

}}}


namespace boost { namespace serialization {

    using namespace hems::messages::collection;

    template<typename Archive>
    void serialize(Archive& ar, msg_download_energy_production_request& msg, const unsigned int version) {
        ar & msg.time;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_download_energy_consumption_request& msg, const unsigned int version) {
        ar & msg.appliance;
        ar & msg.time;
    }

    template<typename Archive>
    void serialize(Archive& ar, msg_download_weather_data_request& msg, const unsigned int version) {
        ar & msg.time;
        ar & msg.station;
    }

}}

#endif /* HEMS_MESSAGES_COLLECTION_H */
