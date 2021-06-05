/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the Automation and Recommendation Module.
 * This module is responsible for providing recommendations for task scheduling and automating
 * appliances.
 */

#include <algorithm>
#include <list>
#include <sstream>
#include <string>

#include <boost/date_time/posix_time/posix_time.hpp>

#include "hems/modules/automation/automation.h"
#include "hems/common/logger.h"
#include "hems/common/messenger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"
#include "hems/common/exit.h"
#include "hems/messages/automation.h"
#include "hems/messages/inference.h"
#include "hems/messages/storage.h"

namespace hems { namespace modules { namespace automation {

    using namespace hems::messages::automation;

    using boost::posix_time::ptime;


    bool sort_appliances_by_width(appliance_t appliance1, appliance_t appliance2) {
        return (appliance1.duty_cycle < appliance2.duty_cycle);
    }

    bool sort_appliances_by_height(appliance_t appliance1, appliance_t appliance2) {
        return (appliance1.rating < appliance2.rating);
    }

    bool sort_appliances_by_area(appliance_t appliance1, appliance_t appliance2) {
        return (appliance1.duty_cycle * appliance1.rating < appliance2.duty_cycle * appliance2.rating);
    }


    void allocate_first_fit(
        std::vector<appliance_t>& appliances, std::map<ptime, types::energy_production_t>& energy,
        std::list<types::task_t>& recommendations
    ) {
        for (const auto& appliance : appliances) {
            for (auto i=0; i<appliance.schedules_per_week; ++i) {
                ptime first_time, last_time;

                /* Check if there is a fit and if so, save the first one. */
                for (const auto& time_and_energy : energy) {
                    if (time_and_energy.second.energy < appliance.rating) {
                        first_time = boost::posix_time::not_a_date_time;
                    } else {
                        if (first_time.is_not_a_date_time()) {
                            first_time = time_and_energy.first;
                        }
                        if (time_and_energy.first - first_time == boost::posix_time::hours(appliance.duty_cycle - 1)) {
                            last_time = time_and_energy.first;
                            break;
                        }
                    }
                }

                /* If there is a fit, create a task and subtract its energy from the step function. */
                if (!first_time.is_not_a_date_time() && !last_time.is_not_a_date_time()) {
                    std::map<ptime, types::energy_production_t>::iterator it_low, it_up;
                    it_low = energy.lower_bound(first_time);
                    it_up = energy.lower_bound(last_time);
                    for (std::map<ptime, types::energy_production_t>::iterator it=it_low; it!=it_up; ++it) {
                        it->second.energy -= appliance.rating;
                    }

                    task_t task = {
                        id                  : 0,
                        name                : "",
                        start_time          : first_time,
                        end_time            : last_time,
                        auto_profile        : 0,
                        is_user_declared    : false,
                        appliances          : { appliance.id }
                    };
                    recommendations.emplace_back(task);
                }
            }
        }
    }

