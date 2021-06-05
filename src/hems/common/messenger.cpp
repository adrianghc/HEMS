/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This is the code for the `messenger` class.
 * This class creates a thread to listen for new messages in a given message queue, and contains
 * methods to send messages to other message queues. It uses callback functions (called message
 * handlers) for different message types.
 */

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <thread>
#include <tuple>
#include <cstdlib>
#include <csignal>
#include <cstring>
#include <ctime>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "extras/semaphore.hpp"

#include "hems/common/messenger.h"
#include "hems/common/logger.h"
#include "hems/common/modules.h"
#include "hems/common/types.h"

namespace hems {

    using boost::archive::text_iarchive;
    using boost::archive::text_oarchive;

    using boost::interprocess::read_only;
    using boost::interprocess::read_write;
    using boost::interprocess::open_only;
    using boost::interprocess::open_or_create;
    using boost::interprocess::shared_memory_object;
    using boost::interprocess::mapped_region;


    messenger::messenger(modules::type owner) : messenger(owner, false) {};

    messenger::messenger(modules::type owner, bool test_mode) :
        owner(owner), test_mode(test_mode), mq_ids(), listener(nullptr) {
        srand(time(NULL) * 2 + owner * 7);
    };

    messenger::~messenger() {
        if (listener != nullptr) {
            send_(0, special_subtype::END_LISTEN_LOOP, owner, "", nullptr);
            listener->join();
            for (const auto& mq_id : mq_ids) {
                mq_close(mq_id.second);
            }

            send_response(0, 0, owner, "");
            listener_res->join();
            for (const auto& mq_id : mq_res_ids) {
                mq_close(mq_id.second);
            }

            logger::this_logger->log("Stopped listening for messages.", logger::level::LOG);
            delete listener;
            delete listener_res;
        }
    }

    messenger* messenger::this_messenger = nullptr;

    const std::map<modules::type, std::string> messenger::messenger::mq_names = {
        { modules::type::LAUNCHER, "/hems_mq_launcher" },
        { modules::type::AUTOMATION, "/hems_mq_automation" },
        { modules::type::COLLECTION, "/hems_mq_collection" },
        { modules::type::INFERENCE, "/hems_mq_inference" },
        { modules::type::STORAGE, "/hems_mq_storage" },
        { modules::type::TRAINING, "/hems_mq_training" },
        { modules::type::UI, "/hems_mq_ui" }
    };

    const std::map<modules::type, std::string> messenger::messenger::mq_res_names = {
        { modules::type::LAUNCHER, "/hems_mq_res_launcher" },
        { modules::type::AUTOMATION, "/hems_mq_res_automation" },
        { modules::type::COLLECTION, "/hems_mq_res_collection" },
        { modules::type::INFERENCE, "/hems_mq_res_inference" },
        { modules::type::STORAGE, "/hems_mq_res_storage" },
        { modules::type::TRAINING, "/hems_mq_res_training" },
        { modules::type::UI, "/hems_mq_res_ui" }
    };


    bool messenger::listen(const msg_handler_map& handler_map) {
        return listen(handler_map, std::vector<int>{});
    }

    bool messenger::listen(const msg_handler_map& handler_map, const std::vector<int> pre_init_whitelist) {
        if (!test_mode) {
            if (handler_map.find(special_subtype::SETTINGS_INIT) == handler_map.end() ||
                handler_map.find(special_subtype::SETTINGS_CHECK) == handler_map.end() ||
                handler_map.find(special_subtype::SETTINGS_COMMIT) == handler_map.end()) {
                logger::this_logger->log(
                    "Message handlers not provided for all settings message subtypes.",
                    logger::level::ERR
                );
                return false;
            }
        }

        auto open_queue = [this] (
            const std::map<modules::type, std::string>& names, std::map<modules::type, mqd_t>& ids
        ) {
            mqd_t listen_mq_id = mq_open(names.at(owner).c_str(), O_RDWR | O_CLOEXEC);
            if (listen_mq_id < 0) {
                logger::this_logger->log(
                    "Could not open message queue for the messenger's owner, " +
                        modules::to_string_extended(owner) + ": " + strerror(errno),
                    logger::level::ERR
                );
                return false;
            } else {
                ids[owner] = listen_mq_id;
                return true;
            }
        };

        /* Open message queue to listen for request and command messages. */
        if (!open_queue(mq_names, mq_ids)) {
            return false;
        }

        /* Open message queue to listen for response messages. */
        if (!open_queue(mq_res_names, mq_res_ids)) {
            return false;
        }

        listener = new std::thread(&messenger::listen_loop, this, handler_map, pre_init_whitelist);
        listener_res = new std::thread(&messenger::listen_loop_res, this);

        return true;
    }

