#ifndef EXEC_COMPLETION_SIGNATURES_HPP
#define EXEC_COMPLETION_SIGNATURES_HPP

#include "exec/details/completion_signature_info.hpp"
#include "exec/details/meta_filter.hpp"
#include "exec/details/meta_function.hpp"
#include "exec/details/type_holder.hpp"

#include <cstddef>
#include <type_traits>

namespace exec {
    template<details::valid_completion_fn... CompletionFnTs>
    struct completion_signatures {
        template<typename TagT>
        [[nodiscard]] static constexpr std::size_t count_of(TagT) noexcept {
            using filter_t = details::meta_function<decltype(
                []<typename T, typename... ArgTs>(TagT, details::type_holder<T(ArgTs...)>) consteval {
                    return std::is_same_v<TagT, T>;
                })>;

            using fns_t = details::meta_filter_t<TagT,
                                                 details::type_holder<details::type_holder<CompletionFnTs>...>,
                                                 filter_t::template type>;

            return []<typename... Ts>(Ts...) consteval {
                return sizeof...(Ts);
            }(fns_t{});
        }
    };

    template<typename SenderT, typename... EnvTs>
    [[nodiscard]] consteval auto get_completion_signatures() {
        // TODO: using new_sender_t = decltype(transform_sender(std::declval<SenderT>(), std::declval<EnvTs>()...));
        using new_sender_t = SenderT;

        if constexpr (requires { std::remove_cvref_t<SenderT>::template get_completion_signatures<SenderT, EnvTs...>(); }) {
            return std::remove_cvref_t<new_sender_t>::template get_completion_signatures<new_sender_t, EnvTs...>();
        }
        else {
            return std::remove_cvref_t<new_sender_t>::template get_completion_signatures<new_sender_t>();
        }
    }

    template<typename SenderT, typename... EnvTs>
    using completion_signatures_of_t = decltype(get_completion_signatures<SenderT, EnvTs...>());
}

#endif // !EXEC_COMPLETION_SIGNATURES_HPP