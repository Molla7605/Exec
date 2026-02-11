#ifndef EXEC_DETAILS_BASE_STOP_CALLBACK_HPP
#define EXEC_DETAILS_BASE_STOP_CALLBACK_HPP

#include <atomic>

namespace exec::details {
    struct base_stop_callback {
        virtual ~base_stop_callback() noexcept {
            if (deleted != nullptr) {
                *deleted = true;
            }
        }

        virtual void invoke() noexcept = 0;

        base_stop_callback* pop() noexcept {
            auto* const local_prev = prev;
            bool local_deleted_flag = false;

            deleted = &local_deleted_flag;

            invoke();

            if (!local_deleted_flag) {
                deleted = nullptr;
                invoked.store(true, std::memory_order_release);
            }

            return local_prev;
        }

        base_stop_callback* next{ nullptr };
        base_stop_callback* prev{ nullptr };

        bool* deleted{ nullptr };
        std::atomic_bool invoked{ false };
    };
}

#endif // !EXEC_DETAILS_BASE_STOP_CALLBACK_HPP