#ifndef EXEC_DETAILS_AS_TUPLE_HPP
#define EXEC_DETAILS_AS_TUPLE_HPP

#include "exec/details/decayed_tuple.hpp"

namespace exec::details {
    template<typename...>
    struct as_tuple {};

    template<typename TagT, typename... ArgTs>
    struct as_tuple<TagT(ArgTs...)> {
        using type = decayed_tuple<TagT, ArgTs...>;
    };

    template<typename SignatureT>
    using as_tuple_t = as_tuple<SignatureT>::type;
}

#endif // !EXEC_DETAILS_AS_TUPLE_HPP