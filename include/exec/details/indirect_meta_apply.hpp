#ifndef EXEC_DETAILS_INDIRECT_META_APPLY_HPP
#define EXEC_DETAILS_INDIRECT_META_APPLY_HPP

#include "exec/details/type_list.hpp"

namespace exec::details {
    template<bool>
    struct indirect_meta_apply {
        template<template<typename...> typename T, typename... ArgTs>
        using meta_apply = T<ArgTs...>;
    };

    template<typename...>
    concept always_true = true;

    template<template<typename...> typename T, typename... ArgTs>
    using indirect_meta_apply_t = indirect_meta_apply<always_true<ArgTs...>>::template meta_apply<T, ArgTs...>;

    template<valid_type_list>
    struct indirect_elements_of {};

    template<template<typename...> typename TypeListT, typename... Ts>
    struct indirect_elements_of<TypeListT<Ts...>> {
        template<template<typename...> typename T>
        using apply = indirect_meta_apply_t<T, Ts...>;
    };
}

#endif // !EXEC_DETAILS_INDIRECT_META_APPLY_HPP