#ifndef EXEC_STOPPED_AS_ERROR_HPP
#define EXEC_STOPPED_AS_ERROR_HPP

#include "exec/env.hpp"
#include "exec/sender.hpp"
#include "exec/stop_token.hpp"

#include "exec/details/write_env.hpp"

namespace exec {

    struct stopped_as_error_t {

        template<sender SenderT, typename ErrT>
        [[nodiscard]] constexpr auto operator()(SenderT&& sender, ErrT&& err) const {
            return details::make_sender(std::forward<SenderT>(sender),std::forward<ErrT>(err));
        }

        template<sender SenderT, typename EnvT>
        constexpr auto transform_sender(SenderT&& sender, EnvT&& env) const {

            auto&& [_, err, child] = sender;
            using E = decltype(auto(err));
            return let_stopped(
              std::forward_like<SenderT>(child),
              [err = std::forward_like<SenderT>(err)]() mutable noexcept(is_nothrow_move_constructible_v<E>) {
                return just_error(std::move(err));
              });
        }
    };
    inline constexpr stopped_as_error_t stopped_as_error{};
}

#endif //EXEC_STOPPED_AS_ERROR_HPP