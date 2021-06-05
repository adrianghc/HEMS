/*
 * Copyright (c) 2020-2021 Adrian Georg Herrmann
 * 
 * This header declares the `messenger` class.
 * This class creates a thread to listen for new messages in a given message queue, and contains
 * methods to send messages to other message queues. It uses callback functions (called message
 * handlers) for different message types.
 */

#ifndef HEMS_COMMON_MESSENGER_H
#define HEMS_COMMON_MESSENGER_H

#include <atomic>
#include <mutex>
#include <thread>
#include <csignal>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include "extras/semaphore.hpp"

#include "hems/common/modules.h"
#include "hems/common/types.h"

namespace hems {

    using boost::archive::text_iarchive;
    using boost::archive::text_oarchive;

    class messenger {

        #define MAXLEN_SHARED_SEGMENT_NAME  25
        #define DEFAULT_SEND_TIMEOUT        5000

        public:
            /**
             * @brief       Constructs a `messenger` object, with test mode disabled.
             * 
             * @param[in]   owner   The owner of this `messenger` instance.
             */
            messenger(modules::type owner);

            /**
             * @brief       Constructs a `messenger` object.
             * 
             * @param[in]   owner       The owner of this `messenger` instance.
             * @param[in]   test_mode   Whether test mode is on or not. In test mode, the settings
             *                          initialization is not enforced and message handlers for the
             *                          settings messages are not required.
             */
            messenger(modules::type owner, bool test_mode);

            virtual ~messenger();

            static messenger* this_messenger;   /** Instance of the `messenger` class to read from
                                                    the message queue and write to other modules'
                                                    message queues in separate threads. */

            static const std::map<modules::type, std::string> mq_names; /** A map from module identifiers to
                                                                            message queue names. This map
                                                                            contains all the message queues
                                                                            used by the HEMS and managed by
                                                                            the Launcher Module. */

            static const std::map<modules::type, std::string> mq_res_names; /** A map from module identifiers to
                                                                                message queue names. This map
                                                                                contains all the message queues
                                                                                used by the HEMS for modules to
                                                                                receive responses to their requests. */

            /**
             * @brief       The broad type of a message, i.e. whether it's a command, a request or a
             *              response. A command differs from a request in that it does not expect a
             *              response. Each module defines its own subtypes.
             */
            enum msg_type {
                COMMAND,
                REQUEST,
                RESPONSE
            };

            /**
             * @brief       A message for message queues, used by all modules.
             *              Each module uses this struct for its command, request or response messages.
             *              For request messages, each module defines its own types and interprets the
             *              content according to that type.
             *              For response messages, each module can define its own error codes.
             */
            typedef struct {
                msg_type        type;   /** The message's type, i.e. whether it's a command, a
                                            request or a response. */
                unsigned int    id;     /** The message's id. A response's id is identical to that
                                            of the request that preceded it. */
                modules::type sender;   /** The sender of this message. */
                union {
                    int subtype;        /** The message's subtype, defined by each module, if the
                                            message is a command or a request. Negative numbers are
                                            reserved. */
                    int code;           /** The response code, if the message is a response.
                                            The success code and the generic error code are globally
                                            known, but each module can define more error codes. */
                };  /** Additional information, depending on the message type. */
                char shared_segment[MAXLEN_SHARED_SEGMENT_NAME];    /** The name of the shared memory segment
                                                                        where the message's payload,
                                                                        interpreted by each module according
                                                                        to the subtype, is located. Can be a
                                                                        0 string for a response message. */
                size_t payload_len;     /** The length of the payload in the shared segment. */
            } msg_t;

            /**
             * @brief       A map from a message subtype to a handler function for that message
             *              subtype.
             *              The handler function must return an exit status as an int and receive a
             *              `text_iarchive&` and a `char*` as input.
             *              The `text_iarchive&` points to a text archive containing a message of
             *              the respective subtype.
             *              The `text_oarchive*` points to an existing text archive where a
             *              serialized response can be stored, if a response is expected. Otherwise,
             *              it is `nullptr`.
             */
            typedef std::map<int, const std::function<int(text_iarchive&, text_oarchive*)>> msg_handler_map;

            /**
             * @brief       Start the listening loop that waits for incoming messages, identifies
             *              their type and calls handler functions accordingly, or wakes up waiting
             *              sender threads when their awaited response arrives.
             * 
             * @param[in]   handler_map     A map from message types to message handler functions.
             * 
             * @return      Whether the listen loop started successfully.
             */
            bool listen(const msg_handler_map& handler_map);

