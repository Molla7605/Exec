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
    exec::sender auto task = exec::starts_on(loop.get_scheduler(), sndr);

    exec::sync_wait(task).value();

    return 0;
}