    void allocate_next_fit(
        std::vector<appliance_t>& appliances, std::map<ptime, types::energy_production_t>& energy,
        std::list<types::task_t>& recommendations
    ) {
        for (const auto& appliance : appliances) {
            ptime it_begin = energy.begin()->first;
            for (auto i=0; i<appliance.schedules_per_week; ++i) {
                ptime first_time, last_time;

                /*  Check if there is a fit and if so, save the next one (starting at the position
                    of the last allocation). */
                std::map<ptime, types::energy_production_t>::iterator it_low1, it_up1;
                it_low1 = energy.lower_bound(it_begin);
                it_up1 = energy.end();

                for (std::map<ptime, types::energy_production_t>::iterator it=it_low1; it!=it_up1; ++it) {
                    if (it->second.energy < appliance.rating) {
                        first_time = boost::posix_time::not_a_date_time;
                    } else {
                        if (first_time.is_not_a_date_time()) {
                            first_time = it->first;
                        }
                        if (it->first - first_time == boost::posix_time::hours(appliance.duty_cycle - 1)) {
                            last_time = it->first;
                            break;
                        }
                    }
                }

                /* Start looping the step function from the beginning if no fit was found. */
                if (first_time.is_not_a_date_time() || last_time.is_not_a_date_time()) {
                    first_time = boost::posix_time::not_a_date_time;
                    last_time = boost::posix_time::not_a_date_time;

                    std::map<ptime, types::energy_production_t>::iterator it_low2, it_up2;
                    it_low2 = energy.begin();
                    it_up2 = energy.upper_bound(it_begin);

                    for (std::map<ptime, types::energy_production_t>::iterator it=it_low2; it!=it_up2; ++it) {
                        if (it->second.energy < appliance.rating) {
                            first_time = boost::posix_time::not_a_date_time;
                        } else {
                            if (first_time.is_not_a_date_time()) {
                                first_time = it->first;
                            }
                            if (it->first - first_time == boost::posix_time::hours(appliance.duty_cycle - 1)) {
                                last_time = it->first;
                                break;
                            }
                        }
                    }
                }


                /* If there is a fit, create a task and subtract its energy from the step function. */
                if (!first_time.is_not_a_date_time() && !last_time.is_not_a_date_time()) {
                    std::map<ptime, types::energy_production_t>::iterator it_low, it_up;
                    it_low = energy.lower_bound(first_time);
                    it_up = energy.lower_bound(last_time);
                    for (std::map<ptime, types::energy_production_t>::iterator it=it_low; it!=it_up; ++it) {
                        it->second.energy -= appliance.rating;
                    }

                    task_t task = {
                        id                  : 0,
                        name                : "",
                        start_time          : first_time,
                        end_time            : last_time,
                        auto_profile        : 0,
                        is_user_declared    : false,
                        appliances          : { appliance.id }
                    };
                    recommendations.emplace_back(task);

                    if (last_time == energy.end()->first) {
                        /*  If allocation happened up to the end of the step function, continue
                            iterating from the beginning of the step function. */
                        it_begin = energy.begin()->first;
                    } else {
                        /* Else, continue iterating from one hour after the last allocation. */
                        it_begin = last_time + boost::posix_time::hours(1);
                    }
                }
            }
        }
    }

    void allocate_best_fit(
        std::vector<appliance_t>& appliances, std::map<ptime, types::energy_production_t>& energy,
        std::list<types::task_t>& recommendations
    ) {
        for (const auto& appliance : appliances) {
            for (auto i=0; i<appliance.schedules_per_week; ++i) {
                ptime best_pos;
                double max_area = 0;
                double cur_area = 0;

                ptime first_time, last_time;

                /* Check if there is a fit and if so, save the first one. */
                for (const auto& time_and_energy : energy) {
                    if (time_and_energy.second.energy < appliance.rating) {
                        cur_area = 0;
                        first_time = boost::posix_time::not_a_date_time;
                    } else {
                        if (first_time.is_not_a_date_time()) {
                            cur_area += time_and_energy.second.energy - appliance.rating;
                            first_time = time_and_energy.first;
                        }
                        if (time_and_energy.first - first_time < boost::posix_time::hours(appliance.duty_cycle - 1)) {
                            cur_area += time_and_energy.second.energy - appliance.rating;
                        } else if (time_and_energy.first - first_time == boost::posix_time::hours(appliance.duty_cycle - 1)) {
                            cur_area += time_and_energy.second.energy - appliance.rating;
                            last_time = time_and_energy.first;

                            /* If this is a better fit than the previous one, save it. */
                            if (cur_area > max_area) {
                                max_area = cur_area;
                                best_pos = first_time;
                            }

                            break;
                        }
                    }
                }

                /* Allocate the best fit. */
                std::map<ptime, types::energy_production_t>::iterator it_low, it_up;
                it_low = energy.lower_bound(best_pos);
                it_up = energy.upper_bound(best_pos + boost::posix_time::hours(appliance.duty_cycle));
                for (std::map<ptime, types::energy_production_t>::iterator it=it_low; it!=it_up; ++it) {
                    it->second.energy -= appliance.rating;
                }

                task_t task = {
                    id                  : 0,
                    name                : "",
                    start_time          : best_pos,
                    end_time            : best_pos + boost::posix_time::hours(appliance.duty_cycle),
                    auto_profile        : 0,
                    is_user_declared    : false,
                    appliances          : { appliance.id }
                };
                recommendations.emplace_back(task);
            }
        }
    }