            /**
             * @brief       Start the listening loop that waits for incoming messages, identifies
             *              their type and calls handler functions accordingly, or wakes up waiting
             *              sender threads when their awaited response arrives.
             * 
             * @param[in]   handler_map         A map from message types to message handler functions.
             * @param[in]   pre_init_whitelist  A list of message types that may be received even
             *                                  before setttings initialization.
             * 
             * @return      Whether the listen loop started successfully.
             */
            bool listen(const msg_handler_map& handler_map, const std::vector<int> pre_init_whitelist);

            /**
             * @brief       Tells the messenger to start handling messages on the queue. This method
             *              is to be called by modules at the end of their constructors. This is to
             *              avoid calling message handlers before the module's constructor has
             *              finished, which would cause segmentation faults.
             */
            void start_handlers();

            /**
             * @brief       Sends a command or request message with the given parameters.
             * 
             * @param[in]   timeout         The timeout in milliseconds. If the timeout is 0, an
             *                              asynchronous command message is sent, for which no
             *                              response is awaited.
             * @param[in]   subtype         The message's subtype, defined by each module. Negative
             *                              numbers are reserved.
             * @param[in]   recipient       The message's recipient.
             * @param[in]   payload         The serialized message payload.
             * @param[out]  response        If `timeout` is not 0, `response` will be modified with
             *                              the response's content as a serialized string. If it is
             *                              `nullptr`, no response will be provided.
             *                              If `timeout` is 0, `response` will be ignored and can be
             *                              `nullptr` .
             * 
             * @return      MQ_ERROR            if the message queue returns an error.
             *              TIMEOUT             if there was a timeout.
             *              NEGATIVE_SUBTYPE    if the subtype is a reserved negative number.
             * 
             *              The response code otherwise (only 0 if the function was called with
             *              `timeout` = 0).
             */
            int send(
                unsigned int timeout, int subtype, modules::type recipient, std::string payload, std::string* response
            );

            /**
             * @brief       Response codes for settings broadcasts.
             */
            enum send_error {
                MQ_ERROR            = -1,
                SEND_TIMEOUT        = -2,
                NEGATIVE_SUBTYPE    = -3
            };

            /**
             * @brief       Broadcasts (new) settings out to all modules.
             * 
             * @param[in]   settings        The new settings.
             * 
             * @return      SUCCESS         if all modules respond with success.
             *              TIMEOUT         if there was a timeout waiting for modules to respond.
             *              INVALID         if a module rejected the new settings as invalid.
             *              INTERNAL_ERROR  if a module rejected the new settings due to an internal
             *                              error.
             */
            int broadcast_settings(types::settings_t settings);

            /**
             * @brief       Broadcasts the current settings out to all modules at HEMS
             *              initialization. Only the HEMS Launcher can use this method.
             * 
             * @param[in]   settings        The settings.
             * 
             * @return      SUCCESS         if all modules respond with success.
             *              TIMEOUT         if there was a timeout waiting for modules to respond.
             *              INVALID_CALLER  if this function was not called by the HEMS Launcher.
             */
            int broadcast_settings_init(types::settings_t settings);

            /**
             * @brief       Response codes for settings broadcasts.
             */
            enum settings_code {
                SUCCESS         = 0x00,
                TIMEOUT         = 0x01,
                INVALID         = 0x02,
                INTERNAL_ERROR  = 0x03,
                INVALID_CALLER  = 0x04
            };

            /**
             * @brief       The special subtypes caught by the messenger before passing them to a
             *              message handler.
             */
            enum special_subtype {
                END_LISTEN_LOOP = -1,
                SETTINGS_INIT   = -2,
                SETTINGS_CHECK  = -3,
                SETTINGS_COMMIT = -4,
                JOIN_RCV_CMD    = -5
            };

            /**
             * @brief       Serializes a message payload into a string.
             * 
             * @param[in]   content         The content to be serialized.
             * 
             * @return      The serialized content as string.
             */
            template<typename T> static std::string serialize(T& content) {
                std::ostringstream stream;
                boost::archive::text_oarchive oa(stream);
                oa << content;
                return stream.str();
            };

