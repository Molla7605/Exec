#ifndef EXEC_DETAILS_META_INDEX_HPP
#define EXEC_DETAILS_META_INDEX_HPP

#include "exec/details/type_holder.hpp"
#include "exec/details/type_list.hpp"

#include <cstddef>
#include <type_traits>

namespace exec::details {
    template<typename...>
    struct meta_index {};

    template<typename SizeT, SizeT TARGET_INDEX, SizeT INDEX>
    struct meta_index<std::integral_constant<SizeT, TARGET_INDEX>, std::integral_constant<SizeT, INDEX>> {
        static_assert("Index out of ranged.");
    };

    template<typename SizeT, SizeT TARGET_INDEX, SizeT INDEX, typename CurrentT, typename... CandidateTs>
    requires (TARGET_INDEX == INDEX)
    struct meta_index<std::integral_constant<SizeT, TARGET_INDEX>, std::integral_constant<SizeT, INDEX>, CurrentT, CandidateTs...> {
        using type = CurrentT;
    };

    template<typename SizeT, SizeT TARGET_INDEX, SizeT INDEX, typename CurrentT, typename... CandidateTs>
    struct meta_index<std::integral_constant<SizeT, TARGET_INDEX>, std::integral_constant<SizeT, INDEX>, CurrentT, CandidateTs...> {
        using type = meta_index<std::integral_constant<SizeT, TARGET_INDEX>, std::integral_constant<SizeT, INDEX + 1>, CandidateTs...>::type;
    };

    template<template<typename...> typename TypeListT, typename... ElementTs, typename SizeT, SizeT TARGET_INDEX>
    struct meta_index<TypeListT<ElementTs...>, std::integral_constant<SizeT, TARGET_INDEX>> {
        using type = meta_index<std::integral_constant<SizeT, TARGET_INDEX>, std::integral_constant<SizeT, 0>, ElementTs...>::type;
    };

    template<size_t INDEX, valid_type_list ListT>
    using meta_index_of_t = meta_index<ListT, std::integral_constant<size_t, INDEX>>::type;

    template<size_t INDEX, typename... Ts>
    using meta_index_t = meta_index_of_t<INDEX, type_holder<Ts...>>;
}

#endif // !EXEC_DETAILS_META_INDEX_HPP