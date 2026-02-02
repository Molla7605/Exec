#ifndef EXEC_DETAILS_SYNC_WAIT_STATE_HPP
#define EXEC_DETAILS_SYNC_WAIT_STATE_HPP

#include "exec/run_loop.hpp"
#include "exec/scheduler.hpp"
#include "exec/transform_completion_signatures.hpp"

#include "exec/details/decayed_tuple.hpp"
#include "exec/details/meta_merge.hpp"

#include <exception>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace exec::details {
    template<typename SenderT, typename EnvT = empty_env>
    using sync_wait_error_type = std::optional<meta_merge_t<std::variant<std::exception_ptr>, error_types_of_t<SenderT, EnvT, std::variant>>>;

    template<typename SenderT, typename EnvT = empty_env>
    using sync_wait_result_type = std::optional<value_types_of_t<SenderT, EnvT, decayed_tuple, std::type_identity_t>>;

    struct sync_wait_error_handler_t {
        static void operator()(const std::exception_ptr& e) {
            std::rethrow_exception(e);
        }

        static void operator()(auto&& e) {
            throw std::forward<decltype(e)>(e);
        }
    };
    inline constexpr sync_wait_error_handler_t sync_wait_error_handler{};

    struct sync_wait_env {
        run_loop* loop;

        [[nodiscard]] constexpr run_loop* query(get_scheduler_t) const noexcept {
            return loop;
        }
    };

    template<typename SenderT>
    struct sync_wait_state {
        run_loop loop;
        sync_wait_error_type<SenderT, sync_wait_env> error;
        sync_wait_result_type<SenderT, sync_wait_env> result;
    };
}

#endif // !EXEC_DETAILS_SYNC_WAIT_STATE_HPP