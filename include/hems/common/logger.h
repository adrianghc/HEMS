/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This header declares the abstract `logger` class and the `remote_logger` implementation.
 * These classes collects errors, outputs and log messages from all modules and write them to files
 * and standard I/O streams.
 */

#ifndef HEMS_COMMON_LOGGER_H
#define HEMS_COMMON_LOGGER_H

#include <string>
#include "hems/common/modules.h"

namespace hems {

    /**
     * @brief   The abstract `logger` class.
     *          This class offers log levels and declares a virtual `log` method that can be called
     *          regardless of implementation.
     */
    class logger {

        public:
            /**
             * @brief       Constructs a logger.
             * 
             * @param[in]   owner       The owner of this instance.
             * @param[in]   debug       Whether to print and log debug messages or not.
             */
            logger(modules::type owner, bool debug) : owner(owner), debug(debug) {};

            virtual ~logger() {};

            /**
             * @brief       Identifies log levels for log messages, debug messages and errors.
             */
            enum level { LOG, DBG, ERR };

            /**
             * @brief       Returns a simple string representation of a module.
             * @return      The string representation.
             */
            static std::string to_string(level level_);

            /**
             * @brief       Logs a message from a given source and a given level.
             * 
             * @param[in]   msg         The log message.
             * @param[in]   log_level   The log level.
             */
            virtual void log(std::string msg, level log_level) = 0;

            static logger* this_logger; /** Instance of the `logger` class. */

        protected:
            modules::type owner;  /** The owner of this instance. */

            bool debug; /** Whether to print and log debug messages or not. */

    };

    /**
     * @brief   The `remote_logger` class.
     *          This class sends errors, outputs and log messages to the HEMS Launcher Module. It is
     *          used by all modules except the launcher module.
     */
    class remote_logger : public logger {

        public:
            /**
             * @brief       Constructs a remote logger.
             * 
             * @param[in]   owner       The owner of this instance.
             * @param[in]   debug       Whether to print and log debug messages or not.
             */
            remote_logger(modules::type owner, bool debug) : logger(owner, debug) {};

            /**
             * @brief       Logs a message from a given source and a given level.
             * 
             * @param[in]   msg         The log message.
             * @param[in]   log_level   The log level.
             */
            void log(std::string msg, level log_level) override;

    };

}

#endif /* HEMS_COMMON_LOGGER_H */
