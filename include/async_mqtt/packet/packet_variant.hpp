// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_PACKET_VARIANT_HPP)
#define ASYNC_MQTT_PACKET_PACKET_VARIANT_HPP

#include <variant>

#include <boost/asio/buffer.hpp>

#include <async_mqtt/exception.hpp>

#include <async_mqtt/packet/control_packet_type.hpp>

#include <async_mqtt/util/overload.hpp>

namespace async_mqtt {
namespace as = boost::asio;

namespace v3_1_1 {

class connack_packet;
class connect_packet;
template <std::size_t PacketIdBytes> class basic_publish_packet;
template <std::size_t PacketIdBytes> class basic_puback_packet;
template <std::size_t PacketIdBytes> class basic_pubrec_packet;
template <std::size_t PacketIdBytes> class basic_pubrel_packet;
template <std::size_t PacketIdBytes> class basic_pubcomp_packet;
template <std::size_t PacketIdBytes> class basic_subscribe_packet;
template <std::size_t PacketIdBytes> class basic_suback_packet;
template <std::size_t PacketIdBytes> class basic_unsubscribe_packet;
template <std::size_t PacketIdBytes> class basic_unsuback_packet;
class pingreq_packet;
class pingresp_packet;
class disconnect_packet;

} // namespace v3_1_1

namespace v5 {

class connack_packet;
class connect_packet;
template <std::size_t PacketIdBytes> class basic_publish_packet;
template <std::size_t PacketIdBytes> class basic_puback_packet;
template <std::size_t PacketIdBytes> class basic_pubrec_packet;
template <std::size_t PacketIdBytes> class basic_pubrel_packet;
template <std::size_t PacketIdBytes> class basic_pubcomp_packet;
template <std::size_t PacketIdBytes> class basic_subscribe_packet;
template <std::size_t PacketIdBytes> class basic_suback_packet;
template <std::size_t PacketIdBytes> class basic_unsubscribe_packet;
template <std::size_t PacketIdBytes> class basic_unsuback_packet;
class pingreq_packet;
class pingresp_packet;
class disconnect_packet;
class auth_packet;

} // namespace v5

/**
 * @breif The varaint type of all packets and system_error
 *
 */
template <std::size_t PacketIdBytes>
class basic_packet_variant {
public:
    basic_packet_variant() = default;

    /**
     * @brief constructor
     * @param packet packet
     */
    template <
        typename Packet,
        std::enable_if_t<
            !std::is_same_v<
                std::decay_t<Packet>,
                basic_packet_variant<PacketIdBytes>
            >
        >* = nullptr
    >
    basic_packet_variant(Packet&& packet):var_{std::forward<Packet>(packet)}
    {}

    /**
     * @brief visit to variant
     * @param func Visitor function
     */
    template <typename Func>
    auto visit(Func&& func) const& {
        return
            std::visit(
                std::forward<Func>(func),
                var_
            );
    }

    /**
     * @brief visit to variant
     * @param func Visitor function
     */
    template <typename Func>
    auto visit(Func&& func) & {
        return
            std::visit(
                std::forward<Func>(func),
                var_
            );
    }

    /**
     * @brief visit to variant
     * @param func Visitor function
     */
    template <typename Func>
    auto visit(Func&& func) && {
        return
            std::visit(
                std::forward<Func>(func),
                force_move(var_)
            );
    }

    /**
     * @brief Get by type. If not match, then throw std::bad_variant_access exception.
     * @return actual packet
     */
    template <typename T>
    decltype(auto) get() {
        return std::get<T>(var_);
    }

    /**
     * @brief Get by type. If not match, then throw std::bad_variant_access exception.
     * @return actual packet
     */
    template <typename T>
    decltype(auto) get() const {
        return std::get<T>(var_);
    }

    /**
     * @brief Get by type pointer
     * @return actual packet pointer. If not match then return nullptr.
     */
    template <typename T>
    decltype(auto) get_if() {
        return std::get_if<T>(&var_);
    }

    /**
     * @brief Get by type pointer
     * @return actual packet pointer. If not match then return nullptr.
     */
    template <typename T>
    decltype(auto) get_if() const {
        return std::get_if<T>(&var_);
    }

    /**
     * @brief Get control_packet_type.
     * @return If packet is stored then return its control_packet_type, If error then return std::nullopt.
     */
    std::optional<control_packet_type> type() const {
        return visit(
            overload {
                [] (auto const& p) -> std::optional<control_packet_type>{
                    return p.type();
                },
                [] (system_error const&) -> std::optional<control_packet_type>{
                    return std::nullopt;
                }
            }
        );
    }

    /**
     * @brief Create const buffer sequence
     *        it is for boost asio APIs
     * @return const buffer sequence
     */
    std::vector<as::const_buffer> const_buffer_sequence() const {
        return visit(
            overload {
                [] (auto const& p) {
                    return p.const_buffer_sequence();
                },
                [] (system_error const&) {
                    return std::vector<as::const_buffer>{};
                }
            }
        );
    }

    operator bool() const {
        return var_.index() != 0;
    }

private:
    using variant_t = std::variant<
        system_error,
        v3_1_1::connect_packet,
        v3_1_1::connack_packet,
        v3_1_1::basic_publish_packet<PacketIdBytes>,
        v3_1_1::basic_puback_packet<PacketIdBytes>,
        v3_1_1::basic_pubrec_packet<PacketIdBytes>,
        v3_1_1::basic_pubrel_packet<PacketIdBytes>,
        v3_1_1::basic_pubcomp_packet<PacketIdBytes>,
        v3_1_1::basic_subscribe_packet<PacketIdBytes>,
        v3_1_1::basic_suback_packet<PacketIdBytes>,
        v3_1_1::basic_unsubscribe_packet<PacketIdBytes>,
        v3_1_1::basic_unsuback_packet<PacketIdBytes>,
        v3_1_1::pingreq_packet,
        v3_1_1::pingresp_packet,
        v3_1_1::disconnect_packet,
        v5::connect_packet,
        v5::connack_packet,
        v5::basic_publish_packet<PacketIdBytes>,
        v5::basic_puback_packet<PacketIdBytes>,
        v5::basic_pubrec_packet<PacketIdBytes>,
        v5::basic_pubrel_packet<PacketIdBytes>,
        v5::basic_pubcomp_packet<PacketIdBytes>,
        v5::basic_subscribe_packet<PacketIdBytes>,
        v5::basic_suback_packet<PacketIdBytes>,
        v5::basic_unsubscribe_packet<PacketIdBytes>,
        v5::basic_unsuback_packet<PacketIdBytes>,
        v5::pingreq_packet,
        v5::pingresp_packet,
        v5::disconnect_packet,
        v5::auth_packet
>;

    variant_t var_;
};

template <std::size_t PacketIdBytes>
inline std::ostream& operator<<(std::ostream& o, basic_packet_variant<PacketIdBytes> const& v) {
    v.visit(
        overload {
            [&] (auto const& p) {
                o << p;
            },
            [&] (system_error const& se) {
                o << se.what();
            }
        }
    );
    return o;
}

/**
 * @related basic_packet_variant
 * @brief type alias of basic_packet_variant (PacketIdBytes=2).
 * @tparam Role          role for packet sendable checking
 * @tparam NextLayer     Just next layer for basic_endpoint. mqtt, mqtts, ws, and wss are predefined.
 */
using packet_variant = basic_packet_variant<2>;

} // namespace async_mqtt

#endif // ASYNC_MQTT_PACKET_PACKET_VARIANT_HPP
