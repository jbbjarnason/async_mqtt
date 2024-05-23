// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_PACKET_IMPL_V5_PINGRESP_HPP)
#define ASYNC_MQTT_PACKET_IMPL_V5_PINGRESP_HPP

#include <utility>
#include <numeric>

#include <boost/numeric/conversion/cast.hpp>

#include <async_mqtt/packet/v5_pingresp.hpp>
#include <async_mqtt/util/buffer.hpp>

#include <async_mqtt/util/move.hpp>
#include <async_mqtt/util/static_vector.hpp>
#include <async_mqtt/util/endian_convert.hpp>

#include <async_mqtt/packet/detail/fixed_header.hpp>

namespace async_mqtt::v5 {

namespace as = boost::asio;

inline
pingresp_packet::pingresp_packet()
    : all_(all_.capacity())
{
    all_[0] = static_cast<char>(detail::make_fixed_header(control_packet_type::pingresp, 0b0000));
    all_[1] = char(0);
}

inline
constexpr control_packet_type pingresp_packet::type() {
    return control_packet_type::pingresp;
}

inline
std::vector<as::const_buffer> pingresp_packet::const_buffer_sequence() const {
    std::vector<as::const_buffer> ret;

    ret.emplace_back(as::buffer(all_.data(), all_.size()));
    return ret;
}

inline
std::size_t pingresp_packet::size() const {
    return all_.size();
}

inline
constexpr std::size_t pingresp_packet::num_of_const_buffer_sequence() {
    return 1; // all
}

inline
pingresp_packet::pingresp_packet(buffer buf, error_code& ec) {
    // fixed_header
    if (buf.empty()) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    all_.push_back(buf.front());
    buf.remove_prefix(1);
    auto cpt_opt = get_control_packet_type_with_check(static_cast<std::uint8_t>(all_.back()));
    if (!cpt_opt || *cpt_opt != control_packet_type::pingresp) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }

    // remaining_length
    if (buf.empty()) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
    all_.push_back(buf.front());

    if (static_cast<std::uint8_t>(all_.back()) != 0) {
        ec = make_error_code(
            disconnect_reason_code::malformed_packet
        );
        return;
    }
}
inline
std::ostream& operator<<(std::ostream& o, pingresp_packet const& v) {
    (void)v;
    o <<
        "v5::pingresp{}";
    return o;
}

} // namespace async_mqtt::v5

#endif // ASYNC_MQTT_PACKET_IMPL_V5_PINGRESP_HPP
