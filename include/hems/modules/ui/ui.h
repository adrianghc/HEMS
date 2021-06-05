/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the User Interface Module.
 * This module is responsible for offering an interface for the user to interact with the HEMS,
 * presenting the current state and offering commands.
 */

#ifndef HEMS_MODULES_UI_UI_H
#define HEMS_MODULES_UI_UI_H

#include <string>
#include <thread>
#include <tuple>
#include <vector>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"

namespace hems { namespace modules { namespace ui {

    using boost::archive::text_iarchive;
    using boost::archive::text_oarchive;

    using boost::posix_time::ptime;

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
     * @brief       Wrapper for the callback function for HTTP POST requests.
     *              This method dynamically handles different HTTP POST requests for different
     *              resources.
     * 
     * @param[in]   body    The body of the HTTP POST request.
     * @param[in]   target  The target of the HTTP POST request.
     * 
     * @return      A tuple of
     *              - a string containing the path or the content of the response resource,
     *              - a string containing the response resource's MIME type,
     *              - whether the response resource's path is returned, or its content.
     */
    std::tuple<const std::string, const std::string, bool> post_callback_wrapper(
        const std::string& body, const std::string& target
    );


    /**
     * @brief   The User Interface Module class.
     *          This module is responsible for offering an interface for the user to interact with
     *          the HEMS, presenting the current state and offering commands.
     */
    class hems_ui {
        public:
            /**
             * @brief       Constructs the User Interface Module.
             * 
             * @param[in]   test_mode       Whether the User Interface Module should run in test
             *                              mode, in which case no settings initialization takes
             *                              place and the messenger is also launched in test mode.
             * @param[in]   ui_server_root  The root directory used by the HTTP server for the user
             *                              interface.
             */
            hems_ui(bool test_mode, std::string ui_server_root);

            ~hems_ui();

            static const modules::type module_type = modules::type::UI; /** The type of this module. */

            static hems_ui* this_instance;  /** The User Interface Module is conceptually a
                                                singleton, a pointer to which is stored here. */

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

            /* END Message handlers. */

            /**
             * @brief       The callback function for HTTP POST requests.
             *              This method dynamically handles different HTTP POST requests for
             *              different resources.
             * 
             * @param[in]   body    The body of the HTTP POST request.
             * @param[in]   target  The target of the HTTP POST request.
             * 
             * @return      A tuple of
             *              - a string containing the path or the content of the response resource,
             *              - a string containing the response resource's MIME type,
             *              - whether the response resource's path is returned, or its content.
             */
            std::tuple<const std::string, const std::string, bool> post_callback(
                const std::string& body, const std::string& target
            );


            /* BEGIN POST Request handlers. */

            /**
             * @brief       This handles HTTP POST requests to the UI telling the HEMS to download
             *              weather data for a given interval.
             * 
             * @param[in]   request_map     A map containing key-value-pairs for all parameters of
             *                              the request, including at the very least a value for the
             *                              `action` key, which specifies the command provided by
             *                              the HTTP POST request.
             * 
             * @return      The content body that serves as response to the HTTP POST request.
             */
            std::string handler_set_stations(std::map<std::string, std::string>& request_map);

            /**
             * @brief       This handles HTTP POST requests to the UI telling the HEMS to download
             *              weather data for a given interval.
             * 
             * @param[in]   request_map     A map containing key-value-pairs for all parameters of
             *                              the request, including at the very least a value for the
             *                              `action` key, which specifies the command provided by
             *                              the HTTP POST request.
             * 
             * @return      The content body that serves as response to the HTTP POST request.
             */
            std::string handler_download_weather(std::map<std::string, std::string>& request_map);

            /**
             * @brief       This handles HTTP POST requests to the UI telling the HEMS to download
             *              weather data for a given interval.
             * 
             * @param[in]   request_map     A map containing key-value-pairs for all parameters of
             *                              the request, including at the very least a value for the
             *                              `action` key, which specifies the command provided by
             *                              the HTTP POST request.
             * 
             * @return      The content body that serves as response to the HTTP POST request.
             */
            std::string handler_download_energy_production(std::map<std::string, std::string>& request_map);

