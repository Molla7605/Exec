#ifndef EXEC_DETAILS_COUNTING_SCOPE_STATE_HPP
#define EXEC_DETAILS_COUNTING_SCOPE_STATE_HPP

#include "exec/details/scope_join.hpp"
#include "exec/details/scope_state_flags.hpp"

#include <atomic>
#include <cassert>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>

namespace exec::details {
    struct counting_scope_state {
        using base_state = impls_for<scope_join_t>::base_state;

        struct dummy_state : base_state {
            void complete() noexcept override {}
        };

        static constexpr std::size_t max_associations =
            (std::numeric_limits<std::size_t>::max() >> 3) - 1;

        dummy_state m_dummy_head;
        std::atomic_size_t m_state;
        std::atomic<base_state*> m_head;

        counting_scope_state() noexcept : m_state{ 0 }, m_head{ &m_dummy_head } {}

        ~counting_scope_state() noexcept {
            const auto state = m_state.load(std::memory_order_acquire);

            if (!is_joined(state) && !is_unused(state)) {
                std::terminate();
            }
        }

        counting_scope_state(const counting_scope_state&) = delete;

        counting_scope_state& operator=(const counting_scope_state&) = delete;

        counting_scope_state(counting_scope_state&&) = delete;

        counting_scope_state& operator=(counting_scope_state&&) = delete;

        bool try_associate() noexcept {
            std::size_t expected = m_state.load(std::memory_order_relaxed);
            std::size_t desired = 0;
            do {
                if (is_joined(expected) ||
                    is_closed(expected) ||
                    get_count(expected) >= max_associations)
                {
                    return false;
                }

                desired = (get_count(expected) + 1) << 3 | get_state(expected) |
                          scope_state_flags::Used;

            } while (!m_state.compare_exchange_weak(expected,
                                                    desired,
                                                    std::memory_order_acq_rel,
                                                    std::memory_order_relaxed));

            return true;
        }

        void disassociate() noexcept {
            std::size_t expected = m_state.load(std::memory_order_relaxed);
            std::size_t desired = 0;
            do {
                assert(get_count(expected) != 0);
                assert(!is_joined(expected));

                if (is_joining(expected) && get_count(expected) == 1) {
                    desired = scope_state_flags::Joined;
                }
                else {
                    desired = (get_count(expected) - 1) << 3 | get_state(expected);
                }
            } while (!m_state.compare_exchange_weak(expected,
                                                    desired,
                                                    std::memory_order_acq_rel,
                                                    std::memory_order_relaxed));

            if (is_joined(desired)) {
                auto* local_head = m_head.exchange(nullptr, std::memory_order_acq_rel);

                while (local_head != nullptr) {
                    auto* const state = std::exchange(local_head, local_head->prev);

                    state->complete();
                }
            }
        }

        void close() noexcept {
            auto state = m_state.load(std::memory_order_relaxed);
            do {
                if (is_closed(state) || is_joined(state)) {
                    return;
                }
            } while (!m_state.compare_exchange_weak(state,
                                                    state | scope_state_flags::Closed,
                                                    std::memory_order_acq_rel,
                                                    std::memory_order_relaxed));
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
                    desired = scope_state_flags::Joined;
                }
                else {
                    desired = expected | scope_state_flags::Joining;
                }
            } while (!m_state.compare_exchange_weak(expected,
                                                    desired,
                                                    std::memory_order_acq_rel,
                                                    std::memory_order_relaxed));

            if (is_joined(desired)) {
                return false;
            }

            base_state* expected_head = m_head.load(std::memory_order_acquire);
            base_state* desired_head = std::addressof(state);
            do {
                if (expected_head == nullptr) {
                    return false;
                }
                desired_head->prev = expected_head;
            } while (!m_head.compare_exchange_weak(expected_head,
                                                   desired_head,
                                                   std::memory_order_release,
                                                   std::memory_order_acquire));

            return true;
        }

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
            return (state & scope_state_flags::Closed) != 0;
        }

        [[nodiscard]]
        static constexpr bool is_joining(std::size_t state) noexcept {
            return (state & scope_state_flags::Joining) != 0;
        }

        [[nodiscard]]
        static constexpr bool is_joined(std::size_t state) noexcept {
            return state == scope_state_flags::Joined;
        }

        [[nodiscard]]
        static constexpr bool is_unused(std::size_t state) noexcept {
            return (state & scope_state_flags::Used) == 0;
        }
    };
}

#endif // !EXEC_DETAILS_COUNTING_SCOPE_STATE_HPP