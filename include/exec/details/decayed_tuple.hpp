#ifndef EXEC_DETAILS_DECAYED_TUPLE_HPP
#define EXEC_DETAILS_DECAYED_TUPLE_HPP

#include <tuple>
#include <type_traits>

namespace exec::details {
    template<typename... Ts>
    using decayed_tuple = std::tuple<std::decay_t<Ts>...>;
}

#endif // !EXEC_DETAILS_DECAYED_TUPLE_HPP