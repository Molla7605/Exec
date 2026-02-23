#ifndef EXEC_DETAILS_GATHER_SIGNATURES_HPP
#define EXEC_DETAILS_GATHER_SIGNATURES_HPP

#include "exec/completion_signatures.hpp"

#include "exec/details/indirect_meta_apply.hpp"
#include "exec/details/meta_filter.hpp"
#include "exec/details/signature_info.hpp"
#include "exec/details/valid_completion_signatures.hpp"

namespace exec::details {
    template<typename TagT, typename SigT>
    struct has_same_tag : std::is_same<TagT, completion_tag_of_t<completion_signatures<SigT>>> {};

    template<template<typename...> typename TupleT, template<typename...> typename VariantT>
    struct apply_signatures {
        template<typename... Ts>
        using apply = indirect_meta_apply_t<VariantT, typename signature_args_of<completion_signatures<Ts>>::template indirect_apply<TupleT>...>;
    };

    template<typename TagT,
             valid_completion_signatures SignatureT,
             template<typename...> typename TupleT,
             template<typename...> typename VariantT>
    using gather_signatures =
            elements_of<meta_filter_t<TagT, SignatureT, has_same_tag>>::template apply<
                apply_signatures<TupleT, VariantT>::template apply
            >;
}

#endif // !EXEC_DETAILS_GATHER_SIGNATURES_HPP