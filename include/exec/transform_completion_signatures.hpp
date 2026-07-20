#ifndef EXEC_TRANSFORM_COMPLETION_SIGNATURES_HPP
#define EXEC_TRANSFORM_COMPLETION_SIGNATURES_HPP

#include "exec/env.hpp"
#include "exec/sender.hpp"

#include "exec/details/decayed_tuple.hpp"
#include "exec/details/default_completion_signatures.hpp"
#include "exec/details/gather_signatures.hpp"
#include "exec/details/variant_or_empty.hpp"

namespace exec {
    template<typename SenderT,
             typename EnvT = empty_env,
             template<typename...> typename TupleT = details::decayed_tuple,
             template<typename...> typename VariantT = details::variant_or_empty>
    requires sender_in<SenderT, EnvT>
    using value_types_of_t = details::gather_signatures<set_value_t, completion_signatures_of_t<SenderT, EnvT>, TupleT, VariantT>;

    template<typename SenderT,
             typename EnvT = empty_env,
             template<typename...> typename VariantT = details::variant_or_empty>
    requires sender_in<SenderT, EnvT>
    using error_types_of_t =
        details::gather_signatures<set_error_t, completion_signatures_of_t<SenderT, EnvT>, std::type_identity_t, VariantT>;

    template<typename SenderT, typename EnvT = empty_env>
    requires sender_in<SenderT, EnvT>
    inline constexpr bool sends_stopped =
        !std::is_same_v<completion_signatures<>,
                        details::gather_signatures<set_stopped_t,
                                                   completion_signatures_of_t<SenderT, EnvT>,
                                                   details::stopped_wrapper<set_stopped_t()>::type,
                                                   completion_signatures>>;
}

#endif // !EXEC_TRANSFORM_COMPLETION_SIGNATURES_HPP