    void messenger::start_handlers() {
        {
            std::lock_guard<std::mutex> lk(sh_m);
            do_start_handlers = true;
        }
        sh_cv.notify_all();
    }

    void messenger::listen_loop(const msg_handler_map& handler_map, const std::vector<int> pre_init_whitelist) {
        /*  If the listen loop starts too fast, there is a risk of receiving a message before the
            module constructor finished. */
        {
            std::unique_lock<std::mutex> lk(sh_m);
            sh_cv.wait(lk, [this]{ return do_start_handlers; });
        }

        size_t buf_size = sizeof(msg_t);

        /*  The thread listens for new messages in an infinite loop. It is terminated only when the
            entire program shuts down. */
        while (true) {
            std::map<unsigned int, std::thread*> rcv_cmd_threads;   /** This map tracks threads that
                                                                        are handling received
                                                                        command messages. */
            bool shutdown = false;  /** Shutdown mode means that the listen loop should break as
                                        soon as no threads handling received command messages are
                                        still running. */

            char msg_buf[buf_size];
            memset(msg_buf, 0x00, buf_size);

            /* Block until a message is received in the queue. */
            ssize_t msg_size = mq_receive(mq_ids.at(owner), msg_buf, buf_size, nullptr);

            if (msg_size < 0) {
                logger::this_logger->log(
                    "Error receiving message, skipping: " + std::string(strerror(errno)),
                    logger::level::DBG
                );
                continue;
            }

            msg_t* msg = reinterpret_cast<msg_t*>(msg_buf);

            if (msg->subtype == special_subtype::END_LISTEN_LOOP && !rcv_cmd_threads.size()) {
                /* This special message subtype signals the end of the listen loop. */
                logger::this_logger->log(
                    "Received message with special subtype, breaking listen loop.",
                    logger::level::DBG
                );

                /* Delete the shared segment to avoid leaks. */
                shared_memory_object shm;
                try {
                    shm = shared_memory_object(open_only, msg->shared_segment, read_only);
                    mapped_region region(shm, read_only);
                    shared_memory_object::remove(msg->shared_segment);
                } catch (const boost::interprocess::interprocess_exception& e) {};

                break;
            } else if (msg->subtype == special_subtype::END_LISTEN_LOOP && rcv_cmd_threads.size()) {
                logger::this_logger->log(
                    "Received message with special subtype, but waiting for threads handling "
                        "received command messages to finish before breaking listen loop.",
                    logger::level::DBG
                );
                shutdown = true;

                /* Delete the shared segment to avoid leaks. */
                shared_memory_object shm;
                try {
                    shm = shared_memory_object(open_only, msg->shared_segment, read_only);
                    mapped_region region(shm, read_only);
                    shared_memory_object::remove(msg->shared_segment);
                } catch (const boost::interprocess::interprocess_exception& e) {};

                continue;
            } else if (msg->subtype == special_subtype::JOIN_RCV_CMD) {
                /*  This special subtype notifies that a thread handling a received command message
                    has finished. */

                shared_memory_object shm;
                try {
                    shm = shared_memory_object(open_only, msg->shared_segment, read_only);
                } catch (const boost::interprocess::interprocess_exception& e) {
                    continue;
                };
                mapped_region region(shm, read_only);

                std::string stream_str = std::string(reinterpret_cast<const char*>(region.get_address()));
                std::istringstream istream(stream_str);
                text_iarchive ia(istream);

                logger::this_logger->log(
                    "A thread handling a received command message has finished.",
                    logger::level::DBG
                );

                unsigned int id;
                ia >> id;

                if (rcv_cmd_threads.find(id) != rcv_cmd_threads.end()) {
                    std::thread* thread = rcv_cmd_threads.at(id);
                    thread->join();
                    delete thread;
                    rcv_cmd_threads.erase(id);
                }

                shared_memory_object::remove(msg->shared_segment);

                /*  If this was the last thread and messenger is in shutdown mode, break the listen
                    loop. */
                if (shutdown && !rcv_cmd_threads.size()) {
                    break;
                } else {
                    continue;
                }
            } else if (shutdown) {
                /* Delete the shared segment to avoid leaks. */
                shared_memory_object shm;
                try {
                    shm = shared_memory_object(open_only, msg->shared_segment, read_only);
                    mapped_region region(shm, read_only);
                    shared_memory_object::remove(msg->shared_segment);
                } catch (const boost::interprocess::interprocess_exception& e) {};

                /*  When in shutdown mode, ignore all messages except for those signaling that a
                    thread handling a received command message has finished. */
                continue;
            }

            std::string msg_type_str;

            if (msg->type == msg_type::COMMAND) {
                msg_type_str = "COMMAND";
            } else if (msg->type == msg_type::REQUEST) {
                msg_type_str = "REQUEST";
            } else if (msg->type == msg_type::RESPONSE) {
                logger::this_logger->log(
                    "Received a response message in the command/request message queue.",
                    logger::level::ERR
                );
                continue;
            } else {
                logger::this_logger->log(
                    "Unknown message type " + std::to_string(msg->type) + ", skipping.",
                    logger::level::DBG
                );
                continue;
            }

            logger::this_logger->log(
                "Received a message from " + modules::to_string_extended(msg->sender)
                    + " (Type " + msg_type_str
                    + ", subtype " + std::to_string(msg->subtype)
                    + ", id " + std::to_string(msg->id)
                    + ").",
                logger::level::DBG
            );

            /*  In test mode, the messenger can handle messages without requiring the settings
                initialization. The HEMS Launcher always skips the settings initialization. */
            if (!test_mode && owner != modules::type::LAUNCHER &&
                !settings_initialized && msg->subtype != special_subtype::SETTINGS_INIT &&
                std::find(pre_init_whitelist.begin(), pre_init_whitelist.end(), msg->subtype) == pre_init_whitelist.end()) {
                logger::this_logger->log(
                    "Received a message before settings were initialized, skipping.",
                    logger::level::DBG
                );
                continue;
            } else if (settings_initialized && msg->subtype == special_subtype::SETTINGS_INIT) {
                logger::this_logger->log(
                    "Settings were already initialized, skipping,",
                    logger::level::DBG
                );
                continue;
            }

            /* Handle message by type. */
            switch (msg->type) {
                case msg_type::COMMAND: {
                    msg_t msg_copy = *(msg);
                    rcv_cmd_threads.emplace(
                        msg->id,
                        new std::thread(&messenger::receive_command, this, handler_map, msg_copy)
                    );
                    break;
                }
                case msg_type::REQUEST: {
                    receive_request(handler_map, *(msg));
                    break;
                }
                default: break;
            }
        }
    }

