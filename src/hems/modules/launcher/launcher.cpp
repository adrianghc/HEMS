/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the code for the HEMS Launcher. The HEMS Launcher is the executable that starts the
 * entire HEMS, launching all modules and initializing their message queues. The lifecycle of the
 * HEMS is identical with the launcher's lifecycle, therefore when the launcher terminates, the
 * entire HEMS does. The launcher also collects errors, outputs and log messages from all modules
 * and writes them to files and standard I/O streams.
 */

#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <csignal>
#include <cstring>
#include <cstdlib>
#include <mqueue.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <boost/thread/shared_mutex.hpp>

#include "extras/semaphore.hpp"

#include "hems/modules/launcher/launcher.h"
#include "hems/modules/launcher/local_logger.h"
#include "hems/common/exit.h"
#include "hems/common/logger.h"
#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/messages/storage.h"

namespace hems { namespace modules { namespace launcher {

    hems_launcher* hems_launcher::this_instance = nullptr;

    hems_launcher::hems_launcher(
        bool debug, bool test_mode, std::string storage_path, std::string db_path,
        std::string collection_path, std::string ui_path, std::string ui_server_root,
        std::string inference_path, std::string automation_path, std::string training_path
    ) : hems_launcher(
            debug, test_mode, storage_path, db_path, collection_path, ui_path, ui_server_root,
            inference_path, automation_path, training_path, types::settings_undefined
        ) {};

    hems_launcher::hems_launcher(
        bool debug, bool test_mode,
        std::string storage_path, std::string db_path, std::string collection_path,
        std::string ui_path, std::string ui_server_root, std::string inference_path,
        std::string automation_path, std::string training_path, types::settings_t settings
    ) : debug(debug) {

        logger::this_logger->log(
            "Starting " + modules::to_string_extended(module_type) + ".",
            logger::level::LOG
        );

        /* Delete message queues to remove junk messages from previous runs. */
        delete_msg_queues();

        /* Create message queues. */
        if (create_msg_queues()) {
            logger::this_logger->log("Error opening message queues, aborting.", logger::level::ERR);
            throw EXIT_FAILURE;
        } else {
            logger::this_logger->log("Message queues created successfully.", logger::level::DBG);
        }

        /* Open messenger object. */
        messenger::this_messenger = new messenger(module_type, test_mode);

        /* Begin listening for messages. */
        if (!messenger::this_messenger->listen(handler_map)) {
            logger::this_logger->log("Cannot listen for messages, aborting.", logger::level::ERR);
            throw EXIT_FAILURE;
        } else {
            logger::this_logger->log("Listening for messages.", logger::level::LOG);
        }

        /*  If debugging mode is on, we skip launching the modules and watching if they are still
            running, and let the user launch the modules themselves instead. */
        if (!debug) {
            /* BEGIN Launch modules. */

            auto start_module = [this](modules::type module, const char* file, char* const* argv) {
                if (!std::ifstream(file).good()) {
                    logger::this_logger->log(
                        "Cannot find binary '" + std::string(file) + "' for "
                        + modules::to_string_extended(module) + ", aborting.",
                        logger::level::ERR
                    );
                    throw EXIT_FAILURE;
                }
                switch (pid_t pid = fork()) {
                    case -1:
                        logger::this_logger->log(
                            "Couldn't start " + modules::to_string_extended(module) + ", aborting: " +
                                strerror(errno),
                            logger::level::ERR
                        );
                        throw EXIT_FAILURE;
                        break;
                    case 0:
                        execv(file, argv);
                        /*  `exec` only returns if something failed, so if the next line is ever
                            reached, something went wrong. We use `std::exit()` directly here
                            because we don't want a fork of the launcher module to call its destructor
                            as well. */
                        logger::this_logger->log(
                            "Couldn't start " + modules::to_string_extended(module) + ", aborting: " +
                                strerror(errno),
                            logger::level::ERR
                        );
                        std::exit(EXIT_FAILURE);
                        break;
                    default:
                        pids.mutex.lock();
                        pids.map[module] = pid;
                        pids.mutex.unlock();
                        break;
                }
            };

            const char* argv1[] = { storage_path.c_str(), "--db", db_path.c_str(), (char*) nullptr };
            start_module(modules::type::STORAGE, storage_path.c_str(), const_cast<char* const*>(argv1));
            watch_storage = new std::thread(&hems_launcher::watch_module, this, modules::type::STORAGE);

            const char* argv2[] = { collection_path.c_str(), (char*) nullptr };
            start_module(modules::type::COLLECTION, collection_path.c_str(), const_cast<char* const*>(argv2));
            watch_collection = new std::thread(&hems_launcher::watch_module, this, modules::type::COLLECTION);

            const char* argv3[] = { ui_path.c_str(), "--root", ui_server_root.c_str(), (char*) nullptr };
            start_module(modules::type::UI, ui_path.c_str(), const_cast<char* const*>(argv3));
            watch_ui = new std::thread(&hems_launcher::watch_module, this, modules::type::UI);

            const char* argv4[] = { inference_path.c_str(), (char*) nullptr };
            start_module(modules::type::INFERENCE, inference_path.c_str(), const_cast<char* const*>(argv4));
            watch_inference = new std::thread(&hems_launcher::watch_module, this, modules::type::INFERENCE);

            const char* argv5[] = { automation_path.c_str(), (char*) nullptr };
            start_module(modules::type::AUTOMATION, automation_path.c_str(), const_cast<char* const*>(argv5));
            watch_automation = new std::thread(&hems_launcher::watch_module, this, modules::type::AUTOMATION);

            const char* argv6[] = { training_path.c_str(), (char*) nullptr };
            start_module(modules::type::TRAINING, training_path.c_str(), const_cast<char* const*>(argv6));
            watch_training = new std::thread(&hems_launcher::watch_module, this, modules::type::TRAINING);

            /* END Launch modules. */
        }

        if (!test_mode) {
            /*  This must run in a separate thread because while it is initiated during the Launcher
                Module's construction, the constructor must finish before the settings
                initialization does. */
            init_settings_thread = new std::thread(&hems_launcher::init_settings, this, settings);
        }

        /* Message handlers must not be called before the module's constructor has finished. */
        logger::this_logger->log("Begin handling incoming messages.", logger::level::LOG);
        messenger::this_messenger->start_handlers();
    }

