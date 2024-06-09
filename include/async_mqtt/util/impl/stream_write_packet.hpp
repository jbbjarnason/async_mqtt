// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_STREAM_WRITE_PACKET_HPP)
#define ASYNC_MQTT_IMPL_STREAM_WRITE_PACKET_HPP

#include <async_mqtt/util/stream.hpp>

namespace async_mqtt {

template <typename NextLayer>
template <typename Packet>
struct stream<NextLayer>::stream_write_packet_op {
    using stream_type = this_type;
    using stream_type_sp = std::shared_ptr<stream_type>;
    using next_layer_type = stream_type::next_layer_type;

    stream_type& strm;
    std::shared_ptr<Packet> packet;
    std::size_t size = packet->size();
    stream_type_sp life_keeper = strm.shared_from_this();
    enum { dispatch, post, write, bulk_write, complete } state = dispatch;

    template <typename Self>
    void operator()(
        Self& self
    ) {
        switch (state) {
        case dispatch: {
            state = post;
            auto& a_strm{strm};
            as::dispatch(
                a_strm.get_executor(),
                force_move(self)
            );
        } break;
        case post: {
            auto& a_strm{strm};
            auto& a_packet{*packet};
            if (!a_strm.bulk_write_ || a_strm.write_queue_.immediate_executable()) {
                state = write;
            }
            else {
                state = bulk_write;
                auto cbs = a_packet.const_buffer_sequence();
                std::copy(cbs.begin(), cbs.end(), std::back_inserter(a_strm.storing_cbs_));
            }
            a_strm.write_queue_.post(
                force_move(self)
            );
        } break;
        case write: {
            strm.write_queue_.start_work();
            if (strm.lowest_layer().is_open()) {
                state = complete;
                auto& a_strm{strm};
                auto& a_packet{*packet};
                if constexpr (
                    has_async_write<next_layer_type>::value) {
                    layer_customize<next_layer_type>::async_write(
                        a_strm.nl_,
                        a_packet.const_buffer_sequence(),
                        force_move(self)
                    );
                }
                else {
                    async_write(
                        a_strm.nl_,
                        a_packet.const_buffer_sequence(),
                        force_move(self)
                    );
                }
            }
            else {
                state = complete;
                auto& a_strm{strm};
                as::dispatch(
                    a_strm.get_executor(),
                    as::append(
                        force_move(self),
                        errc::make_error_code(errc::connection_reset),
                        0
                    )
                );
            }
        } break;
        case bulk_write: {
            strm.write_queue_.start_work();
            if (strm.lowest_layer().is_open()) {
                state = complete;
                auto& a_strm{strm};
                if (a_strm.storing_cbs_.empty()) {
                    auto& a_strm{strm};
                    auto& a_size{size};
                    as::dispatch(
                        a_strm.get_executor(),
                        as::append(
                            force_move(self),
                            errc::make_error_code(errc::success),
                            a_size
                        )
                    );
                }
                else {
                    a_strm.sending_cbs_ = force_move(a_strm.storing_cbs_);
                    if constexpr (
                        has_async_write<next_layer_type>::value) {
                        layer_customize<next_layer_type>::async_write(
                            a_strm.nl_,
                            a_strm.sending_cbs_,
                            force_move(self)
                        );
                    }
                    else {
                        async_write(
                            a_strm.nl_,
                            a_strm.sending_cbs_,
                            force_move(self)
                        );
                    }
                }
            }
            else {
                state = complete;
                auto& a_strm{strm};
                as::dispatch(
                    a_strm.get_executor(),
                    as::append(
                        force_move(self),
                        errc::make_error_code(errc::connection_reset),
                        0
                    )
                );
            }
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }

    template <typename Self>
    void operator()(
        Self& self,
        error_code ec,
        std::size_t bytes_transferred
    ) {
        if (ec) {
            strm.write_queue_.stop_work();
            auto& a_strm{strm};
            as::post(
                a_strm.get_executor(),
                [&a_strm, life_keeper = life_keeper] {
                    a_strm.write_queue_.poll_one();
                }
            );
            self.complete(ec, bytes_transferred);
            return;
        }
        switch (state) {
        case complete: {
            strm.write_queue_.stop_work();
            strm.sending_cbs_.clear();
            auto& a_strm{strm};
            as::post(
                a_strm.get_executor(),
                [&a_strm, life_keeper = life_keeper] {
                    a_strm.write_queue_.poll_one();
                }
            );
            self.complete(ec, size);
        } break;
        default:
            BOOST_ASSERT(false);
            break;
        }
    }
};

template <typename NextLayer>
template <typename Packet, typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code)
)
stream<NextLayer>::async_write_packet(
    Packet packet,
    CompletionToken&& token
) {
    return
        as::async_compose<
            CompletionToken,
            void(error_code, std::size_t)
        >(
            stream_write_packet_op<Packet>{
                *this,
                std::make_shared<Packet>(force_move(packet))
            },
            token,
            get_executor()
        );
}
} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_STREAM_WRITE_PACKET_HPP