    void messenger::listen_loop_res() {
        size_t buf_size = sizeof(msg_t);

        /*  The thread listens for new messages in an infinite loop. It is terminated only when the
            entire program shuts down. */
        while (true) {
            char msg_buf[buf_size];
            memset(msg_buf, 0x00, buf_size);

            /* Block until a message is received in the queue. */
            ssize_t msg_size = mq_receive(mq_res_ids.at(owner), msg_buf, buf_size, nullptr);

            if (msg_size < 0) {
                logger::this_logger->log(
                    "Error receiving message, skipping: " + std::string(strerror(errno)),
                    logger::level::DBG
                );
                continue;
            }

            msg_t* msg = reinterpret_cast<msg_t*>(msg_buf);

            /* This special message id signals the end of the listen loop. */
            if (!(msg->id)) {
                logger::this_logger->log(
                    "Received response with special id, breaking listen loop.",
                    logger::level::DBG
                );
                break;
            }

            std::string msg_type_str;
            if (msg->type == msg_type::RESPONSE) {
                msg_type_str = "RESPONSE";
            } else {
                logger::this_logger->log(
                    "Received a non-response message in the response message queue.",
                    logger::level::ERR
                );
                continue;
            }

            logger::this_logger->log(
                "Received a message from " + modules::to_string_extended(msg->sender)
                    + " (Type " + msg_type_str
                    + ", code " + std::to_string(msg->code)
                    + ", id " + std::to_string(msg->id)
                    + ").",
                logger::level::DBG
            );

            /* Release the sender that was waiting for this response. */
            get_or_put_response(response_action::NOTIFY, msg->id, msg->shared_segment, &msg->code, 0);
        }
    }

