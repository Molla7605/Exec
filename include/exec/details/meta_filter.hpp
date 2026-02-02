#ifndef EXEC_DETAILS_META_FILTER_HPP
#define EXEC_DETAILS_META_FILTER_HPP

#include "exec/details/meta_bind.hpp"
#include "exec/details/type_list.hpp"

#include <type_traits>

namespace exec::details {
    namespace _meta_filter {
        template<typename...>
        struct candidates {};
    }

    template<template<typename...> typename, typename...>
    struct meta_filter {};

    template<template<typename...> typename FilterT, typename Target, typename ListT>
    struct meta_filter<FilterT, Target, ListT, _meta_filter::candidates<>> {
        using type = ListT;
    };

    template<template<typename...> typename FilterT, typename Target, typename ListT, typename CurrentT, typename... CandidateTs>
    requires FilterT<Target, CurrentT>::value
    struct meta_filter<FilterT, Target, ListT, _meta_filter::candidates<CurrentT, CandidateTs...>> {
        using type = meta_filter<FilterT, Target, meta_bind_back_t<ListT, CurrentT>, _meta_filter::candidates<CandidateTs...>>::type;
    };

    template<template<typename...> typename FilterT, typename Target, typename ListT, typename CurrentT, typename... CandidateTs>
    struct meta_filter<FilterT, Target, ListT, _meta_filter::candidates<CurrentT, CandidateTs...>> {
        using type = meta_filter<FilterT, Target, ListT, _meta_filter::candidates<CandidateTs...>>::type;
    };

    template<typename TargetT, valid_type_list SourceT, template<typename...> typename FilterT = std::is_same>
    using meta_filter_t =
        meta_filter<FilterT, TargetT, empty_list_of_t<SourceT>, typename elements_of<SourceT>::template apply<_meta_filter::candidates>>::type;
}

#endif // !EXEC_DETAILS_META_FILTER_HPP