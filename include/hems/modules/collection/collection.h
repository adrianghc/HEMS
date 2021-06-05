/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Measurement Collection Module.
 * This module is responsible for collecting measurement data. Each type of continuously
 * accumulating measurement data is pulled from its source through an appropriate submodule at a
 * given schedule.
 */

#ifndef HEMS_MODULES_COLLECTION_COLLECTION_H
#define HEMS_MODULES_COLLECTION_COLLECTION_H

#include <vector>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"

namespace hems { namespace modules { namespace collection {

    using boost::archive::text_iarchive;
    using boost::archive::text_oarchive;

    using boost::posix_time::ptime;
    using hems::types::id_t;

    /**
     * @brief       Wrapper for the message handler for `SETTINGS_INIT` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The settings broadcast response code.
     */
    int handler_wrapper_settings_init(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `SETTINGS_CHECK` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The settings broadcast response code.
     */
    int handler_wrapper_settings_check(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `SETTINGS_COMMIT` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The settings broadcast response code.
     */
    int handler_wrapper_settings_commit(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_DOWNLOAD_ENERGY_PRODUCTION` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                 if success.
     *              INVALID_DATA            if failed due to invalid data.
     *              UNREACHABLE_SOURCE      if failed due to unreachable source server.
     *              INVALID_RESPONSE_SOURCE if failed due to invalid response from source server.
     *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
     */
    int handler_wrapper_msg_download_energy_production(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_DOWNLOAD_ENERGY_CONSUMPTION` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                 if success.
     *              INVALID_DATA            if failed due to invalid data.
     *              UNREACHABLE_SOURCE      if failed due to unreachable source server.
     *              INVALID_RESPONSE_SOURCE if failed due to invalid response from source server.
     *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
     */
    int handler_wrapper_msg_download_energy_consumption(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_DOWNLOAD_WEATHER_DATA` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                 if success.
     *              INVALID_DATA            if failed due to invalid data.
     *              UNREACHABLE_SOURCE      if failed due to unreachable source server.
     *              INVALID_RESPONSE_SOURCE if failed due to invalid response from source server.
     *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
     */
    int handler_wrapper_msg_download_weather_data(text_iarchive& ia, text_oarchive* oa);


    /**
     * @brief   The Measurement Collection Module class.
     *          This module is responsible for collecting measurement data. Each type of
     *          continuously accumulating measurement data is pulled from its source through an
     *          appropriate submodule at a given schedule.
     */
    class hems_collection {
        public:
            /**
             * @brief       Constructs the Measurement Collection Module.
             * 
             * @param[in]   test_mode       Whether the Measurement Collection Module should run in
             *                              test mode, in which case no settings initialization
             *                              takes place and the messenger is also launched in test
             *                              mode.
             */
            hems_collection(bool test_mode);

            ~hems_collection();

            static const modules::type module_type = modules::type::COLLECTION; /** The type of this module. */

            static hems_collection* this_instance;  /** The Measurement Collection Module is
                                                        conceptually a singleton, a pointer to which
                                                        is stored here. */

            double longitude;   /** The longitude value to be used as the user's location. */
            double latitude;    /** The latitude value to be used as the user's location. */
            double timezone;    /** The timezone in which the user is located. */

            /* BEGIN Message handlers. */

            /**
             * @brief       Message handler for `SETTINGS_INIT` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The settings broadcast response code.
             */
            int handler_settings_init(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `SETTINGS_CHECK` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The settings broadcast response code.
             */
            int handler_settings_check(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `SETTINGS_COMMIT` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The settings broadcast response code.
             */
            int handler_settings_commit(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_DOWNLOAD_ENERGY_PRODUCTION` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                 if success.
             *              INVALID_DATA            if failed due to invalid data.
             *              UNREACHABLE_SOURCE      if failed due to unreachable source server.
             *              INVALID_RESPONSE_SOURCE if failed due to invalid response from source
             *                                      server.
             *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
             */
            int handler_msg_download_energy_production(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_DOWNLOAD_ENERGY_CONSUMPTION` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                 if success.
             *              INVALID_DATA            if failed due to invalid data.
             *              UNREACHABLE_SOURCE      if failed due to unreachable source server.
             *              INVALID_RESPONSE_SOURCE if failed due to invalid response from source
             *                                      server.
             *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
             */
            int handler_msg_download_energy_consumption(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_DOWNLOAD_WEATHER_DATA` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                 if success.
             *              INVALID_DATA            if failed due to invalid data.
             *              UNREACHABLE_SOURCE      if failed due to unreachable source server.
             *              INVALID_RESPONSE_SOURCE if failed due to invalid response from source
             *                                      server.
             *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
             */
            int handler_msg_download_weather_data(text_iarchive& ia, text_oarchive* oa);

            /* END Message handlers. */

        private:
            static const messenger::msg_handler_map handler_map;    /** The message handler map for
                                                                        this module. */

            /**
             * @brief       Downloads the energy production data for the given time.
             * 
             * @param[in]   time    The time for which to get energy production data.
             * 
             * @return      SUCCESS                 if success.
             *              INVALID_DATA            if failed due to invalid data.
             *              UNREACHABLE_SOURCE      if failed due to unreachable source server.
             *              INVALID_RESPONSE_SOURCE if failed due to invalid response from source server.
             *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
             */
            int download_energy_production(ptime time);

            /**
             * @brief       Downloads the energy consumption data of a given appliance for the given
             *              time.
             * 
             * @param[in]   time        The time for which to get energy consumption data.
             * @param[in]   appliance   The appliance for which to get energy consumption data.
             * 
             * @return      // TODO
             */
            int download_energy_consumption(ptime time, id_t appliance);

            /**
             * @brief       Downloads weather data for a given time, coordinates and weather station.
             * 
             * @param[in]   time        The time for which to get weather data.
             * @param[in]   station     The station for which to get weather data.
             * 
             * @return      SUCCESS                 if success.
             *              INVALID_DATA            if failed due to invalid data.
             *              UNREACHABLE_SOURCE      if failed due to unreachable source server.
             *              INVALID_RESPONSE_SOURCE if failed due to invalid response from source server.
             *              DATA_STORAGE_MODULE_ERR if failed due to error from Data Storage Module.
             */
            int download_weather_data(ptime time, id_t station);

            /**
             * @brief       Checks that location data is valid.
             * 
             * @param[in]   longitude   The longitude of the location.
             * @param[in]   latitude    The latitude of the location.
             * @param[in]   timezone    The timezone for the location. Must be between -12 and 12,
             *                          fractions are allowed for non-integer timezones.
             * 
             * @return      True if valid, false otherwise.
             */
            bool check_location_validity(double longitude, double latitude, double timezone);

            /**
             * @brief       Gets a string representation of a timezone without unnecessary double
             *              precision.
             * 
             * @param[in]   timezone    The timezone as a double.
             * @return      A string for the timezone.
             */
            std::string get_timezone_str(double timezone);


            friend class collection_test; /* Friend class to let tests access private members. */
    };

}}}

#endif /* HEMS_MODULES_COLLECTION_COLLECTION_H */
