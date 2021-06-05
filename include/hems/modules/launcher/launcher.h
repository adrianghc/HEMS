/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the header for the HEMS Launcher. The HEMS Launcher is the executable that starts the
 * entire HEMS, launching all modules and initializing their message queues. The lifecycle of the
 * HEMS is identical with the launcher's lifecycle, therefore when the launcher terminates, the
 * entire HEMS does. The launcher also collects errors, outputs and log messages from all modules
 * and writes them to files and standard I/O streams.
 */

#ifndef HEMS_MODULES_LAUNCHER_LAUNCHER_H
#define HEMS_MODULES_LAUNCHER_LAUNCHER_H

#include <map>
#include <mutex>
#include <sys/types.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/thread/shared_mutex.hpp>

#include "extras/semaphore.hpp"

#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"

namespace hems { namespace modules { namespace launcher {

    using boost::archive::text_iarchive;
    using boost::archive::text_oarchive;

    /**
     * @brief       Wrapper for the message handler for `SETTINGS` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The settings broadcast response code (always `SUCCESS`).
     */
    int handler_wrapper_settings(text_iarchive& ia, text_oarchive* oa);

    /**
     * @brief       Wrapper for the message handler for `MSG_LOG` messages.
     * 
     * @param[in]   ia      The text archive containing the message.
     * @param[in]   oa      A text archive where the message handler can store the response payload,
     *                      if applicable. Otherwise `nullptr`.
     * 
     * @return      The response code (always 0).
     */
    int handler_wrapper_msg_log(text_iarchive& ia, text_oarchive* oa);


    /**
     * @brief   The HEMS Launcher class.
     *          The HEMS Launcher launches all modules and initializes their message queues. It also
     *          collects errors, outputs and log messages from all modules and writes them to files
     *          and standard I/O streams.
     */
    class hems_launcher {

        public:
            /**
             * @brief       Constructs the HEMS Launcher.
             * 
             * @param[in]   debug           Whether the HEMS Launcher should run in debug mode, in
             *                              which case modules are not started/ended automatically
             *                              and need to be started/ended manually by the user.
             * @param[in]   test_mode       Whether the HEMS Launcher should run in test mode, in
             *                              which case no settings initialization takes place and
             *                              the messenger is also launched in test mode.
             * 
             * @param[in]   storage_path    The path to the Data Storage Module.
             * @param[in]   db_path         The database file path.
             * 
             * @param[in]   collection_path The path to the Measurement Collection Module.
             * 
             * @param[in]   ui_path         The path to the User Interface Module.
             * @param[in]   ui_server_root  The root directory used by the HTTP server for the user
             *                              interface.
             * 
             * @param[in]   inference_path  The path to the Knowledge Inference Module.
             * 
             * @param[in]   automation_path The path to the Automation and Recommendation Module.
             * 
             * @param[in]   training_path   The path to the Model Training Module.
             * 
             * @param[in]   settings        The new HEMS settings as given through command line
             *                              arguments.
             */
            hems_launcher(
                bool debug, bool test_mode,
                std::string storage_path, std::string db_path, std::string collection_path,
                std::string ui_path, std::string ui_server_root, std::string inference_path,
                std::string automation_path, std::string training_path, types::settings_t settings
            );

            /**
             * @brief       Constructs the HEMS Launcher.
             * 
             * @param[in]   debug           Whether the HEMS Launcher should run in debug mode, in
             *                              which case modules are not started/ended automatically
             *                              and need to be started/ended manually by the user.
             * @param[in]   test_mode       Whether the HEMS Launcher should run in test mode, in
             *                              which case no settings initialization takes place and
             *                              the messenger is also launched in test mode.
             * 
             * @param[in]   storage_path    The path to the Data Storage Module.
             * @param[in]   db_path         The database file path.
             * 
             * @param[in]   collection_path The path to the Measurement Collection Module.
             * 
             * @param[in]   ui_path         The path to the User Interface Module.
             * @param[in]   ui_server_root  The root directory used by the HTTP server for the user
             *                              interface.
             * 
             * @param[in]   inference_path  The path to the Knowledge Inference Module.
             * 
             * @param[in]   automation_path The path to the Automation and Recommendation Module.
             * 
             * @param[in]   training_path   The path to the Model Training Module.
             */
            hems_launcher(
                bool debug, bool test_mode,
                std::string storage_path, std::string db_path, std::string collection_path,
                std::string ui_path, std::string ui_server_root, std::string inference_path,
                std::string automation_path, std::string training_path
            );

