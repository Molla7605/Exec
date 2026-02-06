#ifndef EXEC_DETAILS_TYPE_HOLDER_HPP
#define EXEC_DETAILS_TYPE_HOLDER_HPP

#include "exec/details/type_list.hpp"

namespace exec::details {
    template<typename...>
    struct type_holder {};

    template<typename T>
    inline constexpr bool is_type_holder = false;

    template<typename... Ts>
    inline constexpr bool is_type_holder<type_holder<Ts...>> = true;

    template<typename T>
    concept valid_type_holder = valid_type_list<T> && is_type_holder<T>;
}

#endif // !EXEC_DETAILS_TYPE_HOLDER_HPP