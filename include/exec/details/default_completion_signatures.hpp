#ifndef EXEC_DETAILS_DEFAULT_COMPLETION_SIGNATURES_HPP
#define EXEC_DETAILS_DEFAULT_COMPLETION_SIGNATURES_HPP

#include "exec/completions.hpp"
#include "exec/completion_signatures.hpp"

namespace exec::details {
    template<typename... Ts>
    using default_set_value_t = completion_signatures<set_value_t(Ts...)>;

    template<typename... Ts>
    using default_set_error_t = completion_signatures<set_error_t(Ts)...>;

    template<typename StoppedT>
    struct stopped_wrapper {
        template<typename...>
        using type = StoppedT;
    };
}

#endif // !EXEC_DETAILS_DEFAULT_COMPLETION_SIGNATURES_HPP