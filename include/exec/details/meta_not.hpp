#ifndef EXEC_DETAILS_META_NOT_HPP
#define EXEC_DETAILS_META_NOT_HPP

namespace exec::details {
    template<template<typename...> typename, typename...>
    struct meta_not {};

    template<template<typename...> typename ExpT>
    struct meta_not<ExpT> {
        template<typename... Ts>
        using type = meta_not<ExpT, ExpT<Ts...>>;
    };

    template<template<typename...> typename ExpT, typename... Ts>
    struct meta_not<ExpT, ExpT<Ts...>> {
        static constexpr bool value = !ExpT<Ts...>::value;
    };
}

#endif // !EXEC_DETAILS_META_NOT_HPP