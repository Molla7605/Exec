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

}

#endif // !EXEC_DETAILS_COMPLETION_SIGNATURE_INFO_HPP