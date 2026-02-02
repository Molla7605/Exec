#ifndef EXEC_DETAILS_VALID_COMPLETION_SIGNATURES_HPP
#define EXEC_DETAILS_VALID_COMPLETION_SIGNATURES_HPP

#include "exec/completions.hpp"
#include "exec/completion_signatures.hpp"

namespace exec::details {
    template<typename...>
    inline constexpr bool is_completion_signatures = false;

    template<typename... Ts>
    inline constexpr bool is_completion_signatures<completion_signatures<set_value_t(Ts...)>> = true;

    template<typename T>
    inline constexpr bool is_completion_signatures<completion_signatures<set_error_t(T)>> = true;

    template<>
    inline constexpr bool is_completion_signatures<completion_signatures<set_stopped_t()>> = true;

    template<typename... SignatureTs>
    inline constexpr bool is_completion_signatures<completion_signatures<SignatureTs...>> =
        (is_completion_signatures<completion_signatures<SignatureTs>> && ... && true);

    template<typename T>
    concept valid_completion_signatures = is_completion_signatures<std::remove_cvref_t<T>>;
}

#endif // !EXEC_DETAILS_VALID_COMPLETION_SIGNATURES_HPP