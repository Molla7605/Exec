#include <exec.hpp>

#include <print>
#include <random>
#include <ranges>
#include <stop_token>
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

    ~simple_thread_pool() noexcept {
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
    const uint32_t seed = std::random_device{}();

    simple_thread_pool pool{ std::thread::hardware_concurrency() };

    auto closure = exec::let_value([](int id, uint32_t seed) {
                        thread_local std::mt19937 gen{ seed };
                        thread_local std::uniform_int_distribution dist{ 500, 1500 };

                        uint32_t value = dist(gen);

                        std::println("[{}] Generate random number \"{}\" from {}", id, value, std::this_thread::get_id());

                        return exec::just(id, value);
                   }) |
                   exec::continues_on(pool.get_scheduler()) |
                   exec::then([](int id, uint32_t value) {
                       std::println("[{}] Sleep {}ms from {}", id, value, std::this_thread::get_id());
                       std::this_thread::sleep_for(std::chrono::milliseconds(value));
                   }) |
                   exec::let_error([](auto&&) noexcept {
                       return exec::just();
                   });

    exec::counting_scope scope;
    for (auto id : std::views::iota(1, 50)) {
        exec::spawn(exec::just(id, seed) | closure, scope.get_token());
    }

    scope.close();

    std::println("Waiting tasks from {}", std::this_thread::get_id());

    exec::sync_wait(scope.join());

    std::println("All tasks finished");

    return 0;
}