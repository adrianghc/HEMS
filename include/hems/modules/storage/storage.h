/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Data Storage Module.
 * This module is responsible for managing access to data storage for all other modules.
 * Whenever other modules need to read or write measurements or other data, they can issue messages
 * to the Data Storage Module.
 */

#ifndef HEMS_MODULES_STORAGE_STORAGE_H
#define HEMS_MODULES_STORAGE_STORAGE_H

#include <map>
#include <string>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"

#include "sqlite/sqlite3.h"

namespace hems { namespace modules { namespace storage {

    using boost::archive::text_iarchive;
    using boost::archive::text_oarchive;

    /**
     * @brief       Wrapper for the message handler for `SETTINGS_INIT` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response
     *                      payload, if applicable. Otherwise `nullptr`.
     * 
     * @return      The settings broadcast response code.
     */
    int handler_wrapper_settings_init(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `SETTINGS_CHECK` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response
     *                      payload, if applicable. Otherwise `nullptr`.
     * 
     * @return      The settings broadcast response code.
     */
    int handler_wrapper_settings_check(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `SETTINGS_COMMIT` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response
     *                      payload, if applicable. Otherwise `nullptr`.
     * 
     * @return      The settings broadcast response code.
     */
    int handler_wrapper_settings_commit(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_SET_APPLIANCE` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                         if success.
     *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a field
     *                                              that violated a constraint.
     *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to be
     *                                              replaced.
     *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
     *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type entry's
     *                                              primary key already exist, which should
     *                                              absolutely not happen.
     */
    int handler_wrapper_msg_set_appliance(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_SET_TASK` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                         if success.
     *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a field
     *                                              that violated a constraint.
     *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to be
     *                                              replaced.
     *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
     *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type entry's
     *                                              primary key already exist, which should
     *                                              absolutely not happen.
     */
    int handler_wrapper_msg_set_task(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_SET_AUTO_PROFILE` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                         if success.
     *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a field
     *                                              that violated a constraint.
     *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to be
     *                                              replaced.
     *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
     *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type entry's
     *                                              primary key already exist, which should
     *                                              absolutely not happen.
     */
    int handler_wrapper_msg_set_auto_profile(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_SET_ENERGY_CONSUMPTION` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                         if success.
     *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a field
     *                                              that violated a constraint.
     *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to be
     *                                              replaced.
     *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
     *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type entry's
     *                                              primary key already exist, which should
     *                                              absolutely not happen.
     */
    int handler_wrapper_msg_set_energy_consumption(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_SET_ENERGY_PRODUCTION` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                         if success.
     *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a field
     *                                              that violated a constraint.
     *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to be
     *                                              replaced.
     *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
     *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type entry's
     *                                              primary key already exist, which should
     *                                              absolutely not happen.
     */
    int handler_wrapper_msg_set_energy_production(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_SET_WEATHER` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                         if success.
     *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a field
     *                                              that violated a constraint.
     *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to be
     *                                              replaced.
     *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
     *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type entry's
     *                                              primary key already exist, which should
     *                                              absolutely not happen.
     */
    int handler_wrapper_msg_set_weather(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_SET_SUNLIGHT` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                         if success.
     *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a field
     *                                              that violated a constraint.
     *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to be
     *                                              replaced.
     *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
     *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type entry's
     *                                              primary key already exist, which should
     *                                              absolutely not happen.
     */
    int handler_wrapper_msg_set_sunlight(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_DEL_APPLIANCE` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                         if success.
     *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
     *                                              field that violated a constraint.
     *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
     *                                              be replaced.
     *              MSG_DEL_SQL_ERROR               if an SQL query returned an error.
     */
    int handler_wrapper_msg_del_appliance(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_DEL_TASK` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                         if success.
     *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
     *                                              field that violated a constraint.
     *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
     *                                              be replaced.
     *              MSG_DEL_SQL_ERROR               if an SQL query returned an error.
     */
    int handler_wrapper_msg_del_task(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_DEL_AUTO_PROFILE` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                         if success.
     *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
     *                                              field that violated a constraint.
     *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
     *                                              be replaced.
     *              MSG_DEL_SQL_ERROR               if an SQL query returned an error.
     */
    int handler_wrapper_msg_del_auto_profile(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_DEL_ENERGY_CONSUMPTION` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                         if success.
     *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
     *                                              field that violated a constraint.
     *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
     *                                              be replaced.
     *              MSG_DEL_SQL_ERROR               if an SQL query returned an error.
     */
    int handler_wrapper_msg_del_energy_consumption(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_DEL_ENERGY_PRODUCTION` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                         if success.
     *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
     *                                              field that violated a constraint.
     *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
     *                                              be replaced.
     *              MSG_DEL_SQL_ERROR               if an SQL query returned an error.
     */
    int handler_wrapper_msg_del_energy_production(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_DEL_WEATHER` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                         if success.
     *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
     *                                              field that violated a constraint.
     *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
     *                                              be replaced.
     *              MSG_DEL_SQL_ERROR               if an SQL query returned an error.
     */
    int handler_wrapper_msg_del_weather(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_DEL_SUNLIGHT` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                         if success.
     *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
     *                                              field that violated a constraint.
     *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
     *                                              be replaced.
     *              MSG_DEL_SQL_ERROR               if an SQL query returned an error.
     */
    int handler_wrapper_msg_del_sunlight(text_iarchive& ia, text_oarchive* oa);

     /**
     * @brief       Wrapper for the message handler for `MSG_GET_SETTINGS` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response
     *                      payload, if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                 if success.
     *              MSG_GET_NONE_AVAILABLE  if no entries satisfying the request were available.
     *              MSG_GET_SQL_ERROR       if an SQL query returned an error.
     */
    int handler_wrapper_msg_get_settings(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_GET_APPLIANCES` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The response code // TODO.
     */
    int handler_wrapper_msg_get_appliances(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_GET_APPLIANCES_ALL` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The response code // TODO.
     */
    int handler_wrapper_msg_get_appliances_all(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_GET_TASKS_BY_ID` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The response code // TODO.
     */
    int handler_wrapper_msg_get_tasks_by_id(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_GET_TASKS_BY_TIME` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The response code // TODO.
     */
    int handler_wrapper_msg_get_tasks_by_time(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_GET_TASKS_ALL` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The response code // TODO.
     */
    int handler_wrapper_msg_get_tasks_all(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_GET_AUTO_PROFILES` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The response code // TODO.
     */
    int handler_wrapper_msg_get_auto_profiles(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_GET_AUTO_PROFILES_ALL` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The response code // TODO.
     */
    int handler_wrapper_msg_get_auto_profiles_all(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_GET_ENERGY_PRODUCTION` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                 if success.
     *              MSG_GET_NONE_AVAILABLE  if no entries satisfying the request were available.
     *              MSG_GET_SQL_ERROR       if an SQL query returned an error.
     */
    int handler_wrapper_msg_get_energy_production(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_GET_ENERGY_CONSUMPTION` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The response code // TODO.
     */
    int handler_wrapper_msg_get_energy_consumption(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_GET_ENERGY_CONSUMPTION_ALL` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The response code // TODO.
     */
    int handler_wrapper_msg_get_energy_consumption_all(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_GET_WEATHER` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      SUCCESS                 if success.
     *              MSG_GET_NONE_AVAILABLE  if no entries satisfying the request were available.
     *              MSG_GET_SQL_ERROR       if an SQL query returned an error.
     */
    int handler_wrapper_msg_get_weather(text_iarchive& ia, text_oarchive* oa);


    /**
     * @brief   The Data Storage Module class.
     *          This module is responsible for managing access to data storage for all other modules.
     *          Whenever other modules need to read or write measurements or other data, they can
     *          issue messages to the Data Storage Module.
     */
    class hems_storage {

        public:
            /**
             * @brief       Constructs the Data Storage Module.
             * 
             * @param[in]   db_path         The database file path.
             * @param[in]   test_mode       Whether the Data Storage Module should run in test mode,
             *                              in which case no settings initialization is required and
             *                              the messenger is also launched in test mode.
             */
            hems_storage(bool test_mode, std::string db_path);

            ~hems_storage();

            static const modules::type module_type = modules::type::STORAGE;    /** The type of this module. */

            static hems_storage* this_instance;     /** The Data Storage Module is conceptually a
                                                        singleton, a pointer to which is stored here. */

            /* BEGIN Message handlers. */

            /**
             * @brief       Message handler for `SETTINGS` messages. Depending on the specific
             *              subtype, this method will behave accordingly.
             * 
             * @param[in]   ia          The text archive containing the message.
             * @param[in]   oa          A text archive where the message handler can store the
             *                          response payload, if applicable. Otherwise `nullptr`.
             * @param[in]   check_only  True if the subtype is `SETTINGS_CHECK`.
             *                          False if the subtype is `SETTINGS_COMMIT`.
             * 
             * @return      The settings broadcast response code.
             */
            int handler_settings(text_iarchive& ia, text_oarchive* oa, bool check_only);

            /**
             * @brief       Message handler for `MSG_SET_APPLIANCE` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
             *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type
             *                                              entry's primary key already exist, which
             *                                              should absolutely not happen.
             */
            int handler_msg_set_appliance(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_SET_TASK` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
             *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type
             *                                              entry's primary key already exist, which
             *                                              should absolutely not happen.
             */
            int handler_msg_set_task(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_SET_AUTO_PROFILE` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
             *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type
             *                                              entry's primary key already exist, which
             *                                              should absolutely not happen.
             */
            int handler_msg_set_auto_profile(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_SET_ENERGY_CONSUMPTION` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
             *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type
             *                                              entry's primary key already exist, which
             *                                              should absolutely not happen.
             */
            int handler_msg_set_energy_consumption(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_SET_ENERGY_PRODUCTION` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
             *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type
             *                                              entry's primary key already exist, which
             *                                              should absolutely not happen.
             */
            int handler_msg_set_energy_production(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_SET_WEATHER` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
             *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type
             *                                              entry's primary key already exist, which
             *                                              should absolutely not happen.
             */
            int handler_msg_set_weather(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_SET_SUNLIGHT` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
             *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type
             *                                              entry's primary key already exist, which
             *                                              should absolutely not happen.
             */
            int handler_msg_set_sunlight(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_DEL_APPLIANCE` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_DEL_SQL_ERROR               if an SQL query returned an error.
             */
            int handler_msg_del_appliance(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_DEL_TASK` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_DEL_SQL_ERROR               if an SQL query returned an error.
             */
            int handler_msg_del_task(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_DEL_AUTO_PROFILE` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_DEL_SQL_ERROR               if an SQL query returned an error.
             */
            int handler_msg_del_auto_profile(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_DEL_ENERGY_CONSUMPTION` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_DEL_SQL_ERROR               if an SQL query returned an error.
             */
            int handler_msg_del_energy_consumption(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_DEL_ENERGY_PRODUCTION` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_DEL_SQL_ERROR               if an SQL query returned an error.
             */
            int handler_msg_del_energy_production(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_DEL_WEATHER` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_DEL_SQL_ERROR               if an SQL query returned an error.
             */
            int handler_msg_del_weather(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_DEL_SUNLIGHT` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_DEL_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_DEL_SQL_ERROR               if an SQL query returned an error.
             */
            int handler_msg_del_sunlight(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_GET_SETTINGS` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                 if success.
             *              MSG_GET_NONE_AVAILABLE  if no entries satisfying the request were
             *                                      available.
             *              MSG_GET_SQL_ERROR       if an SQL query returned an error.
             */
            int handler_msg_get_settings(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       A method shared by the handlers for `MSG_GET_APPLIANCES` and
             *              `MSG_GET_APPLIANCES_ALL`.
             * 
             * @param[in]   stmt1       The statement to select from the `appliances` table.
             * @param[in]   stmt2       The statement to select from the `appliances_tasks` table.
             * @param[in]   stmt3       The statement to select from the `appliances_auto_profiles`
             *                          table.
             * @param[in]   appliances  A map from appliance ids to appliances that the results of
             *                          the query are stored in.
             * 
             * @return      SUCCESS                 if success.
             *              MSG_GET_NONE_AVAILABLE  if no entries satisfying the request were
             *                                      available.
             *              MSG_GET_SQL_ERROR       if an SQL query returned an error.
             */
            int handler_msg_get_appliances_common(
                std::string& stmt1, std::string& stmt2, std::string& stmt3,
                std::map<types::id_t, types::appliance_t>& appliances
            );

            /**
             * @brief       Message handler for `MSG_GET_APPLIANCES` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The response code // TODO.
             */
            int handler_msg_get_appliances(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_GET_APPLIANCES_ALL` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The response code // TODO.
             */
            int handler_msg_get_appliances_all(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_GET_TASKS_BY_ID` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The response code // TODO.
             */
            int handler_msg_get_tasks_by_id(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_GET_TASKS_BY_TIME` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The response code // TODO.
             */
            int handler_msg_get_tasks_by_time(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_GET_TASKS_ALL` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The response code // TODO.
             */
            int handler_msg_get_tasks_all(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_GET_AUTO_PROFILES` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The response code // TODO.
             */
            int handler_msg_get_auto_profiles(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_GET_AUTO_PROFILES_ALL` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The response code // TODO.
             */
            int handler_msg_get_auto_profiles_all(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_GET_ENERGY_PRODUCTION` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                 if success.
             *              MSG_GET_NONE_AVAILABLE  if no entries satisfying the request were
             *                                      available.
             *              MSG_GET_SQL_ERROR       if an SQL query returned an error.
             */
            int handler_msg_get_energy_production(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_GET_ENERGY_CONSUMPTION` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The response code // TODO.
             */
            int handler_msg_get_energy_consumption(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_GET_ENERGY_CONSUMPTION_ALL` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The response code // TODO.
             */
            int handler_msg_get_energy_consumption_all(text_iarchive& ia, text_oarchive* oa);

            /**
             * @brief       Message handler for `MSG_GET_WEATHER` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      SUCCESS                 if success.
             *              MSG_GET_NONE_AVAILABLE  if no entries satisfying the request were
             *                                      available.
             *              MSG_GET_SQL_ERROR       if an SQL query returned an error.
             */
            int handler_msg_get_weather(text_iarchive& ia, text_oarchive* oa);

            /* END Message handlers. */

        private:
            static const messenger::msg_handler_map handler_map;    /** The message handler map for
                                                                        this module. */

            sqlite3* db_connection = nullptr;   /** The database connection object. */

            std::string db_path;    /** The SQLite database file name. */

            std::mutex db_mutex;    /** This mutex ensures that only one transaction is started at a
                                        time. */

            /**
             * @brief       Creates the database schema for the HEMS if the database is new.
             *              The database schema is described in the HEMS documentation.
             *              // TODO Specific reference
             * 
             * @return      SQLITE_OK if success, a corresponding SQL error value otherwise.
             */
            int create_schema();

            /**
             * @brief       This begins a database transaction.
             * 
             * @return      True if the db transaction could begin successfully, false otherwise.
             */
            bool db_begin();

            /**
             * @brief       This commits a database transaction.
             * 
             * @param[in]   Whether the database transaction was successful and should be committed,
             *              or be rollbacked otherwise.
             * 
             * @return      True if the db transaction could commit or rollback successfully, false
             *              otherwise.
             */
            bool db_commit(bool success);


            /* BEGIN Message handler submethods. */

            /**
             * @brief       This handles SET messages for those types whose primary key is an
             *              auto-incrementing id, and who have at least one field with a list of
             *              foreign keys.
             * 
             * @param[in]   new_id  A reference to the type's id.
             *                      If the id is 0, this indicates that a new entry is to be added,
             *                      in which case `new_id` will be replaced with the new entry's id.
             *                      If the id is not 0, it indicates that an existing entry is to be
             *                      replaced, in which case `new_id` will not be changed.
             * @param[in]   stmt1   An "UPDATE" statement for the type's table.
             * @param[in]   stmt2   An "INSERT INTO" statement for the type's table.
             * @param[in]   stmt3   A  "DELETE FROM" statement for the compound table that contains
             *                      the type's primary key and the foreign keys of the list.
             * @param[in]   stmt4   An "INSERT INTO" statement for the compound table that contains
             *                      the type's primary key and the foreign keys of the list.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
             *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type
             *                                              entry's primary key already exist, which
             *                                              should absolutely not happen.
             */
            int handler_msg_set_with_id(
                types::id_t& new_id,
                std::string& stmt1, std::string& stmt2, std::string& stmt3, std::string& stmt4
            );

            /**
             * @brief       This handles SET messages for those types whose primary key is not an
             *              auto-incrementing id.
             * 
             * @param[in]   stmt1   A "SELECT COUNT" statement to determine whether to update or
             *                      insert.
             * @param[in]   stmt2   An "UPDATE" statement.
             * @param[in]   stmt3   An "INSERT INTO" statement.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_SET_CONSTRAINT_VIOLATION    if the HEMS data type entry contained a
             *                                              field that violated a constraint.
             *              MSG_SET_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_SET_SQL_ERROR               if an SQL query returned an error.
             *              MSG_SET_FATAL_DUPLICATE         if several entries with the data type
             *                                              entry's primary key already exist, which
             *                                              should absolutely not happen.
             */
            int handler_msg_set_without_id(std::string& stmt1, std::string& stmt2, std::string& stmt3);

            /**
             * @brief       This handles DEL messages.
             * 
             * @param[in]   stmt    A "DELETE FROM" statement.
             * 
             * @return      SUCCESS                         if success.
             *              MSG_DEL_REPLACE_NON_EXISTING    if a non-existing entry was attempted to
             *                                              be replaced.
             *              MSG_DEL_SQL_ERROR               if an SQL query returned an error.
             */
            int handler_msg_del(std::string& stmt);

            /* END Message handler submethods. */

            friend class hems_storage_test; /* Friend class to let tests access private members. */
    };

}}}

#endif /* HEMS_MODULES_STORAGE_STORAGE_H */
