/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This contains code for functions and variables used by all modules.
 */

#include <string>
#include "hems/common/modules.h"

namespace hems { namespace modules {

    std::string to_string(modules::type module) {
        switch (module) {
            case type::LAUNCHER: return "LAUNCHER";
            case type::AUTOMATION: return "AUTOMATION";
            case type::COLLECTION: return "COLLECTION";
            case type::INFERENCE: return "INFERENCE";
            case type::STORAGE: return "STORAGE";
            case type::TRAINING: return "TRAINING";
            case type::UI: return "UI";
            default: return "UNKNOWN";
        }
    };

    std::string to_string_extended(modules::type module) {
        switch (module) {
            case type::LAUNCHER: return "HEMS Launcher";
            case type::AUTOMATION: return "Automation and Recommendation Module";
            case type::COLLECTION: return "Measurement Collection Module";
            case type::INFERENCE: return "Knowledge Inference Module";
            case type::STORAGE: return "Data Storage Module";
            case type::TRAINING: return "Model Training Module";
            case type::UI: return "User Interface Module";
            default: return "Unknown module";
        }
    };

    types::settings_t current_settings;

}}
