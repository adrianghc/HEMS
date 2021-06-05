/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the User Interface Module.
 * This module is responsible for offering an interface for the user to interact with the HEMS,
 * presenting the current state and offering commands.
 */

#include <iostream>
#include <string>
#include <vector>

#include "hems/modules/ui/ui.h"
#include "hems/common/logger.h"
#include "hems/common/exit.h"

int main(int argc, char* argv[]) {
    using namespace hems::modules::ui;

    bool debug = false;

    std::string ui_server_root = "resources";

    std::string valid_args =
        "--help, -h         Get a list of permitted arguments.\n"
        "--debug, -d        Launch with the debug configuration.\n"
        "                   This will log and print debug messages.\n"
        "--root [PATH]      Use [PATH] as the root directory used by the HTTP server for the user "
                            "interface (default is '" + ui_server_root + "').\n";

    for (int i = 1; i < argc; ++i) {
        std::string arg = std::string(argv[i]);
        if (arg == "--help" || arg == "-h") {
            std::cout << valid_args;
            return EXIT_FAILURE;
        } else if (arg == "--debug" || arg == "-d") {
            debug = true;
        } else if (arg == "--root") {
            if (i == argc - 1) {
                std::cout << valid_args;
                return EXIT_FAILURE;
            } else {
                ui_server_root = std::string(argv[++i]);
            }
        } else {
            std::cout << "Invalid usage, permitted arguments are:\n\n" + valid_args;
            return EXIT_FAILURE;
        }
    }

    hems::logger::this_logger = new hems::remote_logger(hems_ui::module_type, debug);

    /* Register signal handlers to ensure that destructors are called before termination. */
    signal(SIGTERM, hems::signal_handler);
    signal(SIGQUIT, hems::signal_handler);
    signal(SIGINT, hems::signal_handler);

    try {
        hems_ui::this_instance = new hems_ui(false, ui_server_root);

        /*  The main thread remains blocked until the exit semaphore is notified through a call to
            the `exit()` function, whereupon the thread leaves this block and the program terminates. */
        hems::exit_sem.wait();
    } catch (int err) {
        hems::logger::this_logger->log(
            "FATAL: Could not start instance of " +
                hems::modules::to_string_extended(hems_ui::module_type) + ".",
            hems::logger::level::ERR
        );
        hems::exit(err);
    }

    delete hems_ui::this_instance;
    delete hems::logger::this_logger;
    return hems::exit_status;
}
