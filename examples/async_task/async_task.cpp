#include <exec.hpp>

#include <print>
#include <stop_token>
#include <thread>

int main() {
    exec::run_loop loop;
    std::jthread thread([&loop](std::stop_token st) {
        std::stop_callback sc(st, [&loop] { loop.finish(); });
        loop.run();
    });

    exec::sender auto sndr = exec::just() | exec::then([] { std::println("Hello from {} thread!", std::this_thread::get_id()); });

    exec::sync_wait(sndr).value();

    return 0;
}