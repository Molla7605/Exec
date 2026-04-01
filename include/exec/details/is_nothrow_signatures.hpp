#ifndef EXEC_DETAILS_IS_NOTHROW_SIGNATURES_HPP
#define EXEC_DETAILS_IS_NOTHROW_SIGNATURES_HPP

#include "exec/completion_signatures.hpp"

#include "exec/details/valid_completion_signatures.hpp"

#include <type_traits>

namespace exec::details {
    template<template<typename...> typename EvalT, valid_completion_signatures SignaturesT, typename... DataTs>
    inline constexpr bool is_nothrow_signatures = false;

    template<template<typename...> typename EvalT, typename TagT, typename... ArgTs, typename... DataTs>
    inline constexpr bool is_nothrow_signatures<EvalT, completion_signatures<TagT(ArgTs...)>, DataTs...> =
        EvalT<DataTs..., ArgTs...>::value;

    template<template<typename...> typename EvalT, typename... SigTs, typename... DataTs>
    inline constexpr bool is_nothrow_signatures<EvalT, completion_signatures<SigTs...>, DataTs...> =
        (EvalT<DataTs..., SigTs>::value && ... && true);
}

#endif // !EXEC_DETAILS_IS_NOTHROW_SIGNATURES_HPP