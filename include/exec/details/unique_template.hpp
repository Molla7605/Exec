#ifndef EXEC_DETAILS_UNIQUE_TEMPLATE_HPP
#define EXEC_DETAILS_UNIQUE_TEMPLATE_HPP

#include "exec/details/meta_bind.hpp"
#include "exec/details/meta_reverse.hpp"

#include <type_traits>

namespace exec::details {
    namespace _unique_template {
        template<typename...>
        struct candidates {};
    }

    template<valid_type_list, typename...>
    struct unique_template {};

    template<typename ListT>
    struct unique_template<ListT, _unique_template::candidates<>> {
        using type = ListT;
    };

    template<typename ListT, typename CurrentT>
    struct unique_template<ListT, _unique_template::candidates<CurrentT>> {
        using type = meta_bind_front_t<ListT, CurrentT>;
    };

    template<typename ListT, typename... CandidateTs, typename CurrentT>
    requires (std::is_same_v<CurrentT, CandidateTs> || ... || false)
    struct unique_template<ListT, _unique_template::candidates<CurrentT, CandidateTs...>> {
        using type = unique_template<ListT, _unique_template::candidates<CandidateTs...>>::type;
    };

    template<typename ListT, typename... CandidateTs, typename CurrentT>
    struct unique_template<ListT, _unique_template::candidates<CurrentT, CandidateTs...>> {
        using type = unique_template<meta_bind_front_t<ListT, CurrentT>, _unique_template::candidates<CandidateTs...>>::type;
    };

    template<template<typename...> typename TypeListT, typename... CandidateTs>
    using unique_template_t = unique_template<TypeListT<>, meta_reverse_t<_unique_template::candidates<CandidateTs...>>>::type;

    template<valid_type_list T>
    using meta_unique_t = unique_template<empty_list_of_t<T>, typename elements_of<meta_reverse_t<T>>::template apply<_unique_template::candidates>>::type;
}

#endif // !EXEC_DETAILS_UNIQUE_TEMPLATE_HPP