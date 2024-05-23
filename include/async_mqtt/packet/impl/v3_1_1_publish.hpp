// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_V3_1_1_PUBLISH_HPP)
#define ASYNC_MQTT_PACKET_IMPL_V3_1_1_PUBLISH_HPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/packet/v3_1_1_publish.hpp>
#include <async_mqtt/util/buffer.hpp>
#include <async_mqtt/util/variable_bytes.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>
#include <async_mqtt/util/utf8validate.hpp>

#include <async_mqtt/packet/packet_id_type.hpp>
#include <async_mqtt/packet/pubopts.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>
#include <async_mqtt/packet/detail/fixed_header.hpp>

#include <async_mqtt/packet/impl/copy_to_static_vector.hpp>

#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
#include <async_mqtt/util/json_like_out.hpp>
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)


namespace async_mqtt::v3_1_1 {

namespace as = boost::asio;

template <std::size_t PacketIdBytes>
template <
    typename StringViewLike,
    typename Payload,
    std::enable_if_t<
        std::is_convertible_v<std::decay_t<StringViewLike>, std::string_view> &&
        detail::is_payload<Payload>(),
        std::nullptr_t
    >
>
inline
basic_publish_packet<PacketIdBytes>::basic_publish_packet(
    typename basic_packet_id_type<PacketIdBytes>::type packet_id,
    StringViewLike&& topic_name,
    Payload&& payloads,
    pub::opts pubopts
)
    : fixed_header_(
          detail::make_fixed_header(control_packet_type::publish, 0b0000) | std::uint8_t(pubopts)
    ),
      topic_name_{std::string{std::forward<StringViewLike>(topic_name)}},
      packet_id_(PacketIdBytes),
      remaining_length_(
          2                      // topic name length
          + topic_name_.size()   // topic name
          + (  (pubopts.get_qos() == qos::at_least_once || pubopts.get_qos() == qos::exactly_once)
               ? PacketIdBytes // packet_id
               : 0)
      )
{
    topic_name_length_buf_.resize(topic_name_length_buf_.capacity());
    endian_store(
        boost::numeric_cast<std::uint16_t>(topic_name_.size()),
        topic_name_length_buf_.data()
    );

    if constexpr (std::is_convertible_v<std::decay_t<Payload>, std::string_view>) {
        remaining_length_ += std::string_view(payloads).size();
        payloads_.emplace_back(buffer{std::string{payloads}});
    }
    else {
        auto b = std::cbegin(payloads);
        auto e = std::cend(payloads);
        auto num_of_payloads = static_cast<std::size_t>(std::distance(b, e));
        payloads_.reserve(num_of_payloads);
        for (; b != e; ++b) {
            auto const& payload = *b;
            remaining_length_ += payload.size();
            payloads_.push_back(payload);
        }
    }

    if (!utf8string_check(topic_name_)) {
        throw system_error(
            make_error_code(
                disconnect_reason_code::topic_name_invalid
            )
        );
    }

    auto rb = val_to_variable_bytes(boost::numeric_cast<std::uint32_t>(remaining_length_));
    for (auto e : rb) {
        remaining_length_buf_.push_back(e);
    }
    switch (pubopts.get_qos()) {
    case qos::at_most_once:
        if (packet_id != 0) {
            throw system_error(
                make_error_code(
                    disconnect_reason_code::protocol_error
                )
            );
        }
        endian_store(0, packet_id_.data());
        break;
    case qos::at_least_once:
    case qos::exactly_once:
        if (packet_id == 0) {
            throw system_error(
                make_error_code(
                    disconnect_reason_code::protocol_error
                )
            );
        }
        endian_store(packet_id, packet_id_.data());
        break;
    default:
        throw system_error(
            make_error_code(
                disconnect_reason_code::malformed_packet
            )
        );
        break;
    }
}

template <std::size_t PacketIdBytes>
template <
    typename StringViewLike,
    typename Payload,
    std::enable_if_t<
        std::is_convertible_v<std::decay_t<StringViewLike>, std::string_view> &&
        detail::is_payload<Payload>(),
        std::nullptr_t
    >
>
inline
basic_publish_packet<PacketIdBytes>::basic_publish_packet(
    StringViewLike&& topic_name,
    Payload&& payloads,
    pub::opts pubopts
) : basic_publish_packet{
        0,
        std::forward<StringViewLike>(topic_name),
        std::forward<Payload>(payloads),
        pubopts
    }
{
}

template <std::size_t PacketIdBytes>
inline
constexpr control_packet_type basic_publish_packet<PacketIdBytes>::type() {
    return control_packet_type::publish;
}

template <std::size_t PacketIdBytes>
inline
std::vector<as::const_buffer> basic_publish_packet<PacketIdBytes>::const_buffer_sequence() const {
    std::vector<as::const_buffer> ret;
    ret.reserve(num_of_const_buffer_sequence());
    ret.emplace_back(as::buffer(&fixed_header_, 1));
    ret.emplace_back(as::buffer(remaining_length_buf_.data(), remaining_length_buf_.size()));
    ret.emplace_back(as::buffer(topic_name_length_buf_.data(), topic_name_length_buf_.size()));
    ret.emplace_back(as::buffer(topic_name_));
    if (packet_id() != 0) {
        ret.emplace_back(as::buffer(packet_id_.data(), packet_id_.size()));
    }
    for (auto const& payload : payloads_) {
        ret.emplace_back(as::buffer(payload));
    }
    return ret;
}

template <std::size_t PacketIdBytes>
inline
std::size_t basic_publish_packet<PacketIdBytes>::size() const {
    return
        1 +                            // fixed header
        remaining_length_buf_.size() +
        remaining_length_;
}

template <std::size_t PacketIdBytes>
inline
std::size_t basic_publish_packet<PacketIdBytes>::num_of_const_buffer_sequence() const {
    return
        1 +                   // fixed header
        1 +                   // remaining length
        2 +                   // topic name length, topic name
        [&] () -> std::size_t {
            if (packet_id() == 0) return 0;
            return 1;
        }() +
        payloads_.size();

}

template <std::size_t PacketIdBytes>
inline
typename basic_packet_id_type<PacketIdBytes>::type basic_publish_packet<PacketIdBytes>::packet_id() const {
    return endian_load<typename basic_packet_id_type<PacketIdBytes>::type>(packet_id_.data());
}

template <std::size_t PacketIdBytes>
inline
constexpr pub::opts basic_publish_packet<PacketIdBytes>::opts() const {
    return pub::opts(fixed_header_);
}

template <std::size_t PacketIdBytes>
inline
std::string basic_publish_packet<PacketIdBytes>::topic() const {
    return std::string{topic_name_};
}

template <std::size_t PacketIdBytes>
inline
buffer const& basic_publish_packet<PacketIdBytes>::topic_as_buffer() const {
    return topic_name_;
}

template <std::size_t PacketIdBytes>
inline
std::string basic_publish_packet<PacketIdBytes>::payload() const {
    return to_string(payloads_);
}

template <std::size_t PacketIdBytes>
inline
auto basic_publish_packet<PacketIdBytes>::payload_range() const {
    return make_packet_range(payloads_);
}

template <std::size_t PacketIdBytes>
inline
std::vector<buffer> const& basic_publish_packet<PacketIdBytes>::payload_as_buffer() const {
    return payloads_;
}

template <std::size_t PacketIdBytes>
inline
constexpr void basic_publish_packet<PacketIdBytes>::set_dup(bool dup) {
    pub::set_dup(fixed_header_, dup);
}

template <std::size_t PacketIdBytes>
inline
basic_publish_packet<PacketIdBytes>::basic_publish_packet(buffer buf, error_code& ec)
    : packet_id_(PacketIdBytes) {
    // fixed_header
    if (buf.empty()) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    fixed_header_ = static_cast<std::uint8_t>(buf.front());
    auto qos_value = pub::get_qos(fixed_header_);
    buf.remove_prefix(1);
    auto cpt_opt = get_control_packet_type_with_check(static_cast<std::uint8_t>(fixed_header_));
    if (!cpt_opt || *cpt_opt != control_packet_type::publish) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    // remaining_length
    if (auto vl_opt = insert_advance_variable_length(buf, remaining_length_buf_)) {
        remaining_length_ = *vl_opt;
    }
    else {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    if (remaining_length_ != buf.size()) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    // topic_name_length
    if (!insert_advance(buf, topic_name_length_buf_)) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    auto topic_name_length = endian_load<std::uint16_t>(topic_name_length_buf_.data());

    // topic_name
    if (buf.size() < topic_name_length) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    topic_name_ = buf.substr(0, topic_name_length);

    if (!utf8string_check(topic_name_)) {
        ec = make_error_code(
            disconnect_reason_code::topic_name_invalid
        );
        return;
    }

    buf.remove_prefix(topic_name_length);

    // packet_id
    switch (qos_value) {
    case qos::at_most_once:
        endian_store(typename basic_packet_id_type<PacketIdBytes>::type{0}, packet_id_.data());
        break;
    case qos::at_least_once:
    case qos::exactly_once:
        if (!copy_advance(buf, packet_id_)) {
            ec = make_error_code(
                disconnect_reason_code::protocol_error
            );
            return;
        }
        break;
    default:
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    };

    // payload
    if (!buf.empty()) {
        payloads_.emplace_back(force_move(buf));
    }
}
template <std::size_t PacketIdBytes>
inline
std::ostream& operator<<(std::ostream& o, basic_publish_packet<PacketIdBytes> const& v) {
    o << "v3_1_1::publish{" <<
        "topic:" << v.topic() << "," <<
        "qos:" << v.opts().get_qos() << "," <<
        "retain:" << v.opts().get_retain() << "," <<
        "dup:" << v.opts().get_dup();
    if (v.opts().get_qos() == qos::at_least_once ||
        v.opts().get_qos() == qos::exactly_once) {
        o << ",pid:" << v.packet_id();
    }
#if defined(ASYNC_MQTT_PRINT_PAYLOAD)
    o << ",payload:";
    for (auto const& e : v.payload_as_buffer()) {
        o << json_like_out(e);
    }
#endif // defined(ASYNC_MQTT_PRINT_PAYLOAD)
    o << "}";
    return o;
}

} // namespace async_mqtt::v3_1_1

#endif // ASYNC_MQTT_PACKET_IMPL_V3_1_1_PUBLISH_HPP
