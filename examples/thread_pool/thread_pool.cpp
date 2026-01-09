#include <exec.hpp>

#include <cstddef>
#include <print>
#include <stop_token>
#include <string_view>
#include <thread>
#include <vector>

class simple_thread_pool {
public:
    simple_thread_pool(std::size_t thread_count) {
        m_threads.reserve(thread_count);
        for (std::size_t i = 0; i < thread_count; ++i) {
            m_threads.emplace_back([this](std::stop_token st) {
                std::stop_callback sc(st, [this] { m_loop.finish(); });
                m_loop.run();
            });
        }
    }

    ~simple_thread_pool() {
        for (auto& thread : m_threads) {
            thread.request_stop();
        }
    }

    exec::scheduler auto get_scheduler() noexcept {
        return m_loop.get_scheduler();
    }

private:
    exec::run_loop m_loop;
    std::vector<std::jthread> m_threads;

};

int main() {
    simple_thread_pool pool{ std::thread::hardware_concurrency() };

    exec::sender auto sndr = exec::just("Hello") |
                             exec::then([](std::string_view str) -> std::string {
                                 std::println("[1] Take message \"{}\" at \"{}\"", str, std::this_thread::get_id());
                                 return { str.data() };
                             }) |
                             exec::continues_on(pool.get_scheduler()) |
                             exec::let_value([](std::string_view str) {
                                 std::println("[2] Take message \"{}\" at \"{}\"", str, std::this_thread::get_id());
                                 return exec::just(std::format("{} world!", str));
                             }) |
                             exec::continues_on(pool.get_scheduler()) |
                             exec::then([](std::string_view str) {
                                 std::println("[3] Take message \"{}\" at \"{}\"", str, std::this_thread::get_id());
                             });

    exec::sync_wait(sndr).value();

    return 0;
}