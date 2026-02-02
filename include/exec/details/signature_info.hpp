#ifndef EXEC_DETAILS_SIGNATURE_INFO_HPP
#define EXEC_DETAILS_SIGNATURE_INFO_HPP

#include "exec/completion_signatures.hpp"

#include "exec/details/indirect_meta_apply.hpp"

namespace exec::details {
    template<typename>
    struct completion_tag_of {};

    template<typename TagT, typename... ArgTs>
    struct completion_tag_of<completion_signatures<TagT(ArgTs...)>> {
        using type = TagT;
    };

    template<typename SignatureT>
    using completion_tag_of_t = completion_tag_of<SignatureT>::type;

    template<typename>
    struct signature_args_of {};

    template<typename TagT, typename... ArgTs>
    struct signature_args_of<completion_signatures<TagT(ArgTs...)>> {
        template<template<typename...> typename T>
        using apply = T<ArgTs...>;

        template<template<typename...> typename T>
        using indirect_apply = indirect_meta_apply_t<T, ArgTs...>;
    };

    template<typename...>
    struct has_completion_tag {
        static constexpr bool value = false;
    };

    template<typename TagT, typename... SignatureTs>
    requires (std::is_same_v<TagT, completion_tag_of_t<completion_signatures<SignatureTs>>> || ... || false)
    struct has_completion_tag<TagT, completion_signatures<SignatureTs...>> {
        static constexpr bool value = true;
    };

    template<typename TagT, valid_completion_signatures SignatureT>
    inline constexpr bool has_completion_tag_v = has_completion_tag<TagT, SignatureT>::value;
}

#endif // !EXEC_DETAILS_SIGNATURE_INFO_HPP