    int hems_automation::get_recommendations(
        ptime start_time, order rect_order, heuristic alloc_heuristic,
        std::list<types::task_t>& recommendations
    ) {
        /* Check that time is within the interval. */
        auto interval = current_settings.interval_energy_production;
        if (start_time.time_of_day().fractional_seconds() || start_time.time_of_day().seconds() ||
            start_time.time_of_day().minutes() % interval) {
            logger::this_logger->log(
                "Invalid value provided for start_time: Must be a multiple of " +
                    std::to_string(interval) + " full minutes but was " +
                    boost::posix_time::to_simple_string(start_time) + ".",
                logger::level::ERR
            );
            return response_code::INVALID_DATA;
        }

        int code;
        std::vector<appliance_t> appliances;
        std::map<ptime, types::energy_production_t> energy;


        /* Get all appliances. */

        messages::storage::msg_get_appliances_all_request appliances_req = {
            is_schedulable : messages::storage::tribool::TRUE
        };

        std::string appliances_res;

        code = messenger::this_messenger->send(
            DEFAULT_SEND_TIMEOUT,
            messages::storage::MSG_GET_APPLIANCES_ALL,
            modules::STORAGE,
            messenger::serialize(appliances_req),
            &appliances_res
        );

        if (code) {
            logger::this_logger->log(
                "Error " + std::to_string(code) + " getting appliances.",
                logger::level::ERR
            );
            return response_code::DATA_STORAGE_MODULE_ERR;
        } else {
            messages::storage::msg_get_appliances_all_response response =
                messenger::deserialize<messages::storage::msg_get_appliances_all_response>(appliances_res);
            appliances = response.appliances;
        }


        /* Get energy production predictions. */

        messages::inference::msg_get_predictions_request energy_req = {
            start_time : start_time
        };

        std::string energy_res;

        code = messenger::this_messenger->send(
            3 * DEFAULT_SEND_TIMEOUT, // Use a bigger timeout here to account for connection issues.
            messages::inference::MSG_GET_PREDICTIONS,
            modules::INFERENCE,
            messenger::serialize(energy_req),
            &energy_res
        );

        if (code) {
            logger::this_logger->log(
                "Error " + std::to_string(code) + " getting energy production predictions for the " +
                    "week starting at " + boost::posix_time::to_simple_string(start_time),
                logger::level::ERR
            );
            return response_code::DATA_STORAGE_MODULE_ERR;
        } else {
            messages::inference::msg_get_predictions_response response =
                messenger::deserialize<messages::inference::msg_get_predictions_response>(energy_res);
            energy = response.energy;
        }


        /* Sort appliances. */
        switch (rect_order) {
            case WIDTH_DESC:
                std::sort(appliances.begin(), appliances.end(), sort_appliances_by_width);
                break;
            case HEIGHT_DESC:
                std::sort(appliances.begin(), appliances.end(), sort_appliances_by_height);
                break;
            case AREA_DESC:
                std::sort(appliances.begin(), appliances.end(), sort_appliances_by_area);
                break;
        }

        /* Allocate tasks. */
        switch (alloc_heuristic) {
            case FIRST_FIT:
                allocate_first_fit(appliances, energy, recommendations);
                break;
            case NEXT_FIT:
                allocate_next_fit(appliances, energy, recommendations);
                break;
            case BEST_FIT:
                allocate_best_fit(appliances, energy, recommendations);
                break;
        }

        return response_code::SUCCESS;
    }

}}}
