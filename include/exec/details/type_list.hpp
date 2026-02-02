#ifndef EXEC_DETAILS_TYPE_LIST_HPP
#define EXEC_DETAILS_TYPE_LIST_HPP

#include <type_traits>

namespace exec::details {
    template<typename>
    struct is_type_list : std::false_type {};

    template<template<typename...> typename TypeListT, typename... Ts>
    struct is_type_list<TypeListT<Ts...>> : std::true_type {};

    template<typename T>
    concept valid_type_list = is_type_list<T>::value;

    template<valid_type_list>
    struct list_of {};

    template<template<typename...> typename TypeListT, typename... Ts>
    struct list_of<TypeListT<Ts...>> {
        template<typename... ArgTs>
        using type = TypeListT<ArgTs...>;
    };

    template<valid_type_list>
    struct elements_of {};

    template<template<typename...> typename TypeListT, typename... Ts>
    struct elements_of<TypeListT<Ts...>> {
        template<template<typename...> typename T>
        using apply = T<Ts...>;
    };

    template<typename T>
    using empty_list_of_t = list_of<T>::template type<>;
}

#endif // !EXEC_DETAILS_TYPE_LIST_HPP