// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../common/test_main.hpp"
#include "../common/global_fixture.hpp"

#include <boost/lexical_cast.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)
struct v5_pubrel;
struct v5_pubrel_pid4;
struct v5_pubrel_pid_only;
struct v5_pubrel_pid_rc;
struct v5_pubrel_prop_len_last;
BOOST_AUTO_TEST_SUITE_END()

#include <async_mqtt/packet/v5_pubrel.hpp>
#include <async_mqtt/packet/packet_iterator.hpp>
#include <async_mqtt/packet/packet_traits.hpp>

BOOST_AUTO_TEST_SUITE(ut_packet)

namespace am = async_mqtt;

BOOST_AUTO_TEST_CASE(v5_pubrel) {
    BOOST_TEST(am::is_pubrel<am::v5::pubrel_packet>());
    BOOST_TEST(!am::is_v3_1_1<am::v5::pubrel_packet>());
    BOOST_TEST(am::is_v5<am::v5::pubrel_packet>());
    BOOST_TEST(am::is_client_sendable<am::v5::pubrel_packet>());
    BOOST_TEST(am::is_server_sendable<am::v5::pubrel_packet>());

    auto props = am::properties{
        am::property::reason_string("some reason")
    };
    auto p = am::v5::pubrel_packet{
        0x1234, // packet_id
        am::pubrel_reason_code::packet_identifier_not_found,
        props
    };
    BOOST_TEST(p.packet_id() == 0x1234);
    BOOST_TEST(p.code() == am::pubrel_reason_code::packet_identifier_not_found);
    BOOST_TEST(p.props() == props);

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x62,                               // fixed_header
            0x12,                               // remaining_length
            0x12, 0x34,                         // packet_id
            char(0x92),                         // reason_code
            0x0e,                               // property_length
            0x1f,                               // reason_string
            0x00, 0x0b, 0x73, 0x6f, 0x6d, 0x65, 0x20, 0x72, 0x65, 0x61, 0x73, 0x6f, 0x6e,
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v5::pubrel_packet{buf};
        BOOST_TEST(p.packet_id() == 0x1234);
        BOOST_TEST(p.code() == am::pubrel_reason_code::packet_identifier_not_found);
        BOOST_TEST(p.props() == props);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::pubrel{pid:4660,rc:packet_identifier_not_found,ps:[{id:reason_string,val:some reason}]}"
    );
}

BOOST_AUTO_TEST_CASE(v5_pubrel_pid4) {
    auto props = am::properties{
        am::property::reason_string("some reason")
    };
    auto p = am::v5::basic_pubrel_packet<4>{
        0x12345678, // packet_id
        am::pubrel_reason_code::packet_identifier_not_found,
        props
    };

    BOOST_TEST(p.packet_id() == 0x12345678);
    BOOST_TEST(p.code() == am::pubrel_reason_code::packet_identifier_not_found);
    BOOST_TEST(p.props() == props);

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x62,                               // fixed_header
            0x14,                               // remaining_length
            0x12, 0x34, 0x56, 0x78,             // packet_id
            char(0x92),                         // reason_code
            0x0e,                               // property_length
            0x1f,                               // reason_string
            0x00, 0x0b, 0x73, 0x6f, 0x6d, 0x65, 0x20, 0x72, 0x65, 0x61, 0x73, 0x6f, 0x6e,
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v5::basic_pubrel_packet<4>{buf};
        BOOST_TEST(p.packet_id() == 0x12345678);
        BOOST_TEST(p.code() == am::pubrel_reason_code::packet_identifier_not_found);
        BOOST_TEST(p.props() == props);

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::pubrel{pid:305419896,rc:packet_identifier_not_found,ps:[{id:reason_string,val:some reason}]}"
    );
}

BOOST_AUTO_TEST_CASE(v5_pubrel_pid_only) {
    auto p = am::v5::pubrel_packet{
        0x1234 // packet_id
    };
    BOOST_TEST(p.code() == am::pubrel_reason_code::success);
    BOOST_TEST(p.props().empty());
    BOOST_TEST(p.packet_id() == 0x1234);

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x62,                               // fixed_header
            0x02,                               // remaining_length
            0x12, 0x34,                         // packet_id
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v5::pubrel_packet{buf};
        BOOST_TEST(p.packet_id() == 0x1234);
        BOOST_TEST(p.code() == am::pubrel_reason_code::success);
        BOOST_TEST(p.props().empty());

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::pubrel{pid:4660}"
    );
}

BOOST_AUTO_TEST_CASE(v5_pubrel_pid_rc) {
    auto p = am::v5::pubrel_packet{
        0x1234, // packet_id
        am::pubrel_reason_code::success
    };
    BOOST_TEST(p.code() == am::pubrel_reason_code::success);
    BOOST_TEST(p.props().empty());
    BOOST_TEST(p.packet_id() == 0x1234);

    {
        auto cbs = p.const_buffer_sequence();
        BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
        char expected[] {
            0x62,                               // fixed_header
            0x03,                               // remaining_length
            0x12, 0x34,                         // packet_id
            0x00,                               // reason_code
        };
        auto [b, e] = am::make_packet_range(cbs);
        BOOST_TEST(std::equal(b, e, std::begin(expected)));

        auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
        auto p = am::v5::pubrel_packet{buf};
        BOOST_TEST(p.packet_id() == 0x1234);
        BOOST_TEST(p.code() == am::pubrel_reason_code::success);
        BOOST_TEST(p.props().empty());

        auto cbs2 = p.const_buffer_sequence();
        BOOST_TEST(cbs2.size() == p.num_of_const_buffer_sequence());
        auto [b2, e2] = am::make_packet_range(cbs2);
        BOOST_TEST(std::equal(b2, e2, std::begin(expected)));
    }
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::pubrel{pid:4660,rc:success}"
    );
}

BOOST_AUTO_TEST_CASE(v5_pubrel_prop_len_last) {
    char expected[] {
        0x62,                               // fixed_header
        0x04,                               // remaining_length
        0x12, 0x34,                         // packet_id
        0x00,                               // reason_code
        0x00,                               // property_length
    };
    auto buf = am::allocate_buffer(std::begin(expected), std::end(expected));
    auto p = am::v5::pubrel_packet{buf};
    BOOST_TEST(p.packet_id() == 0x1234);
    BOOST_TEST(p.code() == am::pubrel_reason_code::success);
    BOOST_TEST(p.props().empty());

    auto cbs = p.const_buffer_sequence();
    BOOST_TEST(cbs.size() == p.num_of_const_buffer_sequence());
    auto [b, e] = am::make_packet_range(cbs);
    BOOST_TEST(std::equal(b, e, std::begin(expected)));
    BOOST_TEST(
        boost::lexical_cast<std::string>(p) ==
        "v5::pubrel{pid:4660,rc:success}"
    );
}

BOOST_AUTO_TEST_SUITE_END()