    hems_launcher::~hems_launcher() {
        logger::this_logger->log(
            "Shutting down " + modules::to_string_extended(module_type) + ".",
            logger::level::LOG
        );

        if (init_settings_thread != nullptr) {
            init_settings_thread->join();
            delete init_settings_thread;
        }

        /* BEGIN Shut down modules. */

        /*  If debugging mode is on, the modules were not launched automatically, so they cannot be
            shut down either. */
        if (!debug) {
            pids.mutex.lock_shared();
            for (const auto& item : pids.map) {
                modules::type module = item.first;
                pid_t pid = item.second;

                logger::this_logger->log(
                    "Signaling " + modules::to_string_extended(module) + " to shut down.",
                    logger::level::LOG
                );
                kill(pid, SIGTERM);
            }
            pids.mutex.unlock_shared();

            logger::this_logger->log(
                "Waiting for modules to shut down gracefully ...",
                logger::level::LOG
            );

            /*  No mutex is needed here because `watch_count.num` is atomic, so it cannot be changed
                by a module watcher while it's being read by the while loop. Were its value to
                change right after entering the while loop such that the loop condition were no
                longer true, the lock would succeed so no harm would have been done. */
            while (watch_count.num > 0) {
                if (!watch_count.sem.wait_for(std::chrono::seconds(5))) {
                    logger::this_logger->log(
                        "There was a timeout waiting for modules to shut down gracefully, terminating.",
                        logger::level::ERR
                    );

                    /* Forcefully kill all remaining processes. */
                    pids.mutex.lock_shared();
                    for (const auto& item : pids.map) {
                        kill(item.second, SIGKILL);
                    }
                    pids.mutex.unlock_shared();
                    break;
                }
            }

            if (watch_count.num <= 0) {
                logger::this_logger->log("All modules shut down.", logger::level::LOG);
            }

            logger::this_logger->log(
                "Successfully shut down " + modules::to_string_extended(module_type) +
                    ", stop listening for messages.",
                logger::level::LOG
            );

            watch_storage->join();
            watch_collection->join();
            watch_ui->join();
            watch_inference->join();
            watch_automation->join();
            watch_training->join();

            delete watch_storage;
            delete watch_collection;
            delete watch_ui;
            delete watch_inference;
            delete watch_automation;
            delete watch_training;
        }

        /* END Shut down modules. */

        /* Delete messenger object. */
        delete messenger::this_messenger;

        /* Delete message queues. */
        delete_msg_queues();
    }

    int hems_launcher::create_msg_queues() {
        auto create = [this] (const std::map<modules::type, std::string>& mqs) {
            for (const auto& item : mqs) {
                modules::type owner = item.first;
                std::string name = item.second;

                struct mq_attr attr = { 
                    mq_flags    : 0,
                    mq_maxmsg   : 10,
                    mq_msgsize  : sizeof(messenger::msg_t),
                    mq_curmsgs  : 0
                };

                mqd_t id = mq_open(name.c_str(), O_RDWR | O_CLOEXEC | O_CREAT, 0666, &attr);
                if (id < 0) {
                    logger::this_logger->log(
                        "Could not create message queue for " + modules::to_string_extended(owner) + ": " +
                            strerror(errno),
                        logger::level::ERR
                    );
                    return 1;
                } else {
                    mq_close(id);
                }
            }
            return 0;
        };

        return (create(messenger::mq_names) | create(messenger::mq_res_names));
    }

