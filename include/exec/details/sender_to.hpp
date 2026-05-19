#ifndef EXEC_DETAILS_SENDER_TO_HPP
#define EXEC_DETAILS_SENDER_TO_HPP

#include "exec/completion_signatures.hpp"
#include "exec/connect.hpp"
#include "exec/env.hpp"
#include "exec/sender.hpp"

#include "exec/details/receiver_of.hpp"

#include <utility>

namespace exec::details {
    template<typename SenderT, typename ReceiverT>
    concept sender_to =
        sender_in<SenderT, env_of_t<ReceiverT>> &&
        receiver_of<ReceiverT, completion_signatures_of_t<SenderT, env_of_t<ReceiverT>>> &&
        requires(SenderT&& sndr, ReceiverT&& rcvr) {
            connect(std::forward<SenderT>(sndr), std::forward<ReceiverT>(rcvr));
        };
}

#endif // !EXEC_DETAILS_SENDER_TO_HPP
