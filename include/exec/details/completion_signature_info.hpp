#ifndef EXEC_DETAILS_COMPLETION_SIGNATURE_INFO_HPP
#define EXEC_DETAILS_COMPLETION_SIGNATURE_INFO_HPP

#include "exec/completions.hpp"

#include "exec/details/indirect_meta_apply.hpp"

namespace exec::details {
    template<typename>
    struct completion_tag_of {};

    template<typename TagT, typename... ArgTs>
    struct completion_tag_of<TagT(ArgTs...)> {
        using type = TagT;
    };

    template<typename CompletionSignatureT>
    using completion_tag_of_t = completion_tag_of<CompletionSignatureT>::type;

    template<typename>
    struct completion_args_of {};

    template<typename TagT, typename... ArgTs>
    struct completion_args_of<TagT(ArgTs...)> {
        template<template<typename...> typename T>
        using apply = T<ArgTs...>;

        template<template<typename...> typename T>
        using indirect_apply = indirect_meta_apply_t<T, ArgTs...>;
    };

    template<typename T>
    concept valid_completion_fn =
        std::is_function_v<T> &&
        (
            std::is_same_v<completion_tag_of_t<T>, set_value_t> ||
            (std::is_same_v<completion_tag_of_t<T>, set_error_t> &&
             std::is_same_v<typename completion_args_of<T>::template apply<std::index_sequence_for>, std::index_sequence<0>>) ||
            (std::is_same_v<completion_tag_of_t<T>, set_stopped_t> &&
             std::is_same_v<typename completion_args_of<T>::template apply<std::index_sequence_for>, std::index_sequence<>>)
        );

}

#endif // !EXEC_DETAILS_COMPLETION_SIGNATURE_INFO_HPP