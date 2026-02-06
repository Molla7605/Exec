#ifndef EXEC_DETAILS_META_ADD_HPP
#define EXEC_DETAILS_META_ADD_HPP

namespace exec::details {
    template<typename...>
    struct meta_add {};

    template<template<typename...> typename TypeListT, typename... ArgTs>
    struct meta_add<TypeListT<ArgTs...>> {
        using type = TypeListT<ArgTs...>;
    };

    template<template<typename...> typename TypeListT, typename... LArgTs, typename... RArgTs>
    struct meta_add<TypeListT<LArgTs...>, TypeListT<RArgTs...>> {
        using type = TypeListT<LArgTs..., RArgTs...>;
    };

    template<template<typename...> typename TypeListT, typename... LArgTs, typename... RArgTs, typename... ListTs>
    struct meta_add<TypeListT<LArgTs...>, TypeListT<RArgTs...>, ListTs...> {
        using type = meta_add<TypeListT<LArgTs..., RArgTs...>, ListTs...>::type;
    };

    template<typename... Ts>
    requires (sizeof...(Ts) > 0)
    using meta_add_t = meta_add<Ts...>::type;
}

#endif // !EXEC_DETAILS_META_ADD_HPP