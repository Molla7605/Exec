#ifndef EXEC_COUNTING_SCOPE_HPP
#define EXEC_COUNTING_SCOPE_HPP

#include "exec/sender.hpp"

#include "exec/details/scope_state_flags.hpp"
#include "exec/details/scope_join.hpp"
#include "exec/details/association.hpp"

#include <atomic>
#include <cassert>
#include <limits>

namespace exec {
    class simple_counting_scope {
    public:
        using assoc_t = details::association_t<simple_counting_scope>;

        class token {
        public:
            template<sender SenderT>
            [[nodiscard]] SenderT&& wrap(SenderT&& input) const noexcept {
                return std::forward<SenderT>(input);
            }

            [[nodiscard]] assoc_t try_associate() const noexcept {
                return m_scope->try_associate();
            }

        private:
            friend class simple_counting_scope;

            explicit token(simple_counting_scope* scope) noexcept : m_scope(scope) {}

            simple_counting_scope* m_scope;

        };

        simple_counting_scope() noexcept : m_state(0), m_head(&m_dummy_head) {}

        ~simple_counting_scope() noexcept {
            const auto state = m_state.load(std::memory_order_acquire);

            if (!is_joined(state) && !is_unused(state)) {
                std::terminate();
            }
        }

        simple_counting_scope(const simple_counting_scope&) = delete;
        simple_counting_scope& operator=(const simple_counting_scope&) = delete;

        simple_counting_scope(simple_counting_scope&&) noexcept = delete;
        simple_counting_scope& operator=(simple_counting_scope&&) noexcept = delete;

        static constexpr std::size_t max_associations =
            (std::numeric_limits<std::size_t>::max() >> 3) - 1;

        [[nodiscard]] token get_token() noexcept {
            return token{ this };
        }

        void close() noexcept {
            auto state = m_state.load(std::memory_order_relaxed);
            do {
                if (is_closed(state) || is_joined(state)) {
                    return;
                }
            } while (!m_state.compare_exchange_weak(state,
                                                    state | details::scope_state_flags::Closed,
                                                    std::memory_order_acq_rel,
                                                    std::memory_order_relaxed));
        }

        [[nodiscard]] sender auto join() noexcept {
            return details::scope_join(this);
        }

    private:
        friend class token;
        friend class details::impls_for<details::scope_join_t>;
        friend struct details::association_t<simple_counting_scope>;

        using base_state = details::impls_for<details::scope_join_t>::base_state;

        [[nodiscard]]
        static constexpr std::size_t get_count(std::size_t state) noexcept {
            return state >> 3;
        }

        [[nodiscard]]
        static constexpr std::size_t get_state(std::size_t state) noexcept {
            return state & 0b111;
        }

        [[nodiscard]]
        static constexpr bool is_closed(std::size_t state) noexcept {
            return (state & details::scope_state_flags::Closed) != 0;
        }

        [[nodiscard]]
        static constexpr bool is_joining(std::size_t state) noexcept {
            return (state & details::scope_state_flags::Joining) != 0;
        }

        [[nodiscard]]
        static constexpr bool is_joined(std::size_t state) noexcept {
            return state == details::scope_state_flags::Joined;
        }

        [[nodiscard]]
        static constexpr bool is_unused(std::size_t state) noexcept {
            return (state & details::scope_state_flags::Used) == 0;
        }

        [[nodiscard]] assoc_t try_associate() noexcept {
            std::size_t expected = m_state.load(std::memory_order_relaxed);
            std::size_t desired = 0;
            do {
                if (is_joined(expected) ||
                    is_closed(expected) ||
                    get_count(expected) >= max_associations)
                {
                    return {};
                }

                desired = (get_count(expected) + 1) << 3 | get_state(expected) |
                          details::scope_state_flags::Used;

            } while (!m_state.compare_exchange_weak(expected,
                                                    desired,
                                                    std::memory_order_acq_rel,
                                                    std::memory_order_relaxed));

            return { this };
        }

        void disassociate() noexcept {
            std::size_t expected = m_state.load(std::memory_order_relaxed);
            std::size_t desired = 0;
            do {
                assert(get_count(expected) != 0);
                assert(!is_joined(expected));

                if (is_joining(expected) && get_count(expected) == 1) {
                    desired = details::scope_state_flags::Joined;
                }
                else {
                    desired = (get_count(expected) - 1) << 3 | get_state(expected);
                }
            } while (!m_state.compare_exchange_weak(expected,
                                                    desired,
                                                    std::memory_order_acq_rel,
                                                    std::memory_order_relaxed));

            if (is_joined(desired)) {
                auto* local_head = m_head.exchange(nullptr, std::memory_order_release);

                while (local_head != nullptr) {
                    auto* const state = std::exchange(local_head, local_head->prev);

                    state->complete();
                }
            }
        }

        template<typename StateT>
        requires std::derived_from<StateT, base_state>
        [[nodiscard]] bool try_start_join(StateT& state) {
            std::size_t expected = m_state.load(std::memory_order_relaxed);
            std::size_t desired = 0;
            do {
                if (is_joined(expected)) {
                    return false;
                }

                // if (is_joined(expected) || is_unused(expected)) {
                if (get_count(expected) == 0) {
                    desired = details::scope_state_flags::Joined;
                }
                else {
                    desired = expected | details::scope_state_flags::Joining;
                }
            } while (!m_state.compare_exchange_weak(expected,
                                                    desired,
                                                    std::memory_order_acq_rel,
                                                    std::memory_order_relaxed));

            if (is_joined(desired)) {
                return false;
            }

            base_state* expected_head = m_head.load(std::memory_order_relaxed);
            base_state* desired_head = std::addressof(state);
            do {
                if (expected_head == nullptr) {
                    return false;
                }
                desired_head->prev = expected_head;
            } while (!m_head.compare_exchange_weak(expected_head,
                                                   desired_head,
                                                   std::memory_order_acq_rel,
                                                   std::memory_order_relaxed));

            return true;
        }

        struct dummy_state : base_state {
            void complete() noexcept override {}
        };

        dummy_state m_dummy_head;
        std::atomic_size_t m_state;
        std::atomic<base_state*> m_head;

    };
}

#endif // !EXEC_COUNTING_SCOPE_HPP