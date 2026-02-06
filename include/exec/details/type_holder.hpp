#ifndef EXEC_DETAILS_TYPE_HOLDER_HPP
#define EXEC_DETAILS_TYPE_HOLDER_HPP

#include "exec/details/type_list.hpp"

#include <type_traits>

namespace exec::details {
    template<typename...>
    struct type_holder {};

    template<valid_type_list>
    struct is_type_holder : std::false_type {};

    template<typename... Ts>
    struct is_type_holder<type_holder<Ts...>> : std::true_type {};

    template<typename T>
    concept valid_type_holder = is_type_holder<std::remove_cvref_t<T>>::value;
}

#endif // !EXEC_DETAILS_TYPE_HOLDER_HPP