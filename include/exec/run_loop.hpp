#ifndef EXEC_RUN_LOOP_HPP
#define EXEC_RUN_LOOP_HPP

#include "exec/completions.hpp"
#include "exec/env.hpp"
#include "exec/receiver.hpp"
#include "exec/sender.hpp"
#include "exec/completion_signatures.hpp"
#include "exec/operation_state.hpp"
#include "exec/scheduler.hpp"
#include "exec/stop_token.hpp"

#include <atomic>
#include <exception>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <utility>

namespace exec {
    class run_loop {
        struct operation_state_base {
            virtual ~operation_state_base() noexcept = default;

            virtual void run() noexcept = 0;
        };

        template<receiver ReceiverT>
        struct operation_state : operation_state_base {
            using operation_state_concept = operation_state_t;

            explicit operation_state(run_loop* loop, receiver auto&& receiver) noexcept :
                loop(loop),
                receiver(std::forward<decltype(receiver)>(receiver)) {}

            run_loop* loop;
            ReceiverT receiver;

            void run() noexcept override {
                if (get_stop_token(get_env(receiver)).stop_requested()) {
                    set_stopped(std::move(receiver));
                }
                else {
                    set_value(std::move(receiver));
                }
            }

            void start() noexcept {
                try {
                    loop->push_back(this);
                }
                catch (...) {
                    set_error(std::move(receiver), std::current_exception());
                }
            }
        };

        struct scheduler {
            struct sender {
                struct env {
                    run_loop* loop;

                    [[nodiscard]] constexpr auto query(get_completion_scheduler_t<set_value_t>) const noexcept {
                        return loop->get_scheduler();
                    }

                    [[nodiscard]] constexpr auto query(get_completion_scheduler_t<set_error_t>) const noexcept {
                        return loop->get_scheduler();
                    }

                    [[nodiscard]] constexpr auto query(get_completion_scheduler_t<set_stopped_t>) const noexcept {
                        return loop->get_scheduler();
                    }
                };

                using sender_concept = sender_t;

                using completion_signatures =
                    completion_signatures<set_value_t(), set_error_t(std::exception_ptr), set_stopped_t()>;

                run_loop* loop;

                [[nodiscard]] auto query(get_env_t) const noexcept {
                    return env{ loop };
                }

                constexpr auto connect(receiver auto&& rcvr) {
                    return operation_state<std::decay_t<decltype(rcvr)>>(loop, std::forward<decltype(rcvr)>(rcvr));
                }
            };

            using scheduler_concept = scheduler_t;

            run_loop* loop;

            [[nodiscard]] constexpr sender schedule() const {
                return sender{ loop };
            }

        private:
            [[nodiscard]]
            friend constexpr bool operator==(const scheduler& left, const scheduler& right) noexcept {
                return left.loop == right.loop;
            }

        };

    public:
        run_loop() noexcept : m_finished(false), m_worker_count(0) {}

        run_loop(run_loop&&) = delete;

        ~run_loop() noexcept {
            size_t task_count{};
            bool stopped{};
            {
                std::scoped_lock lock(m_mutex);
                task_count = m_queue.size();
                stopped = m_finished;
            }

            if (task_count > 0 || !stopped) {
                std::terminate();
            }
        }

        constexpr scheduler get_scheduler() noexcept {
            return scheduler{ this };
        }

        void run() {
            while (auto* task = pop_front()) {
                task->run();
            }
        }

        void finish() noexcept {
            {
                std::scoped_lock lock(m_mutex);
                m_finished = true;
            }
            m_cv.notify_all();
        }

    private:
        void push_back(operation_state_base* task) {
            {
                std::scoped_lock lock(m_mutex);
                if (m_finished) {
                    throw std::runtime_error("Invalid operation on finished run loop.");
                }

                m_queue.push(task);
            }
            m_cv.notify_one();
        }

        operation_state_base* pop_front() {
            std::unique_lock lock(m_mutex);

            m_cv.wait(lock, [this]() -> bool { return m_finished || !m_queue.empty(); });
            if (m_finished && m_queue.empty()) return nullptr;

            auto* task = m_queue.front();
            m_queue.pop();

            return task;
        }

        mutable std::mutex m_mutex;

        bool m_finished;
        std::atomic_size_t m_worker_count;
        std::condition_variable m_cv;
        std::queue<operation_state_base*> m_queue;
    };
}

#endif // !EXEC_RUN_LOOP_HPP