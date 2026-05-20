#ifndef EXEC_TRANSFORM_SENDER_HPP
#define EXEC_TRANSFORM_SENDER_HPP

#include "exec/completions.hpp"
#include "exec/operation_state.hpp"
#include "exec/domain.hpp"
#include "exec/queryable.hpp"
#include "exec/sender.hpp"

namespace exec::details {
    template<typename DomainT, typename TagT, typename SenderT, typename EnvT>
    concept transformable =
        requires {
            std::declval<DomainT>().transform_sender(TagT{}, std::declval<SenderT>(), std::declval<EnvT>());
        };

    template<typename DomainT, typename TagT, typename SenderT, typename EnvT>
    struct transform_sender_impl {
        static constexpr bool nothrow =
            noexcept(std::declval<DomainT>().transform_sender(TagT{}, std::declval<SenderT>(), std::declval<EnvT>()));

        [[nodiscard]]
        constexpr decltype(auto) operator()(DomainT domain, TagT tag, SenderT&& sender, const EnvT& env) noexcept(nothrow) {
            return domain.transform_sender(tag, std::forward<SenderT>(sender), env);
        }
    };

    template<typename DomainT, typename TagT, typename SenderT, typename EnvT>
    requires transformable<DomainT, TagT, SenderT, EnvT>
    struct transform_sender_impl<DomainT, TagT, SenderT, EnvT> {
        static constexpr bool nothrow =
            noexcept(default_domain{}.transform_sender(TagT{}, std::declval<SenderT>(), std::declval<EnvT>()));

        [[nodiscard]]
        constexpr decltype(auto) operator()(DomainT, TagT tag, SenderT&& sender, const EnvT& env) noexcept(nothrow) {
            return default_domain{}.transform_sender(tag, std::forward<SenderT>(sender), env);
        }
    };

    template<typename DomainT, typename TagT, typename SenderT, typename EnvT>
    [[nodiscard]] constexpr decltype(auto) transform_recurse(DomainT domain, TagT tag, SenderT&& sender, const EnvT& env) {
        using transform_t = transform_sender_impl<DomainT, TagT, SenderT, EnvT>;

        using sender2_t =
            decltype(transform_t{}(domain, tag, std::declval<SenderT>(), std::declval<EnvT>()));

        if constexpr (std::is_same_v<std::remove_cvref_t<sender2_t>, std::remove_cvref_t<SenderT>>) {
            return transform_t{}(domain, tag, std::forward<SenderT>(sender), env);
        }
        else {
            auto sender2 = transform_t{}(domain, tag, std::forward<SenderT>(sender), env);

            if constexpr (std::is_same_v<TagT, exec::start_t>) {
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
    [[nodiscard]] constexpr decltype(auto) transform_sender(SenderT&& sender, const EnvT& env) {
        auto start_domain = get_domain(env);
        auto completion_domain = get_completion_domain<>(get_env(sender), env);

        auto make_temp_sender = [&]() {
            return details::transform_recurse(completion_domain, exec::set_value, std::forward<SenderT>(sender), env);
        };

        return details::transform_recurse(start_domain, exec::start, make_temp_sender(), env);
    }
}

#endif // !EXEC_TRANSFORM_SENDER_HPP