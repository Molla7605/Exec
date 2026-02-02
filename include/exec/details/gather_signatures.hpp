#ifndef EXEC_DETAILS_GATHER_SIGNATURES_HPP
#define EXEC_DETAILS_GATHER_SIGNATURES_HPP

#include "exec/completion_signatures.hpp"

#include "exec/details/indirect_meta_apply.hpp"
#include "exec/details/select_signatures_by_tag.hpp"
#include "exec/details/signature_info.hpp"
#include "exec/details/valid_completion_signatures.hpp"

namespace exec::details {
    template<template<typename...> typename TupleT, template<typename...> typename VariantT>
    struct transform_signatures {
        template<typename... Ts>
        using apply = indirect_meta_apply_t<VariantT, typename signature_args_of<completion_signatures<Ts>>::template indirect_apply<TupleT>...>;
    };

    template<typename TagT,
             valid_completion_signatures SignatureT,
             template<typename...> typename TupleT,
             template<typename...> typename VariantT>
    using gather_signatures =
            elements_of<select_signatures_by_tag_t<TagT, SignatureT>>::template apply<
                transform_signatures<TupleT, VariantT>::template apply
            >;
}

#endif // !EXEC_DETAILS_GATHER_SIGNATURES_HPP