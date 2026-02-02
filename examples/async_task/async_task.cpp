#include <exec.hpp>

#include <print>
#include <stop_token>
#include <thread>
#include <tuple>

template<template<typename...> typename T, typename... Args>
using test = T<Args...>;

template<typename T>
concept is_class = std::is_class_v<T>;

int main() {
    exec::sender auto sndr = exec::just(42) |
                             exec::then([](int i) -> int { return i + 1; }) |
                             exec::then([](int i) -> float { return 10.0f; });

    exec::sender auto task = exec::starts_on

    using sender_type = decltype(sndr);
    using completion_signatures = exec::completion_signatures_of_t<sender_type, exec::env_of_t<sender_type>>;

    return 0;
}