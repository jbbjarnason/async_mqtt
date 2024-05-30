// Copyright Takatoshi Kondo 2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This example connects to the specified MQTT broker.
// It then publishes to topic1, topic2, and topic3, and receives the publish results.
// Finally, it disconnects from the broker.
//
// Example:
// ./cl_cpp20coro_mqtt_pub mqtt.redboltz.net 1883

#include <iostream>
#include <string>

#include <boost/asio.hpp>

#include <async_mqtt/all.hpp>

namespace as = boost::asio;
namespace am = async_mqtt;

using client_t = am::client<am::protocol_version::v5, am::protocol::mqtt>;

struct app {
    app(as::any_io_executor exe, std::string_view host, std::string_view port)
        : cli_{exe}
    {
        am::async_underlying_handshake(
            cli_.next_layer(),
            host,
            port,
            [this](auto&&... args) {
                handle_underlying_handshake(
                    std::forward<std::remove_reference_t<decltype(args)>>(args)...
                );
            }
        );
    }

private:
    void handle_underlying_handshake(
        am::error_code ec
    ) {
        std::cout << "underlying_handshake:" << ec.message() << std::endl;
        if (ec) return;
        cli_.async_start(
            true,                // clean_start
            std::uint16_t(0),    // keep_alive
            "",                  // Client Identifier, empty means generated by the broker
            std::nullopt,        // will
            "UserName1",
            "Password1",
            [this](auto&&... args) {
                handle_start_response(std::forward<decltype(args)>(args)...);
            }
        );
    }

    void handle_start_response(
        am::error_code ec,
        std::optional<client_t::connack_packet> connack_opt
    ) {
        std::cout << "start:" << ec.message() << std::endl;
        if (ec) return;
        if (connack_opt) {
            std::cout << *connack_opt << std::endl;
            cli_.async_publish(
                "topic1",
                "payload1",
                am::qos::at_most_once,
                [this](auto&&... args) {
                    handle_publish_response(
                        std::forward<std::remove_reference_t<decltype(args)>>(args)...
                    );
                }
            );
            cli_.async_publish(
                *cli_.acquire_unique_packet_id(), // sync version only works thread safe context
                "topic2",
                "payload2",
                am::qos::at_least_once,
                [this](auto&&... args) {
                    handle_publish_response(
                        std::forward<std::remove_reference_t<decltype(args)>>(args)...
                    );
                }
            );
            cli_.async_publish(
                *cli_.acquire_unique_packet_id(), // sync version only works thread safe context
                "topic3",
                "payload3",
                am::qos::exactly_once,
                [this](auto&&... args) {
                    handle_publish_response(
                        std::forward<std::remove_reference_t<decltype(args)>>(args)...
                    );
                }
            );
        }
    }

    void handle_publish_response(
        am::error_code ec,
        client_t::pubres_type pubres
    ) {
        std::cout << "publish:" << ec.message() << std::endl;
        if (ec) return;
        if (pubres.puback_opt) {
            std::cout << *pubres.puback_opt << std::endl;
        }
        if (pubres.pubrec_opt) {
            std::cout << *pubres.pubrec_opt << std::endl;
        }
        if (pubres.pubcomp_opt) {
            std::cout << *pubres.pubcomp_opt << std::endl;
            cli_.async_disconnect(as::detached);
        }
    }

    client_t cli_;
};

int main(int argc, char* argv[]) {
    am::setup_log(am::severity_level::warning);
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " host port" << std::endl;
        return -1;
    }
    as::io_context ioc;
    app a{ioc.get_executor(), argv[1], argv[2]};
    ioc.run();
}
