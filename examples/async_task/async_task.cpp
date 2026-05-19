#include <exec.hpp>

int main() {
    exec::run_loop loop;
    exec::details::sync_wait_env env{ &loop };
    exec::empty_env env2;

    auto schd = exec::get_completion_scheduler<exec::set_value_t>(env2, env);

    return 0;
}