#ifndef EXEC_DETAILS_CONDITIONAL_META_APPLY_HPP
#define EXEC_DETAILS_CONDITIONAL_META_APPLY_HPP

#include "exec/details/type_holder.hpp"

#include <type_traits>

namespace exec::details {
    template<template<typename...> typename, typename...>
    struct conditional_meta_apply {};

    template<template<typename...> typename TypeListT, typename... TrueTs, typename... FalseTs>
    struct conditional_meta_apply<TypeListT, std::true_type, type_holder<TrueTs...>, type_holder<FalseTs...>> {
        using type = TypeListT<TrueTs...>;
    };

    template<template<typename...> typename TypeListT, typename... TrueTs, typename... FalseTs>
    struct conditional_meta_apply<TypeListT, std::false_type, type_holder<TrueTs...>, type_holder<FalseTs...>> {
        using type = TypeListT<FalseTs...>;
    };

    template<bool CONDITION, template<typename...> typename TypeListT, valid_type_holder TrueT, valid_type_holder FalseT>
    using conditional_meta_apply_t = conditional_meta_apply<TypeListT, std::bool_constant<CONDITION>, TrueT, FalseT>::type;
}

#endif // !EXEC_DETAILS_CONDITIONAL_META_APPLY_HPP