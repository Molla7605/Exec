#ifndef EXEC_DETAILS_STOP_STATE_HPP
#define EXEC_DETAILS_STOP_STATE_HPP

#include "exec/details/base_stop_callback.hpp"
#include "exec/details/spin_lock_hint.hpp"

#include <atomic>
#include <cstdint>
#include <thread>

namespace exec::details {
    class stop_state {
    public:
        enum state : uint8_t {
            Locked = 1,
            Closed = 1 << 1,
        };

        stop_state() noexcept = default;

        [[nodiscard]] bool stop_requested() const noexcept {
            return (m_state.load(std::memory_order_acquire) & Closed) != 0;
        }

        [[nodiscard]] bool try_request_stop() noexcept {
            const auto old = spin_lock();

            if ((old & Closed) != 0) {
                unlock(old);
                return false;
            }

            m_callback_thread = std::this_thread::get_id();

            auto* callback = m_head;
            m_head = nullptr;

            unlock(old | Closed);

            while (callback != nullptr) {
                callback = callback->pop();
            }

            return true;
        }

        [[nodiscard]] bool try_add_callback(base_stop_callback* callback) const noexcept {
            const auto old = spin_lock();

            if ((old & Closed) != 0) {
                unlock(old);
                return false;
            }

            if (m_head != nullptr) {
                m_head->next = callback;
                callback->prev = m_head;
            }

            m_head = callback;

            unlock(old);

            return true;
        }

        void remove_callback(base_stop_callback* callback) const noexcept {
            const auto old = spin_lock();

            if ((old & Closed) != 0) {
                const auto callback_thread = m_callback_thread;
                unlock(old);

                if (callback_thread == std::this_thread::get_id()) {
                    return;
                }

                while (!callback->invoked.load(std::memory_order_acquire)) {
                    EXEC_SPIN_LOCK_HINT();
                }
            }
            else if (m_head == callback) {
                m_head = callback->prev;

                unlock(old);
            }
            else {
                if (callback->next != nullptr) {
                    callback->next->prev = callback->prev;
                }
                if (callback->prev != nullptr) {
                    callback->prev->next = callback->next;
                }

                unlock(old);
            }
        }

    private:
        uint8_t spin_lock() const noexcept {
            uint8_t expected = m_state.load(std::memory_order_relaxed);
            do {
                while ((expected & Locked) != 0) {
                    EXEC_SPIN_LOCK_HINT();
                    expected = m_state.load(std::memory_order_relaxed);
                }
            } while (!m_state.compare_exchange_weak(expected,
                                                    expected | Locked,
                                                    std::memory_order_acquire,
                                                    std::memory_order_relaxed));

            return expected;
        }

        void unlock(uint8_t state) const noexcept {
            m_state.store(state & ~Locked, std::memory_order_release);
        }

        mutable std::atomic_uint8_t m_state{ 0 };
        mutable base_stop_callback* m_head{ nullptr };
        std::thread::id m_callback_thread{};

    };
}

#endif // !EXEC_DETAILS_STOP_STATE_HPP