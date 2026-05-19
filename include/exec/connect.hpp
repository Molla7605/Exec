#ifndef EXEC_CONNECT_HPP
#define EXEC_CONNECT_HPP

#include "exec/env.hpp"
#include "exec/transform_sender.hpp"

namespace exec {
    struct connect_t {
        template<typename SenderT, typename ReceiverT>
        [[nodiscard]] constexpr decltype(auto) operator()(SenderT&& sender, ReceiverT&& receiver) const
            noexcept(noexcept(transform_sender(std::declval<SenderT>(),
                                               get_env(std::declval<ReceiverT>())).connect(std::declval<ReceiverT>())))
        {
            return transform_sender(std::forward<SenderT>(sender), get_env(receiver)).connect(std::forward<ReceiverT>(receiver));
        }
    };
    inline constexpr connect_t connect{};

    template<typename SenderT, typename ReceiverT>
    using connect_result_t = decltype(connect(std::declval<SenderT>(), std::declval<ReceiverT>()));
}

#endif // !EXEC_CONNECT_HPP
