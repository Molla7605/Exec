#ifndef EXEC_DETAILS_META_REVERSE_HPP
#define EXEC_DETAILS_META_REVERSE_HPP

#include "exec/details/type_list.hpp"

namespace exec::details {
    template<typename...>
    struct meta_reverse {};

    template<template<typename...> typename TypeListT, typename... ResultTs>
    struct meta_reverse<TypeListT<ResultTs...>, TypeListT<>> {
        using type = TypeListT<ResultTs...>;
    };

    template<template<typename...> typename TypeListT, typename... ResultTs, typename CurrentT, typename... CandidateTs>
    struct meta_reverse<TypeListT<ResultTs...>, TypeListT<CurrentT, CandidateTs...>> {
        using type = meta_reverse<TypeListT<CurrentT, ResultTs...>, TypeListT<CandidateTs...>>::type;
    };

    template<valid_type_list T>
    using meta_reverse_t = meta_reverse<empty_list_of_t<T>, T>::type;
}

#endif // !EXEC_DETAILS_META_REVERSE_HPP