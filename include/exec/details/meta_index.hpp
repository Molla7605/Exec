#ifndef EXEC_DETAILS_META_INDEX_HPP
#define EXEC_DETAILS_META_INDEX_HPP

#include "exec/details/type_list.hpp"

#include <cstddef>

namespace exec::details {
    namespace _meta_index {
        template<typename...>
        struct container {};

        template<typename SizeT, SizeT INDEX>
        struct index {
            static constexpr SizeT value = INDEX;
        };
    }

    template<typename...>
    struct meta_index {};

    template<typename SizeT, SizeT TARGET_INDEX, SizeT INDEX, typename CurrentT, typename... CandidateTs>
    requires (TARGET_INDEX == INDEX)
    struct meta_index<_meta_index::index<SizeT, TARGET_INDEX>, _meta_index::index<SizeT, INDEX>, CurrentT, CandidateTs...> {
        using type = CurrentT;
    };

    template<typename SizeT, SizeT TARGET_INDEX, SizeT INDEX, typename CurrentT, typename... CandidateTs>
    struct meta_index<_meta_index::index<SizeT, TARGET_INDEX>, _meta_index::index<SizeT, INDEX>, CurrentT, CandidateTs...> {
        using type = meta_index<_meta_index::index<SizeT, TARGET_INDEX>, _meta_index::index<SizeT, ++INDEX>, CandidateTs...>::type;
    };

    template<template<typename...> typename TypeListT, typename... ElementTs, typename SizeT, SizeT TARGET_INDEX>
    struct meta_index<TypeListT<ElementTs...>, _meta_index::index<SizeT, TARGET_INDEX>> {
        using type = meta_index<_meta_index::index<SizeT, TARGET_INDEX>, _meta_index::index<SizeT, 0>, ElementTs...>::type;
    };

    template<size_t INDEX, valid_type_list ListT>
    using meta_index_of_t = meta_index<ListT, _meta_index::index<size_t, INDEX>>::type;

    template<size_t INDEX, typename... Ts>
    using meta_index_t = meta_index_of_t<INDEX, _meta_index::container<Ts...>>;
}

#endif // !EXEC_DETAILS_META_INDEX_HPP