    void hems_launcher::delete_msg_queues() {
        for (const auto& item : messenger::mq_names) {
            mq_unlink(item.second.c_str());
        }

        for (const auto& item : messenger::mq_res_names) {
            mq_unlink(item.second.c_str());
        }
    }

    void hems_launcher::watch_module(modules::type module) {
        int wait_status;
        int exit_status = EXIT_FAILURE;

        /*  There needs to be a second static mutex for the watcher count, which is used by module
            watchers to protect modifications of the variable. They can't use `watch_count.mutex`
            because they would lock up otherwise, as it is locked up by the main thread initially
            once at the end of the constructor. */
        static std::mutex wc_mutex;

        pids.mutex.lock_shared();
        pid_t pid = pids.map.at(module);
        pids.mutex.unlock_shared();

        wc_mutex.lock();
        ++watch_count.num;
        wc_mutex.unlock();

        pid_t pid_ = waitpid(pid, &wait_status, 0);
        if (pid_ == -1) {
            logger::this_logger->log(
                "Error watching process " + std::to_string(pid) + " of " +
                    modules::to_string_extended(module) + ", aborting: " + strerror(errno),
                logger::level::ERR
            );
            exit_status = EXIT_FAILURE;
        } else if (pid_ == pid) {
            exit_status = WIFEXITED(wait_status) ? WEXITSTATUS(wait_status) : EXIT_FAILURE;
            pids.mutex.lock();
            pids.map.erase(module);
            pids.mutex.unlock();
        }

        wc_mutex.lock();
        --watch_count.num;
        wc_mutex.unlock();

        watch_count.sem.notify();

        if (exit_status != EXIT_SUCCESS) {
            logger::this_logger->log(
                modules::to_string_extended(module) + " terminated with status " +
                    std::to_string(exit_status) + ", aborting.",
                logger::level::ERR
            );
        }
        hems::exit(exit_status);
    }

    void hems_launcher::init_settings(types::settings_t settings) {
        logger::this_logger->log(
            "Initializing settings: Waiting for Data Storage Module ...",
            logger::level::LOG
        );

        /*  Get the current settings from the Data Storage Module, if there are any. Broadcast them
            to all modules in that case. */
        std::string response;
        int res = messenger::this_messenger->send(
            2 * DEFAULT_SEND_TIMEOUT,
            messages::storage::msg_type::MSG_GET_SETTINGS,
            modules::STORAGE,
            messenger::this_messenger->serialize(""),
            &response
        );

        if (res == messages::storage::response_code::SUCCESS) {
            modules::current_settings = messenger::this_messenger->deserialize<types::settings_t>(response);
            logger::this_logger->log(
                "Prior settings found: " + to_string(modules::current_settings), logger::level::LOG
            );
        } else if (res == messages::storage::response_code::MSG_GET_NONE_AVAILABLE) {
            logger::this_logger->log(
                "No prior settings found.", logger::level::LOG
            );
        } else {
            logger::this_logger->log(
                "Error retrieving settings for initialization (" + std::to_string(res) + "). Terminating.",
                logger::level::ERR
            );
            exit(EXIT_FAILURE);
        }

        res = messenger::this_messenger->broadcast_settings_init(modules::current_settings);
        if (res != messenger::settings_code::SUCCESS) {
            logger::this_logger->log(
                "Error during settings initialization. Terminating.",
                logger::level::ERR
            );
            exit(EXIT_FAILURE);
        }

        logger::this_logger->log("Settings initialization finished.", logger::level::LOG);

        /* If new settings were given through command line arguments, broadcast them now. */
        if (!types::is_undefined(settings)) {
            logger::this_logger->log(
                "New settings were given through command line arguments, broadcasting them now: " +
                    types::to_string(settings),
                logger::level::LOG
            );

            res = messenger::this_messenger->broadcast_settings(settings);
            if (res != messenger::settings_code::SUCCESS) {
                logger::this_logger->log(
                    "The new settings were not accepted by all modules (code " + std::to_string(res) + "). " +
                        "Proceeding without applying new settings from command line arguments.",
                    logger::level::LOG
                );
            } else {
                logger::this_logger->log(
                    "New settings accepted by all modules: " + types::to_string(settings),
                    logger::level::LOG
                );
            }
        }
    }

}}}
