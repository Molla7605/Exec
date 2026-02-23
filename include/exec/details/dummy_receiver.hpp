#ifndef EXEC_DETAILS_DUMMY_RECEIVER_HPP
#define EXEC_DETAILS_DUMMY_RECEIVER_HPP

#include "exec/receiver.hpp"

#include <exception>

namespace exec::details {
    template<typename EnvT>
    struct dummy_receiver {
        using receiver_concept = exec::receiver_t;

        void set_value(...) && noexcept {

        }

        void set_error(...) && noexcept {

        }

        void set_stopped() && noexcept {

        }

        [[noreturn]] constexpr EnvT get_env() const noexcept {
            std::terminate();
        }
    };
}

#endif // !EXEC_DETAILS_DUMMY_RECEIVER_HPP