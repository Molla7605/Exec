#ifndef EXEC_STOP_TOKEN_HPP
#define EXEC_STOP_TOKEN_HPP

#include "exec/query.hpp"

#include "exec/details/base_stop_callback.hpp"
#include "exec/details/stop_state.hpp"

#include <cassert>
#include <concepts>
#include <functional>
#include <memory>
#include <stop_token>
#include <type_traits>
#include <utility>

namespace exec {
    template<typename T>
    concept stoppable_token =
        requires(const T token) {
            { token.stop_requested() } noexcept -> std::same_as<bool>;
            { token.stop_possible() } noexcept -> std::same_as<bool>;
            { T(token) } noexcept;
        } &&
        std::copyable<T> &&
        std::equality_comparable<T> &&
        std::swappable<T>;

    template<typename T>
    concept unstoppable_token =
        stoppable_token<T> &&
        requires(const T token) {
            requires std::bool_constant<(!token.stop_possible())>::value;
        };

    template<typename CallbackT>
    class stop_callback;

    class stop_token {
    public:
        template<typename CallbackT>
        using callback_type = stop_callback<CallbackT>;

        stop_token() = default;

        [[nodiscard]] bool operator==(const stop_token&) const = default;

        [[nodiscard]] bool stop_requested() const noexcept {
            return m_state != nullptr && m_state->stop_requested();
        }

        [[nodiscard]] bool stop_possible() const noexcept {
            return m_state != nullptr;
        }

        void swap(stop_token& other) noexcept {
            std::swap(m_state, other.m_state);
        }

    private:
        template<typename CallbackT>
        friend class stop_callback;

        friend class stop_source;

        explicit stop_token(const std::shared_ptr<details::stop_state>& state) noexcept : m_state(state) {}

        [[nodiscard]] const std::shared_ptr<details::stop_state>& get_state() const noexcept {
            return m_state;
        }

        std::shared_ptr<details::stop_state> m_state;

    };

    class stop_source {
    public:
        stop_source() : m_state(std::make_shared<details::stop_state>()) {}

        explicit stop_source(std::nostopstate_t) noexcept {}

        [[nodiscard]] stop_token get_token() const noexcept {
            return stop_token{ m_state };
        }

        [[nodiscard]] static constexpr bool stop_possible() noexcept {
            return true;
        }

        [[nodiscard]] bool stop_requested() const noexcept {
            return m_state != nullptr && m_state->stop_requested();
        }

        bool request_stop() noexcept {
            return m_state != nullptr && m_state->try_request_stop();
        }

        void swap(stop_source& other) noexcept {
            m_state.swap(other.m_state);
        }

        [[nodiscard]] bool operator==(const stop_source&) const = default;

    private:
        friend class stop_token;

        std::shared_ptr<details::stop_state> m_state;
    };

    template<typename CallbackT>
    class stop_callback : details::base_stop_callback {
    public:
        using callback_type = CallbackT;

        template<typename InitT>
        explicit stop_callback(stop_token token, InitT&& init)
            noexcept(std::is_nothrow_constructible_v<CallbackT, InitT>) :
                m_callback(std::forward<InitT>(init)), m_state(token.get_state())
        {
            if (m_state != nullptr) {
                if (!m_state->try_add_callback(this)) {
                    stop_callback::invoke();
                    m_state.reset();
                }
            }
        }

        ~stop_callback() noexcept override {
            if (m_state != nullptr) {
                m_state->remove_callback(this);
            }
        }

        stop_callback(const stop_callback&) = delete;
        stop_callback& operator=(const stop_callback&) = delete;

        stop_callback(stop_callback&&) = delete;
        stop_callback& operator=(stop_callback&&) = delete;

    private:
        void invoke() noexcept override {
            std::invoke(m_callback);
        }

        CallbackT m_callback;
        std::shared_ptr<details::stop_state> m_state;

    };

    template<typename CallbackT>
    stop_callback(stop_token, CallbackT) -> stop_callback<CallbackT>;

    template<typename CallbackT>
    class inplace_stop_callback;

    class inplace_stop_source;

    class inplace_stop_token {
    public:
        template<typename CallbackT>
        using callback_type = inplace_stop_callback<CallbackT>;

        inplace_stop_token() = default;

        [[nodiscard]] bool operator==(const inplace_stop_token&) const = default;

        [[nodiscard]] bool stop_requested() const noexcept;

