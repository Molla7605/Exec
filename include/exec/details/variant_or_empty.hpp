#ifndef EXEC_DETAILS_EMPTY_VARIANT_HPP
#define EXEC_DETAILS_EMPTY_VARIANT_HPP

#include <type_traits>
#include <variant>

namespace exec::details {
    struct empty_variant {
        empty_variant() = delete;
    };

    template<typename... Ts>
    struct variant_or_empty {
        using type = std::variant<std::decay_t<Ts>...>;
    };

    template<>
    struct variant_or_empty<> {
        using type = empty_variant;
    };
}

#endif // !EXEC_DETAILS_EMPTY_VARIANT_HPP