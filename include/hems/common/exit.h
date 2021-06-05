/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This header declares functions and variables for signal handling and termination.
 */

#ifndef HEMS_COMMON_EXIT_H
#define HEMS_COMMON_EXIT_H

#include <csignal>
#include <mutex>
#include "extras/semaphore.hpp"

namespace hems {

    extern semaphore exit_sem;  /** A mutex that blocks the main thread until it is to shut down. */

    extern volatile sig_atomic_t exit_status;   /** The status to terminate with. */

    /**
     * @brief       Signal handler for SIGTERM, SIGINT and SIGQUIT events.
     * 
     * @param[in]   signal  The signal type.
     */
    void signal_handler(int signal);

    /**
     * @brief       Exits with a given status.
     * 
     * @param[in]   status  The exit status.
     */
    void exit(int status);

}

#endif /* HEMS_COMMON_EXIT_H */
