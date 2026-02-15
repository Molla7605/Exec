#ifndef EXEC_QUERY_HPP
#define EXEC_QUERY_HPP

#include <concepts>

namespace exec {
    template<typename Type>
    concept queryable = std::destructible<Type>;
}

#endif // !EXEC_QUERY_HPP