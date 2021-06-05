/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the code for the `local_logger` implementation of the abstract `logger` class.
 * This class collects errors, outputs and log messages from all modules and writes them to files
 * and standard I/O streams.
 */

#include <fstream>
#include <iostream>
#include <locale>
#include <map>
#include <sstream>
#include <string>
#include <cstring>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

#include "hems/modules/launcher/launcher.h"
#include "hems/modules/launcher/local_logger.h"

namespace hems { namespace modules { namespace launcher {

    local_logger::local_logger(modules::type owner, bool debug, std::string log_path) : logger(owner, debug) {
        log_file.open(log_path, std::ofstream::app);
        if (log_file.fail()) {
            log(
                "Could not open or create " + log_path + ", log messages will only be printed, not written.",
                level::ERR
            );
        }
    };

    local_logger::~local_logger() {
        if (log_file.is_open()) {
            log_file.close();
        }
    };

    void local_logger::log(std::string msg, level log_level) {
        log(msg, log_level, owner);
    }

    void local_logger::log(std::string msg, level log_level, modules::type src) {
        if (!debug && log_level == level::DBG) {
            return;
        }

        std::string final_msg;
        #ifdef COLOR
        std::string final_msg_color;
        std::string begin_color = "\033[38;5;";
        std::string end_color = "\033[0m";
        #endif

        /* Get the current time. */
        boost::posix_time::ptime cur_time = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::time_facet* facet = new boost::posix_time::time_facet();
        facet->format("%Y-%m-%d %H:%M:%S%F");

        std::stringstream time_stream;
        time_stream.imbue(std::locale(std::locale::classic(), facet));
        time_stream << cur_time;

        /* Append the time. */
        final_msg.append("[").append(time_stream.str()).append("] ");
        #ifdef COLOR
        /* Green for the time. */
        final_msg_color
            .append("[").append(begin_color).append("34m")
            .append(time_stream.str()).append(end_color).append("] ");
        #endif

        /* Append the log level. */
        final_msg.append("[").append(logger::to_string(log_level)).append("] ");
        #ifdef COLOR
        switch (log_level) {
            case level::LOG:
                /* Blue for log messages. */
                final_msg_color.append("[").append(begin_color).append("32m");
                break;
            case level::ERR:
                /* Red for error messages. */
                final_msg_color.append("[").append(begin_color).append("160m");
                break;
            case level::DBG:
                /* Yellow for debug messages. */
                final_msg_color.append("[").append(begin_color).append("220m");
                break;
        }
        final_msg_color.append(logger::to_string(log_level)).append(end_color).append("] ");
        #endif
        int cur_size_log = logger::to_string(log_level).size();
        if (cur_size_log < log_strings_maxlen) {
            final_msg.append(std::string(log_strings_maxlen - cur_size_log, ' '));
            #ifdef COLOR
                final_msg_color.append(std::string(log_strings_maxlen - cur_size_log, ' '));
            #endif
        }

        /* Append the module name. */
        final_msg.append("[").append(modules::to_string(src)).append("] ");
        #ifdef COLOR
        switch (src) {
            case modules::type::LAUNCHER:
                /* Teal for the HEMS Launcher. */
                final_msg_color.append("[").append(begin_color).append("43m");
                break;
            case modules::type::STORAGE:
                /* Violet for the Data Storage Module. */
                final_msg_color.append("[").append(begin_color).append("105m");
                break;
            case modules::type::COLLECTION:
                /* Yellow orange for the Data Storage Module. */
                final_msg_color.append("[").append(begin_color).append("214m");
                break;
            case modules::type::UI:
                /* Light blue for the User Interface Module. */
                final_msg_color.append("[").append(begin_color).append("51m");
                break;
            case modules::type::INFERENCE:
                /* Rose for the Knowledge Inference Module. */
                final_msg_color.append("[").append(begin_color).append("211m");
                break;
            case modules::type::AUTOMATION:
                /* Bright green for the Automation and Recommendation Module. */
                final_msg_color.append("[").append(begin_color).append("118m");
                break;
            case modules::type::TRAINING:
                /* Medium blue for the Model Training Module. */
                final_msg_color.append("[").append(begin_color).append("45m");
                break;
            default:
                /* TODO Add distinct colors for all remaining modules. */
                final_msg_color.append("[").append(begin_color).append("255m");
                break;
        }
        final_msg_color.append(modules::to_string(src)).append(end_color).append("] ");
        #endif
        int cur_size_src = modules::to_string(src).size();
        if (cur_size_src < source_strings_maxlen) {
            final_msg.append(std::string(source_strings_maxlen - cur_size_src, ' '));
            #ifdef COLOR
                final_msg_color.append(std::string(source_strings_maxlen - cur_size_src, ' '));
            #endif
        }

        /* Append the message. */
        final_msg.append(msg).append("\n");
        #ifdef COLOR
        switch (log_level) {
            case level::ERR:
                /* Red for error messages. */
                final_msg_color.append(begin_color).append("160m").append(msg).append(end_color).append("\n");
                break;
            default:
                final_msg_color.append(msg).append("\n");
                break;
        }
        #endif

        /* Log and print. */
        if (log_file.is_open()) {
            log_file << final_msg;
            log_file.flush();
        }

        #ifndef COLOR
        log_streams.at(log_level) << final_msg;
        #endif
        #ifdef COLOR
        log_streams.at(log_level) << final_msg_color;
        #endif
    };

    const std::map<logger::level, std::ostream&> local_logger::log_streams = {
        { logger::level::LOG, std::cout },
        { logger::level::DBG, std::cout },
        { logger::level::ERR, std::cerr }
    };

}}}
