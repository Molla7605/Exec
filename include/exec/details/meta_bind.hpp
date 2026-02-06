#ifndef EXEC_DETAILS_META_BIND_HPP
#define EXEC_DETAILS_META_BIND_HPP

namespace exec::details {
    template<template<typename...> typename TypeListT, typename... Ts>
    struct meta_bind_front {
        template<typename... ArgTs>
        using type = TypeListT<Ts..., ArgTs...>;
    };

    template<template<typename...> typename TypeListT, typename... Ts>
    struct meta_bind_back {
        template<typename... ArgTs>
        using type = TypeListT<ArgTs..., Ts...>;
    };
}

#endif // !EXEC_DETAILS_META_BIND_HPP