            /**
             * @brief       Deserializes a string into a message payload.
             * 
             * @param[in]   content         The string to be deserialized.
             * 
             * @return      The deserialized message payload.
             */
            template<typename T> static T deserialize(std::string& content) {
                T msg;
                std::istringstream stream(content);
                boost::archive::text_iarchive ia(stream);
                ia >> msg;
                return msg;
            };

        private:
            modules::type owner;    /** The owner of this instance. This determines e.g. which
                                        message queue to listen to. */

            bool test_mode; /** Whether the messenger is running in test mode. In test mode, the
                                settings initialization is not enforced and message handlers for the
                                settings messages are not required. */

            std::map<modules::type, mqd_t> mq_ids;  /** The ids of the message queues to listen to
                                                        or write in. */

            std::map<modules::type, mqd_t> mq_res_ids;  /** The ids of the message queues to listen to
                                                            or write in responses. */

            std::thread* listener;      /** The thread running the listen loop for requests and
                                            commands. */
            std::thread* listener_res;  /** The thread running the listen loop for responses. */

            bool do_start_handlers = false; /** Whether the listen loop should begin handling
                                                messages. This variable is set by the `go()` method,
                                                which is called by a module when its constructor
                                                finishes. This is to prevent situations where a
                                                message handler is called before the module's
                                                constructor has finished, leading to segmentation
                                                faults. */
            std::mutex sh_m;                /** A mutex to protect access to `do_start_handlers`. */
            std::condition_variable sh_cv;  /** A condition variable to notify of changes to
                                                `do_start_handlers`. */

            types::settings_t proposed_settings;    /** The last proposed settings. */

            bool settings_initialized = false;  /** Whether the settings have been initialized. This
                                                    should only be false at the beginning of the
                                                    HEMS lifecycle when the module does not know any
                                                    settings yet. */

            /**
             * @brief       A loop that waits for incoming request or command messages and calls
             *              handler functions accordingly.
             * 
             * @param[in]   handler_map         A map from message types to message handler functions.
             * @param[in]   pre_init_whitelist  A list of message types that may be received even
             *                                  before setttings initialization.
             */
            void listen_loop(const msg_handler_map& handler_map, const std::vector<int> pre_init_whitelist);

            /**
             * @brief       A loop that waits for incoming response messages and wakes up waiting
             *              sender threads.
             */
            void listen_loop_res();

            /**
             * @brief       Handles a command message upon reception.
             * 
             * @param[in]   handler_map     A map from message types to message handler functions.
             * @param[in]   msg             The command message.
             */
            void receive_command(const msg_handler_map& handler_map, msg_t msg);

            /**
             * @brief       Handles a request message upon reception.
             * 
             * @param[in]   handler_map     A map from message types to message handler functions.
             * @param[in]   msg             The request message.
             */
            void receive_request(const msg_handler_map& handler_map, msg_t& msg);

            /**
             * @brief       Sends a command or request message with the given parameters.
             * 
             * @param[in]   timeout         The timeout in milliseconds. If the timeout is 0, an
             *                              asynchronous command message is sent, for which no
             *                              response is awaited.
             * @param[in]   subtype         The message's subtype, defined by each module. Negative
             *                              numbers are reserved.
             * @param[in]   recipient       The message's recipient.
             * @param[in]   payload         The serialized message payload.
             * @param[out]  response        If `timeout` is not 0, `response` will be modified with
             *                              the response's content as a serialized string. If it is
             *                              `nullptr`, no response will be provided.
             *                              If `timeout` is 0, `response` will be ignored and can be
             *                              `nullptr` .
             * 
             * @return      MQ_ERROR            if the message queue returns an error.
             *              TIMEOUT             if there was a timeout.
             *              NEGATIVE_SUBTYPE    if the subtype is a reserved negative number.
             * 
             *              The response code otherwise (only 0 if the function was called with
             *              `timeout` = 0).
             */
            int send_(unsigned int timeout, int subtype, modules::type recipient, std::string payload, std::string* response);

            /**
             * @brief       Sends a response message with the given parameters.
             *              This send function does not wait for a response.
             * 
             * @param[in]   id              The response's id, which is identical to that of the
             *                              request that preceded it.
             * @param[in]   response_code   The response code.
             * @param[in]   recipient       The response's recipient, i.e. the sender of the request
             *                              that precededed it.
             * @param[in]   payload         The serialized message payload.
             */
            void send_response(unsigned int id, int response_code, modules::type recipient, std::string payload);

