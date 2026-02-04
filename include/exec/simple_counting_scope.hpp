#ifndef EXEC_SIMPLE_COUNTING_SCOPE_HPP
#define EXEC_SIMPLE_COUNTING_SCOPE_HPP

#include "exec/sender.hpp"

namespace exec {
    struct simple_counting_scope {
        struct token {
            template<sender SenderT>
            [[nodiscard]] SenderT&& wrap(SenderT&& input) const noexcept {
                return std::forward<SenderT>(input);
            }

            bool try_associate() const;

            void disassociate() const;

        private:
            simple_counting_scope* m_scope;
        };

        simple_counting_scope() noexcept;
        ~simple_counting_scope() noexcept;

        simple_counting_scope(const simple_counting_scope&) = delete;
        simple_counting_scope& operator=(const simple_counting_scope&) = delete;

        simple_counting_scope(simple_counting_scope&&) noexcept = delete;
        simple_counting_scope& operator=(simple_counting_scope&&) noexcept = delete;

        [[nodiscard]] token get_token() noexcept;

        void close() noexcept;

        [[nodiscard]] sender auto join() noexcept;
    };
}

#endif // !EXEC_SIMPLE_COUNTING_SCOPE_HPP