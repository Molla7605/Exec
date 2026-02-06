#ifndef EXEC_SPAWN_HPP
#define EXEC_SPAWN_HPP

#include "exec/allocator.hpp"
#include "exec/env.hpp"
#include "exec/receiver.hpp"
#include "exec/sender.hpp"
#include "exec/scope_token.hpp"
#include "exec/operation_state.hpp"

#include <memory>
#include <type_traits>
#include <utility>

namespace exec {
    struct spawn_operation_state_base {
        virtual ~spawn_operation_state_base() = default;

        virtual void start() & noexcept = 0;
    };

    struct spawn_receiver {
        using receiver_concept = exec::receiver_t;

        spawn_operation_state_base* op;

        void set_value() && noexcept {
            op->start();
        }

        void set_stopped() && noexcept {
            op->start();
        }

        [[nodiscard]] constexpr empty_env query(get_env_t) const noexcept {
            return {};
        }
    };

    template<typename AllocT, typename SenderT, typename TokenT>
    struct spawn_operation_state : spawn_operation_state_base {
        using operation_state_concept = exec::operation_state_t;

        using op_t = connect_result_t<SenderT, spawn_receiver>;
        using alloc_t = std::allocator_traits<AllocT>::template rebind_alloc<spawn_operation_state>;

        alloc_t alloc;
        TokenT token;
        op_t op;

        spawn_operation_state(AllocT alloc, SenderT&& sender, TokenT token)
            noexcept(noexcept(exec::connect(std::forward<SenderT>(sender), spawn_receiver{ nullptr }))) :
                alloc(alloc),
                token(token),
                op(exec::connect(token.wrap(std::forward<SenderT>(sender)), spawn_receiver{ this })) {}

        void start() & noexcept override {
            auto local_token = std::move(token);

            destroy();

            local_token.disassociate();
        }

        void run() {
            if (token.try_associate()) {
                exec::start(op);
            }
            else {
                destroy();
            }
        }

        void destroy() noexcept {
            auto local_alloc = std::move(alloc);

            std::allocator_traits<alloc_t>::destroy(local_alloc, this);
            std::allocator_traits<alloc_t>::deallocate(local_alloc, this, 1);
        }
    };

    struct spawn_t {
        template<sender SenderT, scope_token TokenT, typename EnvT = empty_env>
        void operator()(SenderT&& sender, TokenT token, EnvT env = {}) const {
            constexpr auto get_alloc = [&] constexpr noexcept {
                if constexpr (requires { get_allocator(env); }) {
                    return get_allocator(env);
                }
                if constexpr (requires { get_allocator(sender); }) {
                    return get_allocator(sender);
                }

                return std::allocator<void>{};
            };

            using src_alloc_t = std::remove_cvref_t<decltype(get_alloc())>;
            using op_t = spawn_operation_state<src_alloc_t, std::remove_cvref_t<SenderT>, TokenT>;

            using alloc_t = std::allocator_traits<src_alloc_t>::template rebind_alloc<op_t>;

            alloc_t alloc = get_alloc();
            op_t* op = std::allocator_traits<alloc_t>::allocate(alloc, 1);

            try {
                std::allocator_traits<alloc_t>::construct(alloc, op, alloc, std::forward<SenderT>(sender), token);
            }
            catch (...) {
                std::allocator_traits<alloc_t>::deallocate(alloc, op, 1);
                throw;
            }

            try {
                op->run();
            }
            catch (...) {
                op->destroy();
                throw;
            }
        }
    };
    inline constexpr spawn_t spawn{};
}

#endif // !EXEC_SPAWN_HPP