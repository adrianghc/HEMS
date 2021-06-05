/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This header declares functions and variables used by all modules.
 */

#ifndef HEMS_COMMON_MODULES_H
#define HEMS_COMMON_MODULES_H

#include <string>
#include "hems/common/types.h"

namespace hems { namespace modules {

    /**
     * @brief       Identifies a module.
     */
    enum type {
        LAUNCHER,
        AUTOMATION,
        COLLECTION,
        INFERENCE,
        STORAGE,
        TRAINING,
        UI
    };

    /**
     * @brief       Returns a simple string representation of a module.
     * @return      The string representation.
     */
    std::string to_string(type module);

    /**
     * @brief       Returns an extended string representation of a module.
     * @return      The string representation.
     */
    std::string to_string_extended(type module);

    /**
     * @brief       The current settings.
     */
    extern types::settings_t current_settings;

}}

#endif /* HEMS_COMMON_MODULES_H */
