#ifndef EXEC_COUNTING_SCOPES_HPP
#define EXEC_COUNTING_SCOPES_HPP

#include "exec/sender.hpp"
#include "exec/stop_token.hpp"

#include "exec/details/association.hpp"
#include "exec/details/counting_scope_state.hpp"
#include "exec/details/scope_join.hpp"
#include "exec/details/stop_when.hpp"

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
        friend struct details::impls_for<details::scope_join_t>;
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

    class counting_scope {
    public:
        using assoc_t = details::association_t<counting_scope>;

        class token {
        public:
            template<sender SenderT>
            [[nodiscard]] sender auto wrap(SenderT&& sender) const
                noexcept(std::is_nothrow_constructible_v<std::remove_cvref_t<SenderT>, SenderT>)
            {
                return details::stop_when(std::forward<SenderT>(sender), m_scope->m_stop_source.get_token());
            }

            [[nodiscard]] assoc_t try_associate() const noexcept {
                return m_scope->try_associate();
            }

        private:
            friend class counting_scope;

            explicit token(counting_scope* scope) noexcept :
                m_scope(scope) {}

            counting_scope* m_scope;

        };

        static constexpr std::size_t max_associations = details::counting_scope_state::max_associations;

        counting_scope() noexcept = default;

        ~counting_scope() noexcept = default;

        counting_scope(counting_scope&&) = delete;

        [[nodiscard]] token get_token() noexcept {
            return token{ this };
        }

        void close() noexcept {
            m_state.close();
        }

        [[nodiscard]] sender auto join() noexcept {
            return details::scope_join(this);
        }

        void request_stop() noexcept {
            m_stop_source.request_stop();
        }

    private:
        friend class token;
        friend struct details::impls_for<details::scope_join_t>;
        friend struct details::association_t<counting_scope>;

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
        inplace_stop_source m_stop_source;

    };
}

#endif // !EXEC_COUNTING_SCOPES_HPP