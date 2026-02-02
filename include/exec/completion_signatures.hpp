#ifndef EXEC_COMPLETION_SIGNATURES_HPP
#define EXEC_COMPLETION_SIGNATURES_HPP

#include "exec/env.hpp"

#include <utility>

namespace exec {
    template<typename...>
    struct completion_signatures {};

    struct get_completion_signatures_t {
        template<typename SenderT, typename EnvT>
        [[nodiscard]] constexpr decltype(auto) operator()(SenderT&& sndr, EnvT&& env) const noexcept
        requires requires{ std::forward<SenderT>(sndr).get_completion_signatures(std::forward<EnvT>(env)); }
        {
            return std::forward<SenderT>(sndr).get_completion_signatures(std::forward<EnvT>(env));
        }

        template<typename SenderT, typename EnvT>
        requires requires{ typename std::remove_cvref_t<SenderT>::completion_signatures; }
        [[nodiscard]] constexpr auto operator()(const SenderT&, const EnvT&) const noexcept {
            return typename std::remove_cvref_t<SenderT>::completion_signatures{};
        }
    };
    inline constexpr get_completion_signatures_t get_completion_signatures{};

    template<typename SenderT, typename EnvT = empty_env>
    using completion_signatures_of_t = std::invoke_result_t<get_completion_signatures_t, SenderT, EnvT>;
}

#endif // !EXEC_COMPLETION_SIGNATURES_HPP