#ifndef EXEC_COUNTING_SCOPE_HPP
#define EXEC_COUNTING_SCOPE_HPP

#include "exec/sender.hpp"

#include "exec/details/association.hpp"
#include "exec/details/counting_scope_state.hpp"
#include "exec/details/scope_join.hpp"

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

        simple_counting_scope() noexcept = default;

        ~simple_counting_scope() noexcept = default;

        simple_counting_scope(const simple_counting_scope&) = delete;
        simple_counting_scope& operator=(const simple_counting_scope&) = delete;

        simple_counting_scope(simple_counting_scope&&) noexcept = delete;
        simple_counting_scope& operator=(simple_counting_scope&&) noexcept = delete;

        static constexpr std::size_t max_associations = details::counting_scope_state::max_associations;

        [[nodiscard]] token get_token() noexcept {
            return token{ this };
        }

        void close() noexcept {
            m_state.close();
        }

        [[nodiscard]] sender auto join() noexcept {
            return details::scope_join(this);
        }

    private:
        friend class token;
        friend class details::impls_for<details::scope_join_t>;
        friend struct details::association_t<simple_counting_scope>;

        [[nodiscard]] assoc_t try_associate() noexcept {
            return m_state.try_associate() ? assoc_t{ this } : assoc_t{};
        }

        void disassociate() noexcept {
            m_state.disassociate();
        }

        template<typename StateT>
        [[nodiscard]] bool try_start_join(StateT& state) {
            return m_state.try_start_join(state);
        }

        details::counting_scope_state m_state;

    };
}

#endif // !EXEC_COUNTING_SCOPE_HPP