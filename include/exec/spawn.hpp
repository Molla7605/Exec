#ifndef EXEC_SPAWN_HPP
#define EXEC_SPAWN_HPP

#include "exec/allocator.hpp"
#include "exec/env.hpp"
#include "exec/receiver.hpp"
#include "exec/sender.hpp"
#include "exec/scope_token.hpp"
#include "exec/operation_state.hpp"

#include "exec/details/association.hpp"

#include <memory>
#include <type_traits>
#include <utility>

namespace exec {
    struct spawn_operation_state_base {
        virtual ~spawn_operation_state_base() = default;

        virtual void complete() & noexcept = 0;
    };

    struct spawn_receiver {
        using receiver_concept = exec::receiver_tag;

        spawn_operation_state_base* op;

        void set_value() && noexcept {
            op->complete();
        }

        void set_stopped() && noexcept {
            op->complete();
        }

        [[nodiscard]] constexpr empty_env query(get_env_t) const noexcept {
            return {};
        }
    };

    template<typename AllocT, typename SenderT, typename TokenT>
    struct spawn_operation_state : spawn_operation_state_base {
        using operation_state_concept = exec::operation_state_tag;

        using op_t = connect_result_t<SenderT, spawn_receiver>;
        using alloc_t = std::allocator_traits<AllocT>::template rebind_alloc<spawn_operation_state>;
        using assoc_t = details::association_of_t<TokenT>;

        alloc_t alloc;
        assoc_t association;
        op_t op;

        spawn_operation_state(AllocT alloc, SenderT&& sender, TokenT token)
            noexcept(noexcept(exec::connect(std::forward<SenderT>(sender), spawn_receiver{ nullptr }))) :
                alloc(std::move(alloc)),
                association(token.try_associate()),
                op(exec::connect(std::forward<SenderT>(sender), spawn_receiver{ this })) {}

        void complete() & noexcept override {
            auto local_association = std::move(association);
            {
                using traits_t = std::allocator_traits<alloc_t>::template rebind_traits<spawn_operation_state>;
                typename traits_t::allocator_type local_alloc(alloc);
                traits_t::destroy(local_alloc, this);
                traits_t::deallocate(local_alloc, this, 1);
            }
        }

        void run() noexcept {
            if (association) {
                exec::start(op);
            }
            else {
                complete();
            }
        }
    };

    struct spawn_t {
        template<sender SenderT, scope_token TokenT, typename EnvT = empty_env>
        void operator()(SenderT&& sender, TokenT token, EnvT env = {}) const {
            auto new_sender = token.wrap(std::forward<SenderT>(sender));
            auto new_env = [&]() -> decltype(auto) {
                if constexpr (!details::has_query<EnvT, get_allocator_t> && details::has_query<env_of_t<decltype(new_sender)>, get_allocator_t>) {
                    return details::join_env(prop{ get_allocator, get_allocator(exec::get_env(new_sender)) }, env);
                }
                else {
                    return env;
                }
            }();

            static_assert(std::is_same_v<error_types_of_t<decltype(new_sender), decltype(new_env), details::type_holder>, details::type_holder<>>);

            auto get_alloc = [&] {
                if constexpr (details::has_query<std::remove_cvref_t<decltype(new_env)>, get_allocator_t>) {
                    return get_allocator(new_env);
                }
                else {
                    return std::allocator<void>{};
                }
            };

            using alloc_t = std::remove_cvref_t<decltype(get_alloc())>;
            using op_t = decltype(spawn_operation_state{ std::declval<alloc_t>(), details::write_env(new_sender, new_env), token });

            using traits_t = std::allocator_traits<alloc_t>::template rebind_traits<op_t>;

            typename traits_t::allocator_type alloc(get_alloc());
            op_t* op = traits_t::allocate(alloc, 1);

            try {
                traits_t::construct(alloc, op, alloc, details::write_env(new_sender, new_env), token);
            }
            catch (...) {
                traits_t::deallocate(alloc, op, 1);
                throw;
            }

            op->run();
        }
    };
    inline constexpr spawn_t spawn{};
}

#endif // !EXEC_SPAWN_HPP