        [[nodiscard]] bool stop_possible() const noexcept {
            return m_src != nullptr;
        }

        void swap(inplace_stop_token& other) noexcept {
            std::swap(m_src, other.m_src);
        }

    private:
        template<typename CallbackT>
        friend class inplace_stop_callback;

        friend class inplace_stop_source;

        explicit inplace_stop_token(const inplace_stop_source* src) noexcept : m_src(src) {};

        [[nodiscard]] const details::stop_state* get_state() const noexcept;

        const inplace_stop_source* m_src{ nullptr };

    };

    class inplace_stop_source {
    public:
        inplace_stop_source() noexcept = default;

        ~inplace_stop_source() noexcept {
            assert(stop_requested());
        }

        inplace_stop_source(const inplace_stop_source&) = delete;
        inplace_stop_source& operator=(const inplace_stop_source&) = delete;

        inplace_stop_source(inplace_stop_source&&) = delete;
        inplace_stop_source& operator=(inplace_stop_source&&) = delete;

        [[nodiscard]] inplace_stop_token get_token() const noexcept {
            return inplace_stop_token{ this };
        }

        [[nodiscard]] static constexpr bool stop_possible() noexcept {
            return true;
        }

        [[nodiscard]] bool stop_requested() const noexcept {
            return m_state.stop_requested();
        }

        bool request_stop() noexcept {
            return m_state.try_request_stop();
        }

    private:
        friend class inplace_stop_token;

        details::stop_state m_state;

    };

    inline bool inplace_stop_token::stop_requested() const noexcept {
        return m_src != nullptr && m_src->stop_requested();
    }

    inline const details::stop_state* inplace_stop_token::get_state() const noexcept {
        if (m_src == nullptr) {
            return nullptr;
        }

        return &m_src->m_state;
    }

    template<typename CallbackT>
    class inplace_stop_callback : details::base_stop_callback {
    public:
        using callback_type = CallbackT;

        template<typename InitT>
        explicit inplace_stop_callback(inplace_stop_token token, InitT&& init)
            noexcept(std::is_nothrow_constructible_v<CallbackT, InitT>) :
                m_callback(std::forward<InitT>(init)), m_state(token.get_state())
        {
            if (m_state != nullptr) {
                if (!m_state->try_add_callback(this)) {
                    inplace_stop_callback::invoke();
                    m_state = nullptr;
                }
            }
        }

        ~inplace_stop_callback() noexcept override {
            if (m_state != nullptr) {
                m_state->remove_callback(this);
            }
        }

        inplace_stop_callback(const inplace_stop_callback&) = delete;
        inplace_stop_callback& operator=(const inplace_stop_callback&) = delete;

        inplace_stop_callback(inplace_stop_callback&&) = delete;
        inplace_stop_callback& operator=(inplace_stop_callback&&) = delete;

    private:
        void invoke() noexcept override {
            std::invoke(m_callback);
        }

        CallbackT m_callback;
        const details::stop_state* m_state;

    };

    template<typename CallbackT>
    inplace_stop_callback(inplace_stop_token, CallbackT) -> inplace_stop_callback<CallbackT>;

    class naver_stop_token {
        struct callback {
            explicit callback(naver_stop_token, auto&&) noexcept {}
        };

    public:
        template<typename CallbackT>
        using callback_type = callback;

        [[nodiscard]] bool operator==(const naver_stop_token&) const = default;

        [[nodiscard]] static constexpr bool stop_requested() noexcept { return false; }

        [[nodiscard]] static constexpr bool stop_possible() noexcept { return false; }

    };

    struct get_stop_token_t {
        template<typename EnvT>
        requires std::invocable<const std::remove_cvref_t<EnvT>&, get_stop_token_t>
        [[nodiscard]] constexpr decltype(auto) operator()(const EnvT& env) const noexcept {
            return env.query(*this);
        }

        template<typename EnvT>
        [[nodiscard]] constexpr const naver_stop_token& operator()(const EnvT&) const noexcept {
            static naver_stop_token token{};

            return token;
        }

        [[nodiscard]] static consteval bool query(forwarding_query_t) noexcept {
            return true;
        }
    };
    inline constexpr get_stop_token_t get_stop_token{};

    template<typename T>
    using stop_token_of_t = std::remove_cvref_t<decltype(get_stop_token(std::declval<T>()))>;
}

#endif // !EXEC_STOP_TOKEN_HPP