            ~hems_launcher();

            static const modules::type module_type = modules::type::LAUNCHER;   /** The type of this module. */

            static hems_launcher* this_instance;    /** The Launcher Module is conceptually a
                                                        singleton, a pointer to which is stored here. */

            /* BEGIN Message handlers. */

            /**
             * @brief       Message handler for `MSG_LOG` messages.
             * 
             * @param[in]   ia      The text archive containing the message.
             * @param[in]   oa      A text archive where the message handler can store the response
             *                      payload, if applicable. Otherwise `nullptr`.
             * 
             * @return      The response code (always 0).
             */
            int handler_msg_log(text_iarchive& ia, text_oarchive* oa);

            /* END Message handlers. */

        private:
            static const messenger::msg_handler_map handler_map;    /** The message handler map for
                                                                        this module. */

            std::thread* watch_storage = nullptr;           /** Thread that watches the Data Storage
                                                                Module process. */
            std::thread* watch_collection = nullptr;        /** Thread that watches the Measurement
                                                                Collection Module process. */
            std::thread* watch_ui = nullptr;                /** Thread that watches the User
                                                                Interface Module process. */
            std::thread* watch_inference = nullptr;         /** Thread that watches the Knowledge
                                                                Inference Module process. */
            std::thread* watch_automation = nullptr;        /** Thread that watches the Automation
                                                                and Recommendation Module process. */
            std::thread* watch_training = nullptr;          /** Thread that watches the Model
                                                                Training Module process. */

            std::thread* init_settings_thread = nullptr;    /** Setting initializer thread. */

            struct {
                std::map<modules::type, pid_t> map; /** Maps module identifiers to their respective
                                                        PIDs. */
                boost::shared_mutex mutex;          /** A mutex to protect access to `pids.map`. */
            } pids;

            struct {
                sig_atomic_t num {0};   /** The number of active module watcher threads. */
                semaphore sem;          /** A semaphore to keep track of module watchers. */
            } watch_count;

            bool debug; /** Whether the HEMS Launcher should run in debug mode, in which case
                            modules are not started/ended automatically and need to be started/ended
                            manually by the user. */

            /**
             * @brief   Creates the message queues for all modules.
             * 
             * @return  EXIT_SUCCESS if success.
             *          EXIT_FAILURE if error.
             */
            int create_msg_queues();

            /**
             * @brief   Deletes the message queues for all modules.
             */
            void delete_msg_queues();

            /**
             * @brief       Watches a module for whether it's still running, and if it isn't, which
             *              exit code it returned. When a module has shut down, the module watcher
             *              will issue a call to shut down the Launcher Module itself.
             * 
             * @param[in]   module      The module to be watched.
             */
            void watch_module(modules::type module);

            /**
             * @brief       Initializes the HEMS settings across all modules by first acquiring the
             *              current settings (if any) from the Data Storage Module and broadcast
             *              them to all modules. Then, if new settings were given through command
             *              line arguments, broadcast the new settings.
             * 
             * @param[in]   settings    The new settings given through command line arguments. If
             *                          none were given, this will be the undefined settings struct.
             */
            void init_settings(types::settings_t settings);

    };

}}}

#endif /* HEMS_MODULES_LAUNCHER_LAUNCHER_H */
