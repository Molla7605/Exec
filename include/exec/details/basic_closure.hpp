#ifndef EXEC_DETAILS_BASIC_CLOSURE_HPP
#define EXEC_DETAILS_BASIC_CLOSURE_HPP

#include "exec/sender_adapter_closure.hpp"

#include "exec/details/product_type.hpp"

#include <type_traits>
#include <utility>

namespace exec::details {
    template<typename TagT, typename... ArgTs>
    struct basic_closure : sender_adapter_closure<basic_closure<TagT, ArgTs...>> {
        template<typename Self, sender SenderT>
        [[nodiscard]] constexpr decltype(auto) operator()(this Self&& self, SenderT&& sender) {
            return std::forward_like<Self>(self.args).apply([&]<typename... Ts>(Ts&&... args) mutable {
                return TagT{}(std::forward<SenderT>(sender), std::forward<Ts>(args)...);
            });
        }

        product_type<ArgTs...> args;
    };

    template<typename TagT, typename... ArgTs>
    basic_closure(sender_adapter_closure<TagT>, product_type<ArgTs...>) -> basic_closure<TagT, std::decay_t<ArgTs>...>;
}

#endif // !EXEC_DETAILS_BASIC_CLOSURE_HPP