// Copyright Takatoshi Kondo 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(ASYNC_MQTT_IMPL_CLIENT_RECV_HPP)
#define ASYNC_MQTT_IMPL_CLIENT_RECV_HPP

#include <boost/asio/dispatch.hpp>

#include <async_mqtt/packet/packet_variant.hpp>
#include <async_mqtt/impl/client_impl.hpp>
#include <async_mqtt/util/log.hpp>

namespace async_mqtt {

template <protocol_version Version, typename NextLayer>
struct client<Version, NextLayer>::
recv_op {
    this_type& cl;
    enum { dispatch, recv, complete } state = dispatch;
    template <typename Self>
    void operator()(
        Self& self
    ) {
        if (state == dispatch) {
            state = recv;
            auto& a_cl{cl};
            as::dispatch(
                a_cl.ep_->get_executor(),
                force_move(self)
            );
        }
        else {
            BOOST_ASSERT(state == recv);
            state = complete;
            if (cl.recv_queue_.empty()) {
                cl.recv_queue_inserted_ = false;
                auto tim = std::make_shared<as::steady_timer>(
                    cl.ep_->get_executor()
                );
                cl.tim_notify_publish_recv_.expires_at(
                    std::chrono::steady_clock::time_point::max()
                );
                auto& a_cl{cl};
                a_cl.tim_notify_publish_recv_.async_wait(
                    force_move(self)
                );
            }
            else {
                auto [ec, pv] = force_move(cl.recv_queue_.front());
                cl.recv_queue_.pop_front();
                self.complete(
                    ec,
                    force_move(pv)
                );
            }
        }
    }

    template <typename Self>
    void operator()(
        Self& self,
        error_code /* ec */
    ) {
        BOOST_ASSERT(state == complete);
        if (cl.recv_queue_inserted_) {
            auto [ec, pv] = force_move(cl.recv_queue_.front());
            cl.recv_queue_.pop_front();
            self.complete(
                ec,
                force_move(pv)
            );
        }
        else {
            self.complete(
                make_error_code(errc::operation_canceled),
                packet_variant{}
            );
        }
    }
};

template <protocol_version Version, typename NextLayer>
template <typename CompletionToken>
BOOST_ASIO_INITFN_AUTO_RESULT_TYPE(
    CompletionToken,
    void(error_code, std::optional<publish_packet>, std::optional<disconnect_packet>)
)
client<Version, NextLayer>::async_recv(
    CompletionToken&& token
) {
    ASYNC_MQTT_LOG("mqtt_api", info)
        << ASYNC_MQTT_ADD_VALUE(address, this)
        << "recv";
    return
        as::async_compose<
            CompletionToken,
            void(error_code, packet_variant)
        >(
            recv_op{
                *this
            },
            token,
            get_executor()
        );
}

} // namespace async_mqtt

#endif // ASYNC_MQTT_IMPL_CLIENT_RECV_HPP