            /**
             * @brief       This handles HTTP POST requests to the UI telling the HEMS to get
             *              energy production predictions for a given week.
             * 
             * @param[in]   request_map     A map containing key-value-pairs for all parameters of
             *                              the request, including at the very least a value for the
             *                              `action` key, which specifies the command provided by
             *                              the HTTP POST request.
             * 
             * @return      The content body that serves as response to the HTTP POST request.
             */
            std::string handler_get_energy_production_predictions(
                std::map<std::string, std::string>& request_map
            );

            /**
             * @brief       This handles HTTP POST requests to the UI telling the HEMS to set an
             *              appliance.
             * 
             * @param[in]   request_map     A map containing key-value-pairs for all parameters of
             *                              the request, including at the very least a value for the
             *                              `action` key, which specifies the command provided by
             *                              the HTTP POST request.
             * 
             * @return      The content body that serves as response to the HTTP POST request.
             */
            std::string handler_set_appliance(std::map<std::string, std::string>& request_map);

            /**
             * @brief       This handles HTTP POST requests to the UI telling the HEMS to get
             *              appliances.
             * 
             * @param[in]   request_map     A map containing key-value-pairs for all parameters of
             *                              the request, including at the very least a value for the
             *                              `action` key, which specifies the command provided by
             *                              the HTTP POST request.
             * 
             * @return      The content body that serves as response to the HTTP POST request.
             */
            std::string handler_get_appliances(std::map<std::string, std::string>& request_map);

            /**
             * @brief       This handles HTTP POST requests to the UI telling the HEMS to delete an
             *              appliance (or all of them).
             * 
             * @param[in]   request_map     A map containing key-value-pairs for all parameters of
             *                              the request, including at the very least a value for the
             *                              `action` key, which specifies the command provided by
             *                              the HTTP POST request.
             * 
             * @return      The content body that serves as response to the HTTP POST request.
             */
            std::string handler_del_appliance(std::map<std::string, std::string>& request_map);

            /**
             * @brief       This handles HTTP POST requests to the UI telling the HEMS to request
             *              task recommendations.
             * 
             * @param[in]   request_map     A map containing key-value-pairs for all parameters of
             *                              the request, including at the very least a value for the
             *                              `action` key, which specifies the command provided by
             *                              the HTTP POST request.
             * 
             * @return      The content body that serves as response to the HTTP POST request.
             */
            std::string handler_get_recommendations(std::map<std::string, std::string>& request_map);

            /* END POST Request handlers. */

        private:
            static const messenger::msg_handler_map handler_map;    /** The message handler map for
                                                                        this module. */

            std::thread* ui_server_worker;  /** A pointer to the worker thread of the user
                                                interface's HTTP server. */

            std::string ui_server_root; /** The root directory used by the user interface's HTTP
                                            server. */

            /**
             * @brief       Start the worker and listen to incoming HTTP GET or POST requests.
             */
            void listen();

            /**
             * Maps command strings (given via HTTP POST request) to functions that handle these
             * commands.
             * 
             * The handler functions take as argument:
             * 
             * A map containing key-value-pairs for all parameters of the request, including at the
             * very least a value for the `action` key, which specifies the command provided by the
             * HTTP POST request.
             * 
             * The handler functions return:
             * 
             * The content body that serves as response to the HTTP POST request.
             */
            static const std::map<std::string, const std::function<std::string(std::map<std::string, std::string>&)>>
            request_handlers;

            /**
             * @brief       Reads a resource into memory and replaces the dynamic content area with the
             *              content given via argument.
             * 
             * @param[in]   filename            The name of the resource.
             * @param[in]   dynamic_content     The dynamic content to place into the resource's dynamic
             *                                  content area.
             * 
             * @return      The resource's content with the dynamic content placed in.
             */
            static std::string set_dynamic_content_of_file(std::string filename, std::string dynamic_content);


            friend class ui_test;   /* Friend class to let tests access private members. */
    };

}}}

#endif /* HEMS_MODULES_UI_UI_H */