    void messenger::receive_command(const msg_handler_map& handler_map, msg_t msg) {
        /* Map the shared segment into memory. */
        shared_memory_object shm;
        try {
            shm = shared_memory_object(open_only, msg.shared_segment, read_only);
        } catch (const boost::interprocess::interprocess_exception& e) {
            logger::this_logger->log(
                "Tried to open a dead shared segment from an old message, skipping.",
                logger::level::DBG
            );
            return;
        };
        mapped_region region(shm, read_only);

        /* Deserialize the message payload. */
        std::string stream_str = std::string(reinterpret_cast<const char*>(region.get_address()));
        std::istringstream istream(stream_str);
        text_iarchive ia(istream);

        if (msg.subtype == special_subtype::SETTINGS_COMMIT) {
            /*  If this is a SETTINGS_COMMIT message, make sure that the settings are the ones
                previously proposed and approved. */
            std::string stream_str_ =
                std::string(reinterpret_cast<const char*>(region.get_address()));
            std::istringstream istream_(stream_str_);
            text_iarchive ia_(istream_);

            types::settings_t settings;
            ia_ >> settings;
            if (settings != proposed_settings) {
                logger::this_logger->log(
                    "Skipping settings submitted for commit without a successful check.",
                    logger::level::DBG
                );
                shared_memory_object::remove(msg.shared_segment);
                return;
            }
        }

        /* Call the handler function for this message. */
        int code = handler_map.at(msg.subtype)(ia, nullptr);

        /*  If this is a SETTINGS_COMMIT message and we got this far, it means that the settings in
            the message were indeed valid, so remember them as such. */
        if (msg.subtype == special_subtype::SETTINGS_COMMIT) {
            modules::current_settings = proposed_settings;
        }

        /*  When the handler function returns, it is assumed that the shared memory referenced in
            the message is no longer needed. */
        shared_memory_object::remove(msg.shared_segment);

        /*  Since this was a command message, no response is sent, so the handler function's
            response code is only logged, not sent back. */
        logger::this_logger->log(
            "Message handler for message with id " + std::to_string(msg.id)
                + " of type " + std::to_string(msg.type)
                + " and subtype " + std::to_string(msg.subtype)
                + " responded with code " + std::to_string(code) + ".",
            logger::level::DBG
        );

        /* Notify that this thread has finished so it can be joined. */
        send_(0, special_subtype::JOIN_RCV_CMD, owner, serialize(msg.id), nullptr);
    }

    void messenger::receive_request(const msg_handler_map& handler_map, msg_t& msg) {
        /* Map the shared segment into memory. */
        shared_memory_object shm;
        try {
            shm = shared_memory_object(open_only, msg.shared_segment, read_only);
        } catch (const boost::interprocess::interprocess_exception& e) {
            logger::this_logger->log(
                "Tried to open a dead shared segment from an old message, skipping.",
                logger::level::DBG
            );
            return;
        };
        mapped_region region(shm, read_only);

        /* Deserialize the message payload. */
        std::string stream_str = std::string(reinterpret_cast<const char*>(region.get_address()));
        std::istringstream istream(stream_str);
        text_iarchive ia(istream);

        /*  This is the output string stream where the response can be stored by the handler. */
        std::ostringstream ostream;
        text_oarchive oa(ostream);

        if (msg.subtype == special_subtype::SETTINGS_INIT) {
            /*  If this is a SETTINGS_INIT message, initialize the settings with the ones in the
                message. */
            std::string stream_str_ =
                std::string(reinterpret_cast<const char*>(region.get_address()));
            std::istringstream istream_(stream_str_);
            text_iarchive ia_(istream_);

            ia_ >> modules::current_settings;

            settings_initialized = true;
        }

        /* Call the handler function for this message. */
        int code = handler_map.at(msg.subtype)(ia, &oa);

        /*  If this is a SETTINGS_CHECK message and the check passed, remember the
            proposed settings. */
        if (msg.subtype == special_subtype::SETTINGS_CHECK && code == settings_code::SUCCESS) {
            std::string stream_str_ =
                std::string(reinterpret_cast<const char*>(region.get_address()));
            std::istringstream istream_(stream_str_);
            text_iarchive ia_(istream_);
            ia_ >> proposed_settings;
        }

        /*  When the handler function returns, it is assumed that the shared memory referenced in
            the message is no longer needed. */
        shared_memory_object::remove(msg.shared_segment);

        /*  Not all handler functions generate response content, in which case only a response code
            is sent back. If there is response content, `ostream` will contain it. */
        send_response(msg.id, code, msg.sender, ostream.str());

        logger::this_logger->log(
            "Message handler for message with id " + std::to_string(msg.id)
                + " of type " + std::to_string(msg.type)
                + " and subtype " + std::to_string(msg.subtype)
                + " responded with code " + std::to_string(code) + ".",
            logger::level::DBG
        );
    }


