#ifndef EXEC_DOMAIN_HPP
#define EXEC_DOMAIN_HPP

#include "exec/env.hpp"
#include "exec/forwarding_query.hpp"
#include "exec/scheduler.hpp"
#include "exec/sender.hpp"

#include "exec/details/env_traits.hpp"
#include "exec/details/hide_sched.hpp"

namespace exec {
    struct default_domain {
        template<typename TagT, sender SenderT, queryable EnvT>
        requires requires { tag_of_t<SenderT>{}.transform_sender(TagT{}, std::declval<SenderT>(), std::declval<EnvT>()); }
        static constexpr decltype(auto) transform_sender(TagT, SenderT&& sender, const EnvT& env)
            noexcept(noexcept(tag_of_t<SenderT>{}.transform_sender(TagT{}, std::forward<SenderT>(sender), env)))
        {
            return tag_of_t<SenderT>{}.transform_sender(TagT{}, std::forward<SenderT>(sender), env);
        }

        template<typename SenderT>
        static constexpr decltype(auto) transform_sender(auto, SenderT&& sender, const auto&) noexcept {
            static_assert(exec::sender<SenderT>);

            return static_cast<SenderT>(std::forward<SenderT>(sender));
        }

        template<typename TagT, sender SenderT, typename... ArgTs>
        static constexpr decltype(auto) apply_sender(TagT, SenderT&& sender, ArgTs&&... args)
            noexcept(noexcept(TagT{}.apply_sender(std::forward<SenderT>(sender), std::forward<ArgTs>(args)...)))
        {
            TagT{}.apply_sender(std::forward<SenderT>(sender), std::forward<ArgTs>(args)...);
        }
    };

    template<typename CompletionT>
    struct get_completion_domain_t {
    private:
        template<typename EnvT, typename TagT, typename... ArgTs>
        static constexpr bool is_queryable = details::has_query<EnvT, TagT, ArgTs...> ||
                                             details::has_query<EnvT, TagT>;

    public:
        template<typename AttrsT, typename... EnvTs>
        [[nodiscard]] constexpr auto operator()(const AttrsT& attrs, EnvTs&&... envs) const noexcept {
            if constexpr (is_queryable<AttrsT, get_completion_domain_t, EnvTs...>) {
                return details::try_query(attrs, *this, std::forward<EnvTs>(envs)...);
            }
            else if constexpr (std::is_same_v<CompletionT, void>) {
                return get_completion_domain_t<exec::set_value_t>{}(attrs, std::forward<EnvTs>(envs)...);
            }
            else if constexpr (requires {
                    auto(get_completion_scheduler<CompletionT>(std::declval<AttrsT>(), std::declval<EnvTs>()...));
                    details::try_query(get_completion_scheduler<CompletionT>(std::declval<AttrsT>(),
                                                                             std::declval<EnvTs>()...),
                                       get_completion_domain_t<exec::set_value_t>{},
                                       std::declval<EnvTs>()...);
                })
            {
                return details::try_query(get_completion_scheduler<CompletionT>(attrs, std::forward<EnvTs>(envs)...),
                                          get_completion_domain_t<exec::set_value_t>{},
                                          std::forward<EnvTs>(envs)...);
            }
            else if constexpr (sizeof...(EnvTs) > 0 && scheduler<AttrsT>){
                return default_domain{};
            }
            else {
                static_assert(false);
            }
        }

        [[nodiscard]] static consteval bool query(forwarding_query_t) noexcept {
            return true;
        }

    };
    template<typename CompletionT = void>
    inline constexpr get_completion_domain_t<CompletionT> get_completion_domain{};

    struct get_domain_t {
        template<typename EnvT>
        [[nodiscard]] constexpr auto operator()(const EnvT& env) const noexcept {
            if constexpr (details::has_query<EnvT, get_domain_t>) {
                return auto(env.query(*this));
            }
            else if constexpr (details::has_query<EnvT, get_scheduler_t> &&
                               details::has_query<decltype(get_scheduler(std::declval<EnvT>())),
                                                  get_completion_domain_t<exec::set_value_t>,
                                                  decltype(details::hide_sched(std::declval<EnvT>()))>)
            {
                return get_completion_domain<exec::set_value_t>(get_scheduler(env), details::hide_sched(env));
            }
            else {
                return default_domain{};
            }
        }

        [[nodiscard]] static consteval bool query(forwarding_query_t) noexcept {
            return true;
        }
    };
    inline constexpr get_domain_t get_domain{};
}

#endif // !EXEC_DOMAIN_HPP
