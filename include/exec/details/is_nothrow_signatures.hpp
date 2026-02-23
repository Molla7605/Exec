#ifndef EXEC_DETAILS_IS_NOTHROW_SIGNATURES_HPP
#define EXEC_DETAILS_IS_NOTHROW_SIGNATURES_HPP

#include "exec/completion_signatures.hpp"

#include "exec/details/valid_completion_signatures.hpp"

#include <type_traits>

namespace exec::details {
    template<typename InvocableT, valid_completion_signatures SignaturesT>
    inline constexpr bool is_nothrow_signatures = false;

    template<typename InvocableT, typename TagT, typename... ArgTs>
    inline constexpr bool is_nothrow_signatures<InvocableT, completion_signatures<TagT(ArgTs...)>> =
        std::is_nothrow_invocable_v<InvocableT, ArgTs...>;

    template<typename InvocableT, typename... SigTs>
    inline constexpr bool is_nothrow_signatures<InvocableT, completion_signatures<SigTs...>> =
        (is_nothrow_signatures<InvocableT, SigTs> && ... && (sizeof...(SigTs) > 0));
}

#endif // !EXEC_DETAILS_IS_NOTHROW_SIGNATURES_HPP