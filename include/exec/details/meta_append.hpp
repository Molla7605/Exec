#ifndef EXEC_DETAILS_META_APPEND_HPP
#define EXEC_DETAILS_META_APPEND_HPP

#include "exec/details/type_list.hpp"

namespace exec::details {
    template<valid_type_list>
    struct meta_append_front {};

    template<template<typename...> typename TypeListT, typename... Ts>
    struct meta_append_front<TypeListT<Ts...>> {
        template<typename... ArgTs>
        using type = TypeListT<ArgTs..., Ts...>;
    };

    template<valid_type_list T, typename... ArgTs>
    using meta_append_front_t = meta_append_front<T>::template type<ArgTs...>;

    template<valid_type_list>
    struct meta_append_back {};

    template<template<typename...> typename TypeListT, typename... Ts>
    struct meta_append_back<TypeListT<Ts...>> {
        template<typename... ArgTs>
        using type = TypeListT<Ts..., ArgTs...>;
    };

    template<valid_type_list T, typename... ArgTs>
    using meta_append_back_t = meta_append_back<T>::template type<ArgTs...>;
}

#endif // !EXEC_DETAILS_META_APPEND_HPP