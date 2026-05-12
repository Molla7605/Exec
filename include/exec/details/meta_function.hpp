#ifndef EXEC_DETAILS_META_FUNCTION_HPP
#define EXEC_DETAILS_META_FUNCTION_HPP

#include "exec/details/type_holder.hpp"

#include <concepts>

namespace exec::details {
    template<typename...>
    struct meta_function {};

    template<typename InvocableT, typename... ArgTs>
    struct meta_function<InvocableT, type_holder<ArgTs...>> {
        static constexpr auto value = InvocableT{}(ArgTs{}...);
    };

    template<typename InvocableT>
    struct meta_function<InvocableT> {
        template<typename... ArgTs>
        requires std::invocable<InvocableT, ArgTs...>
        using type = meta_function<InvocableT, type_holder<ArgTs...>>;
    };
}

#endif // !EXEC_DETAILS_META_FUNCTION_HPP