    int messenger::send(
        unsigned int timeout, int subtype, modules::type recipient, std::string payload, std::string* response
    ) {
        if (subtype < 0) {
            return send_error::NEGATIVE_SUBTYPE;
        }
        return send_(timeout, subtype, recipient, payload, response);
    }

    int messenger::send_(unsigned int timeout, int subtype, modules::type recipient, std::string payload, std::string* response) {
        static int id_counter = 0;  /** A static counter variable for the ids of request and command
                                        messages sent out. */
        static std::mutex id_mutex; /** A static mutex to protect access to `id_counter`. */

        id_mutex.lock();
        unsigned int id = !(++id_counter) ? ++id_counter : id_counter; /** id 0 is reserved. */
        id_mutex.unlock();

        msg_t message = {
            type        : timeout ? msg_type::REQUEST : msg_type::COMMAND,
            id          : id,
            sender      : owner,
            {subtype    : subtype},
        };
        message.payload_len = payload.size();

        /* Create a new shared segment for the payload. */
        generate_random_shared_segment_name(message.shared_segment);

        /* Map the shared segment into memory. */
        shared_memory_object shm(open_or_create, message.shared_segment, read_write);
        shm.truncate(payload.size() + 1); // Don't let it crash when the payload is an empty string.
        mapped_region region(shm, read_write);

        /* Copy the payload into the shared segment. */
        memcpy(region.get_address(), payload.c_str(), payload.size());

        /*  If `type` is `REQUEST`, `shared_segment` will be modified with the name of a new shared
            memory segment where the response's content is located. */
        return send_core(message, recipient, timeout, response);
    }

    void messenger::send_response(unsigned int id, int response_code, modules::type recipient, std::string payload) {
        msg_t response = {
            type    : msg_type::RESPONSE,
            id      : id,
            sender  : owner,
            {code   : response_code},
        };
        response.payload_len = payload.size();

        /* A valid request demands a response. */
        generate_random_shared_segment_name(response.shared_segment);

        /* Map the shared segment into memory. */
        shared_memory_object shm(open_or_create, response.shared_segment, read_write);
        shm.truncate(payload.size() + 1); // Don't let it crash when the payload is an empty string.
        mapped_region region(shm, read_write);

        /* Copy the payload into the shared segment. */
        memcpy(region.get_address(), payload.c_str(), payload.size());

        send_core(response, recipient, false, nullptr);
    }