            /**
             * @brief       Sends a message with the given parameters.
             * 
             * @param[in]   msg             A reference to a completed message object.
             * @param[in]   recipient       The message's recipient.
             * @param[in]   timeout         The timeout in milliseconds. If the timeout is 0, an
             *                              asynchronous command message is sent, for which no
             *                              response is awaited.
             * @param[out]  response        If `timeout` is not 0, `response` will be modified with
             *                              the response's content as a serialized string. If it is
             *                              `nullptr`, no response will be provided.
             *                              If `timeout` is 0, `response` will be ignored and can be
             *                              `nullptr` .
             * 
             * @return      MQ_ERROR            if the message queue returns an error.
             *              TIMEOUT             if there was a timeout.
             *              NEGATIVE_SUBTYPE    if the subtype is a reserved negative number.
             * 
             *              The response code otherwise.
             */
            int send_core(msg_t& msg, modules::type recipient, unsigned int timeout, std::string* response);


            /**
             * @brief       The type of action associated with a call to `get_or_put_response()`:
             *              `INTEREST` means that `get_or_put_response()` is called in order to
             *              announce interest for a response message.
             *              `WAIT` means that `get_or_put_response()` is called in order to wait for
             *              a response message for which interest was previously announced.
             *              `NOTIFY` means that `get_or_put_response()` is called in order to notify
             *              that a response message that a caller has interest in has arrived.
             */
            enum response_action {
                INTEREST,
                WAIT,
                NOTIFY
            };

            /**
             * @brief       Access the map of message ids for which a response is awaited. This
             *              access can be to insert a new id, to check if an id is present, or to
             *              delete an id.
             * 
             * @param[in]   action          Whether the requested access is `INTEREST`, `WAIT` or
             *                              `NOTIFY` .
             * @param[in]   id              The message id for which to make the access.
             * @param[in]   shared_segment  If `action` is `INTEREST`, the address to the `char*`
             *                              where the name of the response content's shared segment
             *                              is to be stored. This char* is stored as the value to
             *                              key `id` in the map.
             *                              If `action` is `WAIT`, it will be ignored and can be
             *                              `nullptr` .
             *                              If `action` is `NOTIFY`, the address to the `char*`
             *                              where the name of the response content's shared segment
             *                              is currently stored. Its content is then copied to the
             *                              `char*` that is stored as the value for the key `id`.
             * @param[in]   code            If `action` is `INTEREST`, a pointer to store the
             *                              response code in.
             *                              If `action` is `WAIT`, `nullptr`.
             *                              If `action` is `NOTIFY`, a pointer to the response code,
             *                              or `nullptr` if there is a message queue failure.
             * @param[in]   timeout         If `action` is `WAIT`, the timeout for awaiting a
             *                              response.
             *                              Otherwise 0.
             */
            void get_or_put_response(
                response_action action, unsigned int id, char* shared_segment, int* code, unsigned int timeout
            );


            /**
             * @brief       This function notifies a module of a settings update or initialization
             *              and waits for its response.
             * 
             * @param[in]   settings        The settings to notify the module of.
             * @param[in]   module          The module to be notified.
             * @param[in]   responses_map   A pointer to the map where the response is stored for
             *                              each module.
             * @param[in]   responses_mutex A pointer to a mutex to protect access to `responses.map`.
             * @param[in]   init            Whether this call is for a settings initialization or
             *                              not.
             */
            void notify_module_settings(
                types::settings_t settings, modules::type module, std::map<modules::type, int>* responses_map,
                std::mutex* responses_mutex, bool init
            );


            /**
             * @brief       Generates a random name for a shared segment. The name will have a
             *              length of messages::MAXLEN_SHARED_SEGMENT_NAME - 1 and be stored in the
             *              given buffer.
             *              IT IS THE RESPONSIBILITY OF THE CALLER TO MAKE SURE BUFFER HAS A SIZE OF
             *              MAXLEN_SHARED_SEGMENT_NAME.
             * 
             * @param[out]  buffer          The buffer where the generated shared segment name will
             *                              be stored.
             */
            static void generate_random_shared_segment_name(char* buffer);


            friend class messenger_test; /* Friend class to let tests access private members. */

    };

}

#endif /* HEMS_COMMON_MESSENGER_H */
