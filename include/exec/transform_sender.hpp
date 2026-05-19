#ifndef EXEC_TRANSFORM_SENDER_HPP
#define EXEC_TRANSFORM_SENDER_HPP

#include "exec/domain.hpp"
#include "exec/sender.hpp"
#include "exec/queryable.hpp"

#include "exec/details/env_traits.hpp"

namespace exec::details {
    template<typename DomainT, typename TagT, typename SenderT, typename EnvT>
    requires requires { std::declval<DomainT>().transform_sender(TagT{}, std::declval<SenderT>(), std::declval<EnvT>()); }
    [[nodiscard]] constexpr decltype(auto) transform_sender(DomainT domain, TagT tag, SenderT&& sender, const EnvT& env)
        noexcept(noexcept(domain.transform_sender(tag, std::declval<SenderT>(), env)))
    {
        return domain.transform_sender(tag, std::forward<SenderT>(sender), env);
    }

    template<typename DomainT, typename TagT, typename SenderT, typename EnvT>
    [[nodiscard]] constexpr decltype(auto) transform_sender(DomainT domain, TagT tag, SenderT&& sender, const EnvT& env)
        noexcept(noexcept(default_domain{}.transform_sender(tag, std::declval<SenderT>(), std::declval<EnvT>())))
    {
        return default_domain{}.transform_sender(tag, std::forward<SenderT>(sender), std::forward<EnvT>(env));
    }

    template<typename DomainT, typename TagT, typename SenderT, typename EnvT>
    [[nodiscard]] constexpr decltype(auto) transform_recurse(DomainT domain, TagT tag, SenderT&& sender, const EnvT& env) {
        using sender2_t =
            decltype(transform_sender(domain, tag, std::declval<SenderT>(), std::declval<EnvT>()));

        if constexpr (std::is_same_v<std::remove_cvref_t<sender2_t>, std::remove_cvref_t<SenderT>>) {
            return transform_sender(domain, tag, std::forward<SenderT>(sender), env);
        }
        else {
            auto sender2 = transform_sender(domain, tag, std::forward<SenderT>(sender), env);
            if constexpr (std::is_same_v<TagT, start_t>) {
                auto domain2 = domain;

                return transform_recurse(domain2, tag, std::move(sender2), env);
            }
            else {
                auto domain2 = get_completion_domain<>(get_env(sender2), env);

                return transform_recurse(domain2, tag, std::move(sender2), env);
            }
        }
    }
}

namespace exec {
    template<sender SenderT, queryable EnvT>
    [[nodiscard]] constexpr decltype(auto) transform_sender(SenderT&& sender, const EnvT& env)
        noexcept(
            noexcept(
                details::transform_sender(get_domain(env),
                                          exec::start,
                                          details::transform_recurse(get_completion_domain<>(get_env(sender), env),
                                                                     exec::set_value,
                                                                     std::declval<SenderT>(),
                                                                     env),
                                          env)
            )
        )
    {
        auto start_domain = get_domain(env);
        auto completion_domain = get_completion_domain<>(get_env(sender), env);

        auto make_temp_sender = [&]() {
            return details::transform_recurse(completion_domain, exec::set_value, std::forward<SenderT>(sender), env);
        };

        return details::transform_recurse(start_domain, exec::start, make_temp_sender(), env);
    }
}

#endif // !EXEC_TRANSFORM_SENDER_HPP