    int messenger::send_core(msg_t& msg, modules::type recipient, unsigned int timeout, std::string* response) {
        mqd_t cur_mq_id;

        /*  Get the appropriate message queue id and name, depending on whether the message is a
            response or not. */

        std::map<modules::type, mqd_t>& ids =
            (msg.type == msg_type::RESPONSE) ? mq_res_ids : mq_ids;

        const std::map<modules::type, std::string> names =
            (msg.type == msg_type::RESPONSE) ? mq_res_names : mq_names;

        if (ids.find(recipient) == ids.end()) {
            cur_mq_id = mq_open(names.at(recipient).c_str(), O_WRONLY | O_CLOEXEC);
            ids[recipient] = cur_mq_id;
        } else {
            cur_mq_id = ids.at(recipient);
        }

        unsigned int priority;
        switch (msg.subtype) {
            case special_subtype::END_LISTEN_LOOP:
                priority = 10;
                break;
            case special_subtype::SETTINGS_CHECK:
                priority = 5;
                break;
            case special_subtype::SETTINGS_COMMIT:
                priority = 7;
                break;
            default:
                priority = 0;
                break;
        }

        if (timeout) {
            /* Create a new shared segment for the payload. */
            char shared_segment[MAXLEN_SHARED_SEGMENT_NAME];

            /* Block until a response is received. */
            int code;
            get_or_put_response(response_action::INTEREST, msg.id, shared_segment, &code, 0);
            int send_err = mq_send(cur_mq_id, reinterpret_cast<char*>(&msg), sizeof(msg), priority);
            if (send_err == -1) {
                get_or_put_response(response_action::NOTIFY, msg.id, nullptr, nullptr, 0);
                return send_error::MQ_ERROR;
            }
            get_or_put_response(response_action::WAIT, msg.id, nullptr, nullptr, timeout);

            if (response != nullptr && code != send_error::SEND_TIMEOUT) {
                /* Map the shared segment into memory. */
                shared_memory_object shm(open_only, shared_segment, read_only);
                mapped_region region(shm, read_only);

                /* Assign to the response string the content inside the shared segment. */
                response->assign(reinterpret_cast<const char*>(region.get_address()));

                /*  Since the content of the shared segment has already been copied into the
                    response string, the shared segment is no longer needed. */
                shared_memory_object::remove(shared_segment);
            }
            return code;
        } else {
            return mq_send(cur_mq_id, reinterpret_cast<char*>(&msg), sizeof(msg), priority);
        }
    }


