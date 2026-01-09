#include <exec.hpp>

#include <format>
#include <print>
#include <string>

int main() {
    exec::sender auto sndr = exec::just(42) |
                             exec::then([](int value) -> float { return static_cast<float>(value) * 2.5f; }) |
                             exec::then([](float value) -> std::string { return std::format("Value is {}!", value); });

    auto [value] = exec::sync_wait(sndr).value();
    std::println("{}", value);

    return 0;
}