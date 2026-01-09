#include <exec.hpp>

#include <print>
#include <typeinfo>

int main() {
    {
        [[maybe_unused]] exec::sender auto sndr = exec::just(42);
        using sender_type = decltype(sndr);
        using completion_signatures = exec::completion_signatures_of_t<sender_type, exec::env_of_t<sender_type>>;

        std::println("Completion signatures of just(\"42\"): {}", typeid(completion_signatures).name());
    }
    {
        [[maybe_unused]] exec::sender auto sndr = exec::just_error(42);
        using sender_type = decltype(sndr);
        using completion_signatures = exec::completion_signatures_of_t<sender_type, exec::env_of_t<sender_type>>;

        std::println("Completion signatures of just_error(\"42\"): {}", typeid(completion_signatures).name());
    }
    {
        [[maybe_unused]] exec::sender auto sndr = exec::just(42) | exec::then([](int) -> float { return 2.5f; });
        using sender_type = decltype(sndr);
        using completion_signatures = exec::completion_signatures_of_t<sender_type, exec::env_of_t<sender_type>>;

        std::println("Completion signatures of just(\"42\") | then([](int) -> float {{ return 2.5f; }}): {}", typeid(completion_signatures).name());
    }
    {
        [[maybe_unused]] exec::sender auto sndr = exec::just_stopped() | exec::then([]() -> float { return 2.5f; });
        using sender_type = decltype(sndr);
        using completion_signatures = exec::completion_signatures_of_t<sender_type, exec::env_of_t<sender_type>>;

        std::println("Completion signatures of just_stopped() | then([]() -> float {{ return 2.5f; }}): {}", typeid(completion_signatures).name());
    }

    return 0;
}