    void messenger::get_or_put_response(
        response_action action, unsigned int id, char* shared_segment, int* code, unsigned int timeout
    ) {
        /**
         * A static map containing all the ids of messages which are currently awaiting a response.
         * The values are tuples of char, mutex and int pointers.
         * 
         * The `char*` pointers point to the names of shared segments where responses are expected.
         * The `int*` point to where the response codes should be stored.
         */
        static std::map<int, std::tuple<char*, int*>> waiting_map;
        static std::mutex waiting_map_mutex;    /** A static mutex to protect access to `waiting_map`. */

        static std::set<int> notified_ids;      /** A set of message ids for which a response has
                                                    arrived but has not yet been collected. */
        static std::mutex ids_cvm;              /** A mutex to protect access to `notified_ids`. */
        static std::condition_variable ids_cv;  /** A condition variable to notify of changes to
                                                    `notified_ids`. */

        switch (action) {
            case response_action::INTEREST: {
                std::lock_guard<std::mutex> lkw(waiting_map_mutex);
                waiting_map.emplace(id, std::make_tuple(shared_segment, code));
                break;
            }
            case response_action::WAIT: {
                auto wait_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout);
                auto wait_cond = [id]{ return notified_ids.find(id) != notified_ids.end(); };

                std::unique_lock<std::mutex> lk(ids_cvm);

                /*  Block the calling thread here until a thread calling the function with
                    `response_action::NOTIFY` unlocks it again, or until the timeout has passed. */
                if (notified_ids.find(id) == notified_ids.end() && !ids_cv.wait_until(lk, wait_deadline, wait_cond)) {
                    std::lock_guard<std::mutex> lkw(waiting_map_mutex);
                    *(std::get<1>(waiting_map.at(id))) = send_error::SEND_TIMEOUT;
                }
                notified_ids.erase(id);
                break;
            }
            case response_action::NOTIFY: {
                std::lock_guard<std::mutex> lkw(waiting_map_mutex);
                if (waiting_map.find(id) != waiting_map.end()) {
                    if (shared_segment != nullptr) {
                        memccpy(std::get<0>(waiting_map.at(id)), shared_segment, '\0', MAXLEN_SHARED_SEGMENT_NAME);
                    }
                    if (code != nullptr) {
                        *(std::get<1>(waiting_map.at(id))) = *code;
                    }
                    {
                        std::lock_guard<std::mutex> lk(ids_cvm);
                        notified_ids.insert(id);
                    }
                    ids_cv.notify_all();
                    waiting_map.erase(id);
                }
                break;
            }
        }
    }


    void messenger::notify_module_settings(
        types::settings_t settings, modules::type module, std::map<modules::type, int>* responses_map, std::mutex* responses_mutex, bool init
    ) {
        int subtype = init ? special_subtype::SETTINGS_INIT : special_subtype::SETTINGS_CHECK;
        int res = send_(2 * DEFAULT_SEND_TIMEOUT, subtype, module, serialize(settings), nullptr);
        std::lock_guard<std::mutex> lk(*responses_mutex);
        responses_map->emplace(std::make_pair(module, res));
    }

    int messenger::broadcast_settings(types::settings_t settings) {
        std::map<modules::type, int> responses_map; /** Maps module identifiers to the response codes
                                                        they return to the settings broadcast. */
        std::mutex responses_mutex;                 /** A mutex to protect access to `responses_map`. */
        std::vector<std::thread> threads;           /** A vector of notifier threads. */

        proposed_settings = settings;

        /* Notify all modules in new threads. */
        for (const auto& item : mq_names) {
            if (item.first == owner) {
                continue;
            } else {
                threads.push_back(std::move(std::thread(
                    &messenger::notify_module_settings, this, settings, item.first, &responses_map,
                    &responses_mutex, false
                )));
            }
        }

        /* Await responses of all modules. */
        for (auto& thread : threads) {
            thread.join();
        }

        /* Check if there was a timeout or if any module rejected the new settings with an error. */
        int code = settings_code::SUCCESS;
        for (const auto& response : responses_map) {
            if (response.second == send_error::SEND_TIMEOUT) {
                logger::this_logger->log(
                    "There was a timeout waiting for modules to accept the new settings. The new "
                        "settings will not be committed.",
                    logger::level::ERR
                );
                return settings_code::TIMEOUT;
            } else if (response.second != settings_code::SUCCESS) {
                logger::this_logger->log(
                    modules::to_string_extended(response.first) + " rejected new settings with "
                        "error " + std::to_string(response.second) + ". The new settings will not "
                        "be committed.",
                    logger::level::ERR
                );
                code = response.second;
                break;
            }
        }

        /* Tell all modules to accept or reject the new settings. */
        if (code == settings_code::SUCCESS) {
            for (const auto& item : mq_names) {
                send_(0, special_subtype::SETTINGS_COMMIT, item.first, serialize(settings), nullptr);
            }
        }

        return code;
    }

    int messenger::broadcast_settings_init(types::settings_t settings) {
        if (owner != modules::type::LAUNCHER) {
            return settings_code::INVALID_CALLER;
        }

        std::map<modules::type, int> responses_map; /** Maps module identifiers to the response codes
                                                        they return to the settings broadcast. */
        std::mutex responses_mutex;                 /** A mutex to protect access to `responses_map`. */
        std::vector<std::thread> threads;           /** A vector of notifier threads. */

        /* Notify all modules in new threads. */
        for (const auto& item : mq_names) {
            if (item.first == owner) {
                continue;
            } else {
                threads.push_back(std::move(std::thread(
                    &messenger::notify_module_settings, this, settings, item.first, &responses_map,
                    &responses_mutex, true
                )));
            }
        }

        /* Await responses of all modules with a timeout. */
        for (auto& thread : threads) {
            thread.join();
        }

        /* Check if there was a timeout or if any module rejected the new settings with an error. */
        for (const auto& response : responses_map) {
            if (response.second == send_error::SEND_TIMEOUT) {
                logger::this_logger->log(
                   "There was a timeout waiting for modules to accept the settings.",
                    logger::level::ERR
                );
                return settings_code::TIMEOUT;
            }
        }

        return settings_code::SUCCESS;
    }


    void messenger::generate_random_shared_segment_name(char* buffer) {
        static const char charset[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

        /*  `rand()` may not be a good rng, but it's cheap and good enough for strings of this
            length. We don't need very good randomness, only collision freeness. */
        for (int i = 0; i < MAXLEN_SHARED_SEGMENT_NAME - 1; ++i) {
            buffer[i] = charset[rand() % (sizeof(charset) - 1)];
        }
        buffer[MAXLEN_SHARED_SEGMENT_NAME - 1] = '\0';
    }

}
