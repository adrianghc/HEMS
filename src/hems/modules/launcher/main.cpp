/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the code for the HEMS Launcher. The HEMS Launcher is the executable that starts the
 * entire HEMS, launching all modules and initializing their message queues. The lifecycle of the
 * HEMS is identical with the launcher's lifecycle, therefore when the launcher terminates, the
 * entire HEMS does. The launcher also collects errors, outputs and log messages from all modules
 * and writes them to files and standard I/O streams.
 */

#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include "hems/modules/launcher/launcher.h"
#include "hems/modules/launcher/local_logger.h"
#include "hems/common/exit.h"
#include "hems/common/logger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"

int main(int argc, char* argv[]) {
    using namespace hems::modules::launcher;

    bool debug = false;

    double longitude            = 0;
    double latitude             = 0;
    double timezone             = 0;

    std::string log_path        = "log.txt";

    std::string storage_path    = "storage";
    std::string db_path         = "hems.db";

    std::string collection_path = "collection";

    std::string ui_path         = "ui";
    std::string ui_server_root  = "resources";

    std::string inference_path  = "inference";

    std::string automation_path = "automation";

    std::string training_path   = "training";

    std::string valid_args =
        "--help, -h             Get a list of permitted arguments.\n"
        "--debug, -d            Launch with the debug configuration.\n"
        "                       This will log and print debug messages.\n"
        "                       This will also prevent other modules from launching automatically.\n"
        "--log          [PATH]  Write log messages into a file at [PATH].\n"
                                "\n"
        "--lonlat       [VALUE] The longitude and latitude values (','-separated) to be used as the\n"
        "                       user's location.\n"
        "--timezone     [VALUE] The timezone in which the user is located (a number relative to UTC).\n";
                                "\n"
        "--storage      [PATH]  Look for the " + hems::modules::to_string_extended(hems::modules::type::STORAGE) +
                                " in [PATH] (default is '" + storage_path + "').\n"
        "--db           [PATH]  Launch with the SQLite database file at [PATH] (default is " + db_path + ").\n"
                                "\n"
        "--collection   [PATH]  Look for the " + hems::modules::to_string_extended(hems::modules::type::COLLECTION) +
                                " in [PATH] (default is '" + collection_path + "').\n"
                                "\n"
        "--ui           [PATH]  Look for the " + hems::modules::to_string_extended(hems::modules::type::UI) +
                                " in [PATH] (default is '" + ui_path + "').\n"
        "--root         [PATH]  Use [PATH] as the root directory used by the HTTP server for the user "
                                "interface (default is '" + ui_server_root + "').\n"
                                "\n"
        "--inference    [PATH]  Look for the " + hems::modules::to_string_extended(hems::modules::type::INFERENCE) +
                                " in [PATH] (default is '" + inference_path + "').\n"
                                "\n"
        "--automation   [PATH]  Look for the " + hems::modules::to_string_extended(hems::modules::type::AUTOMATION) +
                                " in [PATH] (default is '" + automation_path + "').\n"
                                "\n"
        "--training     [PATH]  Look for the " + hems::modules::to_string_extended(hems::modules::type::TRAINING) +
                                " in [PATH] (default is '" + training_path + "').\n"
                                "\n";

    for (int i = 1; i < argc; ++i) {
        std::string arg = std::string(argv[i]);
        if (arg == "--help" || arg == "-h") {
            std::cout << valid_args;
            return EXIT_FAILURE;
        } else if (arg == "--debug" || arg == "-d") {
            debug = true;
        } else if (arg == "--log") {
            if (i == argc - 1) {
                std::cout << valid_args;
                return EXIT_FAILURE;
            } else {
                log_path = std::string(argv[++i]);
            }
        } else if (arg == "--lonlat") {
            if (i == argc - 1) {
                std::cout << valid_args;
                return EXIT_FAILURE;
            } else {
                std::string arg(argv[++i]);
                std::regex regex(",");
                std::vector<std::string> strs(
                    std::sregex_token_iterator(arg.begin(), arg.end(), regex, -1),
                    std::sregex_token_iterator()
                );
                if (strs.size() != 2) {
                    std::cout << valid_args;
                    return EXIT_FAILURE;
                } else {
                    longitude = std::stof(strs.at(0), nullptr);
                    latitude = std::stof(strs.at(1), nullptr);
                }
            }
        } else if (arg == "--timezone") {
            if (i == argc - 1) {
                std::cout << valid_args;
                return EXIT_FAILURE;
            } else {
                timezone = std::stof(argv[++i]);
            }
        } else if (arg == "--storage") {
            if (i == argc - 1) {
                std::cout << valid_args;
                return EXIT_FAILURE;
            } else {
                storage_path = std::string(argv[++i]);
            }
        } else if (arg == "--db") {
            if (i == argc - 1) {
                std::cout << valid_args;
                return EXIT_FAILURE;
            } else {
                db_path = std::string(argv[++i]);
            }
        } else if (arg == "--collection") {
            if (i == argc - 1) {
                std::cout << valid_args;
                return EXIT_FAILURE;
            } else {
                collection_path = std::string(argv[++i]);
            }
        } else if (arg == "--ui") {
            if (i == argc - 1) {
                std::cout << valid_args;
                return EXIT_FAILURE;
            } else {
                ui_path = std::string(argv[++i]);
            }
        } else if (arg == "--root") {
            if (i == argc - 1) {
                std::cout << valid_args;
                return EXIT_FAILURE;
            } else {
                ui_server_root = std::string(argv[++i]);
            }
        } else if (arg == "--inference") {
            if (i == argc - 1) {
                std::cout << valid_args;
                return EXIT_FAILURE;
            } else {
                inference_path = std::string(argv[++i]);
            }
        } else if (arg == "--automation") {
            if (i == argc - 1) {
                std::cout << valid_args;
                return EXIT_FAILURE;
            } else {
                automation_path = std::string(argv[++i]);
            }
        } else if (arg == "--training") {
            if (i == argc - 1) {
                std::cout << valid_args;
                return EXIT_FAILURE;
            } else {
                training_path = std::string(argv[++i]);
            }
        } else {
            std::cout << "Invalid usage, permitted arguments are:\n\n" + valid_args;
            return EXIT_FAILURE;
        }
    }

    hems::logger::this_logger = new local_logger(hems_launcher::module_type, debug, log_path);

    /* Register signal handlers to ensure that destructors are called before termination. */
    signal(SIGTERM, hems::signal_handler);
    signal(SIGQUIT, hems::signal_handler);
    signal(SIGINT, hems::signal_handler);

    try {
        hems::types::settings_t settings = {
            longitude   : longitude,
            latitude    : latitude,
            timezone    : timezone
        };
        hems_launcher::this_instance = new hems_launcher(
            debug, false, storage_path, db_path, collection_path, ui_path, ui_server_root,
            inference_path, automation_path, training_path, settings
        );

        /*  The main thread remains blocked until the exit semaphore is notified through a call to
            the `exit()` function, whereupon the thread leaves this block and the program terminates. */
        hems::exit_sem.wait();
    } catch (int err) {
        hems::exit_status = err;
    }

    delete hems_launcher::this_instance;

    hems::logger::this_logger->log(
        "Terminating with status " + std::to_string(hems::exit_status) + ".",
        hems::logger::level::LOG
    );

    delete hems::logger::this_logger;
    return hems::exit_status;
}
