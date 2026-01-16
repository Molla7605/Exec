#ifndef EXEC_EXEC_HPP
#define EXEC_EXEC_HPP

#include <atomic>
#include <concepts>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <stop_token>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace exec {
    template<typename Type>
    concept queryable = std::destructible<Type>;

    struct forwarding_query_t {
        template<typename QueryT>
        [[nodiscard]] consteval bool operator()(const QueryT& query) const noexcept {
            if constexpr (requires { std::declval<QueryT>().query(std::declval<forwarding_query_t>()); }) {
                return query.query(*this);
            }

            return std::derived_from<QueryT, forwarding_query_t>;
        }
    };
    inline constexpr forwarding_query_t forwarding_query{};

    template<typename QueryT>
    concept is_forwarding_query = forwarding_query(QueryT{});

    template<typename EnvT>
    struct forwarding_env : EnvT {
        template<typename QueryT>
        requires is_forwarding_query<QueryT>
        [[nodiscard]] constexpr decltype(auto) query(QueryT) const noexcept {
            return static_cast<const EnvT&>(*this).query(QueryT{});
        }
    };

    template<typename EnvT>
    forwarding_env(EnvT) -> forwarding_env<EnvT>;

    template<typename EnvT>
    constexpr auto forward_env(EnvT&& env) noexcept {
        return forwarding_env{ std::forward<EnvT>(env) };
    }

    struct get_stop_token_t {
        template<typename EnvT>
        requires requires{ std::declval<EnvT>().template query<get_stop_token_t>({}); }
        [[nodiscard]] constexpr decltype(auto) operator()(auto&& env) const noexcept {
            return std::forward<decltype(env)>(env).query(*this);
        }

        [[nodiscard]] constexpr const std::stop_token& operator()(auto&&) const noexcept {
            static std::stop_token token;

            return token;
        }

        [[nodiscard]] static consteval bool query(forwarding_query_t) noexcept {
            return true;
        }
    };
    inline constexpr get_stop_token_t get_stop_token{};

    template<typename T>
    using stop_token_of_t = std::remove_cvref_t<decltype(get_stop_token(std::declval<T>()))>;

    struct set_value_t {
        template<typename receiver_t, typename... value_ts>
        constexpr auto operator()(receiver_t&& receiver, value_ts&&... values) const noexcept {
            return std::forward<receiver_t>(receiver).set_value(std::forward<value_ts>(values)...);
        }
    };
    inline constexpr set_value_t set_value{};

    struct set_error_t {
        template<typename receiver_t, typename value_t>
        constexpr auto operator()(receiver_t&& receiver, value_t&& value) const noexcept {
            return std::forward<receiver_t>(receiver).set_error(std::forward<value_t>(value));
        }
    };
    inline constexpr set_error_t set_error{};

    struct set_stopped_t {
        template<typename receiver_t>
        constexpr auto operator()(receiver_t&& receiver) const noexcept {
            return std::forward<receiver_t>(receiver).set_stopped();
        }
    };
    inline constexpr set_stopped_t set_stopped{};

    struct get_scheduler_t {
        template<typename EnvT>
        [[nodiscard]] constexpr decltype(auto) operator()(const EnvT& env) const noexcept {
            return env.query(*this);
        }
    };
    inline constexpr get_scheduler_t get_scheduler{};

    struct get_delegation_scheduler_t {
        template<typename EnvT>
        [[nodiscard]] constexpr decltype(auto) operator()(const EnvT& env) const noexcept {
            return env.query(*this);
        }
    };
    inline constexpr get_delegation_scheduler_t get_delegation_scheduler{};

    template<typename TagT>
    struct get_completion_scheduler_t {
        template<typename EnvT>
        [[nodiscard]] constexpr decltype(auto) operator()(const EnvT& env) const noexcept {
            return env.query(*this);
        }
    };
    template<typename TagT>
    inline constexpr get_completion_scheduler_t<TagT> get_completion_scheduler{};

    // TODO: struct get_domain_t{};

    struct empty_env {};

    struct get_env_t {
        template<queryable QueryableT>
        [[nodiscard]] constexpr decltype(auto) operator()(const QueryableT& queryable) const noexcept {
            if constexpr (requires { queryable.query(*this); }) {
                return queryable.query(*this);
            }
            else {
                return empty_env{};
            }
        }
    };
    inline constexpr get_env_t get_env{};

    template<typename QueryableT>
    using env_of_t = std::invoke_result_t<get_env_t, QueryableT>;

    // TODO: struct default_domain{};

    template<typename...>
    struct completion_signatures {};

    struct get_completion_signatures_t {
        template<typename SenderT, typename EnvT>
        [[nodiscard]] constexpr decltype(auto) operator()(SenderT&& sndr, EnvT&& env) const noexcept
        requires requires{ std::forward<SenderT>(sndr).get_completion_signatures(std::forward<EnvT>(env)); }
        {
            return std::forward<SenderT>(sndr).get_completion_signatures(std::forward<EnvT>(env));
        }

        template<typename SenderT, typename EnvT>
        requires requires{ typename std::remove_cvref_t<SenderT>::completion_signatures; }
        [[nodiscard]] constexpr auto operator()(const SenderT&, const EnvT&) const noexcept {
            return typename std::remove_cvref_t<SenderT>::completion_signatures{};
        }
    };
    inline constexpr get_completion_signatures_t get_completion_signatures{};

    template<typename sender_t, typename env_t = empty_env>
    using completion_signatures_of_t = std::invoke_result_t<get_completion_signatures_t, sender_t, env_t>;

    namespace details {
        template<typename...>
        inline constexpr bool is_completion_signatures = false;

        template<typename... Ts>
        inline constexpr bool is_completion_signatures<completion_signatures<set_value_t(Ts...)>> = true;

        template<typename T>
        inline constexpr bool is_completion_signatures<completion_signatures<set_error_t(T)>> = true;

        template<>
        inline constexpr bool is_completion_signatures<completion_signatures<set_stopped_t()>> = true;

        template<typename... SignatureTs>
        inline constexpr bool is_completion_signatures<completion_signatures<SignatureTs...>> =
            (is_completion_signatures<completion_signatures<SignatureTs>> && ... && true);

        template<typename T>
        concept valid_completion_signatures = is_completion_signatures<std::remove_cvref_t<T>>;
    }

    struct receiver_t {};

    template<typename T>
    concept receiver =
        std::derived_from<typename std::remove_cvref_t<T>::receiver_concept, receiver_t> &&
        requires(const std::remove_cvref_t<T>& rcvr) {
            { get_env(rcvr) } -> queryable;
        } &&
        std::is_move_constructible_v<std::remove_cvref_t<T>> &&
        std::constructible_from<std::remove_cvref_t<T>, T>;

    namespace details {
        template<typename SignatureT, typename ReceiverT>
        concept valid_completion_for =
            requires(SignatureT* sig) {
                []<typename TagT, typename... Ts>(TagT(*)(Ts...))
                    requires std::invocable<TagT, std::remove_cvref_t<ReceiverT>, Ts...> {}(sig);
            };

        template<typename ReceiverT, typename SignatureT>
        concept has_completion =
            requires(SignatureT* sig) {
                []<valid_completion_for<ReceiverT>... Ts>(completion_signatures<Ts...>*) {}(sig);
            };
    }

    template<typename ReceiverT, typename CompletionSignaturesT>
    concept receiver_of = receiver<ReceiverT> && details::has_completion<ReceiverT, CompletionSignaturesT>;

    struct sender_t {};

    template<typename T>
    concept sender =
        std::derived_from<typename std::remove_cvref_t<T>::sender_concept, sender_t> &&
        requires(const std::remove_cvref_t<T>& sndr) {
            { get_env(sndr) } -> queryable;
        } &&
        std::is_move_constructible_v<std::remove_cvref_t<T>> &&
        std::constructible_from<std::remove_cvref_t<T>, T>;

    struct connect_t {
        template<typename SenderT, typename ReceiverT>
        [[nodiscard]] constexpr auto operator()(SenderT&& sender, ReceiverT&& receiver) const noexcept {
            return std::forward<SenderT>(sender).connect(std::forward<ReceiverT>(receiver));
        }
    };
    inline constexpr connect_t connect{};

    template<typename SenderT, typename ReceiverT>
    using connect_result_t = decltype(connect(std::declval<SenderT>(), std::declval<ReceiverT>()));

    template<typename sender_t, typename env_t = empty_env>
    concept sender_in =
        sender<sender_t> &&
        queryable<env_t> &&
        requires(sender_t&& sndr, env_t&& env) {
            { get_completion_signatures(std::forward<sender_t>(sndr), std::forward<env_t>(env)) } ->
                details::valid_completion_signatures;
        };

    template<typename sender_t, typename receiver_t>
    concept sender_to =
        sender_in<sender_t, env_of_t<receiver_t>> &&
        receiver_of<receiver_t, completion_signatures_of_t<sender_t, env_of_t<receiver_t>>> &&
        requires(sender_t&& sndr, receiver_t&& rcvr) {
            connect(std::forward<sender_t>(sndr), std::forward<receiver_t>(rcvr));
        };

    namespace details {
        template<typename... Ts>
        using default_set_value_t = completion_signatures<set_value_t(Ts...)>;

        template<typename... Ts>
        using default_set_error_t = completion_signatures<set_error_t(Ts)...>;

        template<template<typename...> typename, typename...>
        struct unique_template {};

        template<template<typename...> typename TypeListT, typename... Ts>
        struct unique_template<TypeListT, TypeListT<Ts...>> {
            using type = TypeListT<Ts...>;
        };

        template<template<typename...> typename TypeListT, typename... ListTs, typename T, typename... Ts>
        struct unique_template<TypeListT, TypeListT<ListTs...>, T, Ts...> {
            using type = std::conditional_t<
                (false | ... | std::is_same_v<T, Ts>),
                typename unique_template<TypeListT, TypeListT<ListTs...>, Ts...>::type,
                typename unique_template<TypeListT, TypeListT<ListTs..., T>, Ts...>::type>;
        };

        template<template<typename...> typename TypeListT, typename... Ts>
        using unique_template_t = unique_template<TypeListT, TypeListT<>, Ts...>::type;

        template<template<typename...> typename, typename...>
        struct meta_add {};

        template<template<typename...> typename TypeListT>
        struct meta_add<TypeListT> {
            using type = TypeListT<>;
        };

        template<template<typename...> typename TypeListT, typename... ArgTs>
        struct meta_add<TypeListT, TypeListT<ArgTs...>> {
            using type = unique_template_t<TypeListT, ArgTs...>;
        };

        template<template<typename...> typename TypeListT, typename... LArgTs, typename... RArgTs>
        struct meta_add<TypeListT, TypeListT<LArgTs...>, TypeListT<RArgTs...>> {
            using type = unique_template_t<TypeListT, LArgTs..., RArgTs...>;
        };

        template<template<typename...> typename TypeListT, typename... LArgTs, typename... RArgTs, typename... Ts>
        struct meta_add<TypeListT, TypeListT<LArgTs...>, TypeListT<RArgTs...>, Ts...> {
            using type = meta_add<TypeListT, unique_template_t<TypeListT, LArgTs..., RArgTs...>, Ts...>::type;
        };

        template<template<typename...> typename TypeListT, typename... ArgTs>
        using meta_add_t = meta_add<TypeListT, ArgTs...>::type;

        template<bool>
        struct indirect_meta_apply {
            template<template<typename...> typename T, typename... ArgTs>
            using meta_apply = T<ArgTs...>;
        };

        template<typename...>
        concept always_true = true;

        template<template<typename...> typename T, typename... ArgTs>
        using meta_apply_t = indirect_meta_apply<always_true<ArgTs...>>::template meta_apply<T, ArgTs...>;

        template<typename...>
        struct signature_info {};

        template<typename TagT, typename... Ts>
        struct signature_info<TagT(Ts...)> {
            using tag = TagT;

            template<template<typename...> typename T>
            using apply = meta_apply_t<T, Ts...>;

            template<typename InvocableT>
            using invoke_result = std::invoke_result_t<InvocableT, Ts...>;
        };

        template<template<typename...> typename, typename...>
        struct select_signatures_by_tag {};

        template<template<typename...> typename TypeListT, typename TagT, typename... SignatureTs>
        struct select_signatures_by_tag<TypeListT, TagT, completion_signatures<SignatureTs...>, completion_signatures<>> {
            using self = select_signatures_by_tag;

            using type = TypeListT<SignatureTs...>;

            template<template<typename...> typename T>
            using apply = T<SignatureTs...>;
        };

        template<template<typename...> typename TypeListT, typename TagT, typename... SignatureTs, typename T, typename... Ts>
        struct select_signatures_by_tag<TypeListT, TagT, completion_signatures<SignatureTs...>, completion_signatures<T, Ts...>> {
            using self = std::conditional_t<
                std::is_same_v<TagT, typename signature_info<T>::tag>,
                typename select_signatures_by_tag<TypeListT, TagT, completion_signatures<SignatureTs..., T>, completion_signatures<Ts...>>::self,
                typename select_signatures_by_tag<TypeListT, TagT, completion_signatures<SignatureTs...>, completion_signatures<Ts...>>::self
            >;

            using type = self::type;

            template<template<typename...> typename type>
            using apply = self::template apply<type>;
        };

        template<template<typename...> typename TypeListT, typename TagT, typename SignatureT>
        using select_signatures_by_tag_t = select_signatures_by_tag<TypeListT, TagT, TypeListT<>, SignatureT>::self;

        template<template<typename...> typename TupleT, template<typename...> typename VariantT>
        struct meta_apply_helper {
            template<typename... Ts>
            using apply = meta_apply_t<VariantT, typename signature_info<Ts>::template apply<TupleT>...>;
        };

        template<typename... Ts>
        using add_signatures = meta_add_t<completion_signatures, Ts...>;

        template<typename TagT,
                 valid_completion_signatures SignatureT,
                 template<typename...> typename TupleT,
                 template<typename...> typename VariantT>
        using gather_signatures =
            select_signatures_by_tag_t<completion_signatures, TagT, SignatureT>::template apply<meta_apply_helper<TupleT, VariantT>::template apply>;

        template<typename StoppedT>
        struct stopped_wrapper {
            template<typename...>
            using type = StoppedT;
        };

        struct empty_variant {
            empty_variant() = delete;
        };

        template<typename... Ts>
        struct variant_or_empty {
            using type = std::variant<std::decay_t<Ts>...>;
        };

        template<>
        struct variant_or_empty<> {
            using type = empty_variant;
        };

        template<typename... Ts>
        using decayed_tuple = std::tuple<std::decay_t<Ts>...>;
    }

    template<typename SenderT,
             typename EnvT = empty_env,
             template<typename...> typename TupleT = details::decayed_tuple,
             template<typename...> typename VariantT = details::variant_or_empty>
    requires sender_in<SenderT, EnvT>
    using value_types_of_t = details::gather_signatures<set_value_t, completion_signatures_of_t<SenderT, EnvT>, TupleT, VariantT>;

    template<typename SenderT,
             typename EnvT = empty_env,
             template<typename...> typename VariantT = details::variant_or_empty>
    requires sender_in<SenderT, EnvT>
    using error_types_of_t =
        details::gather_signatures<set_error_t, completion_signatures_of_t<SenderT, EnvT>, std::type_identity_t, VariantT>;

    template<typename SenderT, typename EnvT = empty_env>
    requires sender_in<SenderT, EnvT>
    inline constexpr bool sends_stopped =
        !std::is_same_v<completion_signatures<>,
                        details::gather_signatures<set_stopped_t,
                                                   completion_signatures_of_t<SenderT, EnvT>,
                                                   details::stopped_wrapper<completion_signatures<set_stopped_t()>>::template type,
                                                   completion_signatures>>;

    // TODO: using tag_of_t = ;

    template<typename InputSignaturesT,
             typename AdditionalSignaturesT = completion_signatures<>,
             template<typename...> typename SetValueT = details::default_set_value_t,
             template<typename> typename SetErrorT = details::default_set_error_t,
             typename SetStoppedT = completion_signatures<set_stopped_t()>>
    using transform_completion_signatures = details::meta_add_t<
                completion_signatures,
                AdditionalSignaturesT,
                details::gather_signatures<set_value_t,
                                           InputSignaturesT,
                                           SetValueT,
                                           details::add_signatures>,
                details::gather_signatures<set_error_t,
                                           InputSignaturesT,
                                           std::type_identity_t,
                                           SetErrorT>,
                details::gather_signatures<set_stopped_t,
                                           InputSignaturesT,
                                           details::stopped_wrapper<SetStoppedT>::template type,
                                           details::add_signatures>
            >;

    template<typename SenderT,
             typename EnvT = empty_env,
             typename AdditionalSignaturesT = completion_signatures<>,
             template<typename...> typename SetValueT = details::default_set_value_t,
             template<typename> typename SetErrorT = details::default_set_error_t,
             typename SetStoppedT = completion_signatures<set_stopped_t()>>
    using transform_completion_signatures_of =
        transform_completion_signatures<completion_signatures_of_t<SenderT, EnvT>,
                                        AdditionalSignaturesT,
                                        SetValueT, SetErrorT, SetStoppedT>;

    struct scheduler_t {};

    struct schedule_t {
        [[nodiscard]] constexpr sender auto operator()(auto&& schd) const noexcept {
            return std::forward<decltype(schd)>(schd).schedule();
        }
    };
    inline constexpr schedule_t schedule{};

    template<typename T>
    concept scheduler =
        std::derived_from<typename std::remove_cvref_t<T>::scheduler_concept, scheduler_t> &&
        queryable<T> &&
        requires(T&& schd) {
            { schedule(std::forward<T>(schd)) } -> sender;
            { auto(get_completion_scheduler<set_value_t>(get_env(schedule(std::forward<T>(schd))))) } ->
                std::same_as<std::remove_cvref_t<T>>;
        } &&
        std::is_copy_constructible_v<T> &&
        std::equality_comparable<T>;

    struct operation_state_t {};

    template <typename T>
    concept operation_state =
        std::derived_from<typename T::operation_state_concept, operation_state_t> &&
        std::is_object_v<T> &&
        requires(T& op) {
            { op.start() } noexcept;
        };

    struct start_t {
        template<typename T>
        constexpr auto operator()(T&& op) const noexcept {
            std::forward<T>(op).start();
        }
    };
    inline constexpr start_t start{};

    // TODO: constexpr sender decltype(auto) transform_sender(...) noexcept(...) {}
    // TODO: constexpr sender decltype(auto) transform_env(...) noexcept(...) {}
    // TODO: constexpr sender decltype(auto) apply_sender(...) noexcept(...) {}

    template<typename TagT, receiver ReceiverT, typename... ValueTs>
    struct just_operation_state {
        using operation_state_concept = operation_state_t;

        [[no_unique_address]] ReceiverT receiver;
        [[no_unique_address]] std::tuple<ValueTs...> tuple;

        template<typename Self>
        constexpr void start(this Self&& self) noexcept {
            std::apply(
                [&]<typename... Ts>(Ts&&... values) {
                    TagT{}(std::forward_like<Self>(self.receiver), std::forward<Ts>(values)...);
                },
                std::forward_like<Self>(self.tuple)
            );
        }
    };

    template<typename TagT, typename... ValueTs>
    struct just_sender {
        using sender_concept = sender_t;

        using completion_signatures = completion_signatures<TagT(ValueTs...)>;

        [[no_unique_address]] std::tuple<ValueTs...> tuple;

        template<typename Self, receiver ReceiverT>
        [[nodiscard]] constexpr auto connect(this Self&& self, ReceiverT&& rcvr) noexcept ->
            just_operation_state<TagT, std::decay_t<ReceiverT>, ValueTs...>
        {
            return { std::forward<ReceiverT>(rcvr), std::forward_like<Self>(self.tuple) };
        }
    };

    struct just_t {
        template<typename... Ts>
        [[nodiscard]] constexpr auto operator()(Ts&&... values) const noexcept ->
            just_sender<set_value_t, std::decay_t<Ts>...>
        {
            return { { std::forward<Ts>(values)... } };
        }
    };
    inline constexpr just_t just{};

    struct just_error_t {
        template<typename T>
        [[nodiscard]] constexpr auto operator()(T&& value) const noexcept ->
            just_sender<set_error_t, std::decay_t<T>>
        {
            return { std::forward<T>(value) };
        }
    };
    inline constexpr just_error_t just_error{};

    struct just_stopped_t {
        [[nodiscard]] constexpr auto operator()() const noexcept -> just_sender<set_stopped_t> {
            return {};
        }
    };
    inline constexpr just_stopped_t just_stopped{};

    template<typename CreatorT, typename... MemberTs>
    struct pipe {
        [[no_unique_address]] CreatorT creator;
        [[no_unique_address]] std::tuple<MemberTs...> members;

        [[nodiscard]] friend constexpr sender auto operator|(sender auto&& sndr, pipe&& self) noexcept {
            return std::apply([&]<typename... Ts>(Ts&&... args) constexpr {
                return std::forward_like<decltype(self)>(self.creator(std::forward<decltype(sndr)>(sndr), std::forward<Ts>(args)...));
            }, std::forward_like<decltype(self)>(self.members));
        }
    };

    template<typename OperationT>
    struct let_receiver {
        using receiver_concept = receiver_t;

        OperationT* op;

        void set_value(auto&&... values) {
            op->template process<set_value_t>(std::forward<decltype(values)>(values)...);
        }

        void set_error(auto&& value) {
            op->template process<set_error_t>(std::forward<decltype(value)>(value));
        }

        void set_stopped() {
            op->template process<set_stopped_t>();
        }
    };

    namespace details {
        template<template<typename...> typename, typename...>
        struct enable_result {};

        template<template<typename...> typename TypeListT, typename InvocableT, typename... Ts>
        requires requires { std::invoke(std::declval<InvocableT>(), std::declval<Ts>()...); } && (!std::is_void_v<std::invoke_result_t<InvocableT, Ts...>>)
        struct enable_result<TypeListT, InvocableT, TypeListT<Ts...>> {
            using type = std::invoke_result_t<InvocableT, Ts...>;
        };

        template<template<typename...> typename TypeListT, typename InvocableT, typename ReceiverT, typename... SignatureTs>
        using compute_result_t =
            unique_template_t<TypeListT,
                              connect_result_t<typename signature_info<SignatureTs>::template invoke_result<InvocableT>, ReceiverT>...>;

        template<template<typename...> typename TypeListT, typename EnvT>
        struct bind_env_wrapper {
            template<typename... Ts>
            using type = TypeListT<EnvT, Ts...>;
        };
    }

    template<typename...>
    struct let_operation_state {};

    template<typename TagT, typename SenderT, typename ReceiverT, typename FnT, typename... SignatureTs>
    struct let_operation_state<TagT, SenderT, ReceiverT, FnT, completion_signatures<SignatureTs...>> {
        using operation_state_concept = operation_state_t;

        using first_op_t = connect_result_t<SenderT, let_receiver<let_operation_state>>;
        using second_op_variant_t = details::compute_result_t<std::variant, FnT, ReceiverT, SignatureTs...>;

        using state_t = std::variant<SenderT, first_op_t, second_op_variant_t>;

        state_t state;
        FnT fn;
        ReceiverT receiver;

        template<typename T>
        void passthrough(this auto&& self, auto&&... values) {
            T{}(std::forward_like<decltype(self)>(self.receiver), std::forward<decltype(values)>(values)...);
        }

        template<typename T>
        requires std::is_same_v<TagT, T>
        void process(this auto&& self, auto&&... values) {
            try {
                auto& result =
                    self.state.template emplace<second_op_variant_t>(
                        exec::connect(
                            std::invoke(std::forward_like<decltype(self)>(self.fn), std::forward<decltype(values)>(values)...),
                            std::forward_like<decltype(self)>(self.receiver)
                        )
                    );

                std::visit([&]<typename op_t>(op_t&& op) { exec::start(std::forward<op_t>(op)); }, result);
            }
            catch (...) {
                set_error(std::forward_like<decltype(self)>(self.receiver), std::current_exception());
            }
        }

        template<typename T>
        void process(this auto&& self, auto&&... values) {
            self.template passthrough<T>(std::forward<decltype(values)>(values)...);
        }

        void start(this auto&& self) noexcept {
            auto& op =
                self.state.template emplace<first_op_t>(
                    exec::connect(std::get<SenderT>(std::forward_like<decltype(self)>(self.state)), let_receiver{ &self })
                );

            exec::start(op);
        }
    };

    template<typename TagT, sender SenderT, typename FnT>
    struct let_sender {
        using sender_concept = sender_t;

        SenderT sender;
        FnT fn;

        template<typename EnvT, typename... ArgTs>
        requires std::invocable<FnT, ArgTs...>
        using signature_t = completion_signatures_of_t<std::invoke_result_t<FnT, ArgTs...>, EnvT>;

        template<typename EnvT>
        requires std::is_same_v<TagT, set_value_t>
        [[nodiscard]] constexpr auto get_completion_signatures(const EnvT&) const noexcept {
            return transform_completion_signatures_of<SenderT, EnvT, completion_signatures<>, details::bind_env_wrapper<signature_t, EnvT>::template type>{};
        }

        template<typename EnvT>
        requires std::is_same_v<TagT, set_error_t>
        [[nodiscard]] constexpr auto get_completion_signatures(const EnvT&) const noexcept {
            return transform_completion_signatures_of<SenderT, EnvT, completion_signatures<>, details::default_set_value_t, details::bind_env_wrapper<signature_t, EnvT>::template type>{};
        }

        template<typename EnvT>
        requires std::is_same_v<TagT, set_stopped_t>
        [[nodiscard]] constexpr auto get_completion_signatures(const EnvT&) const noexcept {
            return transform_completion_signatures_of<SenderT, EnvT, completion_signatures<>, details::default_set_value_t, details::default_set_error_t, signature_t<EnvT>>{};
        }

        [[nodiscard]] constexpr decltype(auto) query(get_env_t) const noexcept {
            return forward_env(get_env(sender));
        }

        template<receiver ReceiverT>
        [[nodiscard]] constexpr auto connect(this auto&& self, ReceiverT&& rcvr) noexcept ->
            let_operation_state<TagT,
                                SenderT,
                                std::decay_t<ReceiverT>,
                                FnT,
                                typename details::select_signatures_by_tag<completion_signatures,
                                                                           TagT,
                                                                           completion_signatures<>,
                                                                           completion_signatures_of_t<SenderT, env_of_t<ReceiverT>>>::type>
        {
            return {
                { std::forward_like<decltype(self)>(self.sender) },
                std::forward_like<decltype(self)>(self.fn),
                std::forward<ReceiverT>(rcvr)
            };
        }
    };

    struct let_value_t {
        [[nodiscard]] constexpr auto operator()(sender auto&& input, auto&& invocable) const noexcept ->
            let_sender<set_value_t, std::decay_t<decltype(input)>, std::decay_t<decltype(invocable)>>
        {
            return { std::forward<decltype(input)>(input), std::forward<decltype(invocable)>(invocable) };
        }

        [[nodiscard]] constexpr auto operator()(auto&& invocable) const noexcept {
            return pipe {
                []<typename... Ts>(Ts&&... args) constexpr {
                    return let_sender<set_value_t, Ts...>{ std::forward<Ts>(args)... };
                },
                std::make_tuple(std::forward<decltype(invocable)>(invocable))
            };
        }
    };
    inline constexpr let_value_t let_value{};

    struct let_error_t {
        [[nodiscard]] constexpr auto operator()(sender auto&& input, auto&& invocable) const noexcept ->
            let_sender<set_error_t, std::decay_t<decltype(input)>, std::decay_t<decltype(invocable)>>
        {
            return { std::forward<decltype(input)>(input), std::forward<decltype(invocable)>(invocable) };
        }

        [[nodiscard]] constexpr auto operator()(auto&& invocable) const noexcept {
            return pipe {
                []<typename... Ts>(Ts&&... args) constexpr {
                    return let_sender<set_error_t, Ts...>{ std::forward<Ts>(args)... };
                },
                std::make_tuple(std::forward<decltype(invocable)>(invocable))
            };
        }
    };
    inline constexpr let_error_t let_error{};

    struct let_stopped_t {
        [[nodiscard]] constexpr auto operator()(sender auto&& input, std::invocable auto&& invocable) const noexcept ->
            let_sender<set_stopped_t, std::decay_t<decltype(input)>, std::decay_t<decltype(invocable)>>
        {
            return { std::forward<decltype(input)>(input), std::forward<decltype(invocable)>(invocable) };
        }

        [[nodiscard]] constexpr auto operator()(std::invocable auto&& invocable) const noexcept {
            return pipe {
                []<typename... Ts>(Ts&&... args) constexpr {
                    return let_sender<set_stopped_t, Ts...>{ std::forward<Ts>(args)... };
                },
                std::make_tuple(std::forward<decltype(invocable)>(invocable))
            };
        }
    };
    inline constexpr let_stopped_t let_stopped{};

    struct starts_on_t {
        [[nodiscard]] constexpr auto operator()(scheduler auto&& scheduler, sender auto&& sender) const noexcept {
            return let_value(schedule(std::forward<decltype(scheduler)>(scheduler)),
                             [s = std::forward<decltype(sender)>(sender)] { return s; });
        }
    };
    inline constexpr starts_on_t starts_on{};

    struct continues_on_t {
        constexpr auto operator()(sender auto&& input, scheduler auto&& scheduler) const noexcept {
            return let_value(input, [schd = std::forward<decltype(scheduler)>(scheduler)]<typename... value_ts>(value_ts&&... values) {
                return starts_on(schd, just(std::forward<value_ts>(values)...));
            });
        }

        constexpr auto operator()(scheduler auto&& scheduler) const noexcept {
            return pipe{
                [this]<typename input_t, typename scheduler_t>(input_t&& input, scheduler_t&& schd) constexpr {
                    return this->operator()(std::forward<input_t>(input), std::forward<scheduler_t>(schd));
                },
                std::make_tuple(std::forward<decltype(scheduler)>(scheduler))
            };
        }
    };
    inline constexpr continues_on_t continues_on{};

    template<typename TagT, receiver ReceiverT, typename FnT>
    struct then_receiver {
        using receiver_concept = receiver_t;

        ReceiverT receiver;
        FnT fn;

        void set_value(auto&&... values) {
            process<set_value_t>(std::forward<decltype(values)>(values)...);
        }

        void set_error(auto&& value) {
            process<set_error_t>(std::forward<decltype(value)>(value));
        }

        void set_stopped() {
            process<set_stopped_t>();
        }

    private:
        template<typename T>
        requires std::is_same_v<TagT, T>
        void process(this auto&& self, auto&&... values) {
            try {
                if constexpr (std::is_void_v<std::invoke_result_t<FnT, decltype(values)...>>) {
                    std::invoke(std::forward_like<decltype(self)>(self.fn), std::forward<decltype(values)>(values)...);
                    exec::set_value(std::forward_like<decltype(self)>(self.receiver));
                }
                else {
                    exec::set_value(std::forward_like<decltype(self)>(self.receiver),
                                    std::invoke(std::forward_like<decltype(self)>(self.fn), std::forward<decltype(values)>(values)...));
                }
            }
            catch (...) {
                exec::set_error(std::forward_like<decltype(self)>(self.receiver), std::current_exception());
            }
        }

        template<typename T>
        void process(this auto&& self, auto&&... values) {
            T{}(std::forward_like<decltype(self)>(self.receiver), std::forward<decltype(values)>(values)...);
        }
    };

    template<typename TagT, sender SenderT, typename FnT>
    struct then_sender {
        using sender_concept = sender_t;

        SenderT sender;
        FnT fn;

        template<typename... Ts>
        using signature = completion_signatures<set_value_t(std::invoke_result_t<FnT, Ts...>)>;

        template<typename EnvT>
        requires std::is_same_v<TagT, set_value_t>
        [[nodiscard]] constexpr auto get_completion_signatures(const EnvT&) const noexcept {
            return transform_completion_signatures_of<SenderT,
                                                      EnvT,
                                                      completion_signatures<set_error_t(std::exception_ptr)>,
                                                      signature>{};
        }

        template<typename EnvT>
        requires std::is_same_v<TagT, set_error_t>
        [[nodiscard]] constexpr auto get_completion_signatures(const EnvT&) const noexcept {
            return transform_completion_signatures_of<SenderT,
                                                      EnvT,
                                                      completion_signatures<set_error_t(std::exception_ptr)>,
                                                      details::default_set_value_t,
                                                      signature>{};
        }

        template<typename EnvT>
        requires std::is_same_v<TagT, set_stopped_t>
        [[nodiscard]] constexpr auto get_completion_signatures(const EnvT&) const noexcept {
            return transform_completion_signatures_of<SenderT,
                                                      EnvT,
                                                      completion_signatures<set_error_t(std::exception_ptr)>,
                                                      details::default_set_value_t,
                                                      details::default_set_error_t,
                                                      signature<>>{};
        }

        [[nodiscard]] constexpr decltype(auto) query(get_env_t) const noexcept {
            return forward_env(get_env(sender));
        }

        [[nodiscard]] constexpr auto connect(receiver auto&& rcvr) noexcept {
            return exec::connect(std::move(sender),
                                 then_receiver<TagT, std::decay_t<decltype(rcvr)>, FnT>{
                                      std::forward<decltype(rcvr)>(rcvr),
                                      std::move(fn)
                                 });
        }
    };

    struct then_t {
        [[nodiscard]] constexpr auto operator()(sender auto&& input, auto&& invocable) const noexcept ->
            then_sender<set_value_t, std::decay_t<decltype(input)>, std::decay_t<decltype(invocable)>>
        {
            return { std::forward<decltype(input)>(input), std::forward<decltype(invocable)>(invocable) };
        }

        [[nodiscard]] constexpr auto operator()(auto&& invocable) const noexcept {
            return pipe{
                []<typename... arg_ts>(arg_ts&&... args) constexpr {
                    return then_sender<set_value_t, std::decay_t<arg_ts>...>{ std::forward<arg_ts>(args)... };
                }, std::make_tuple(std::forward<decltype(invocable)>(invocable)) };
        }
    };
    inline constexpr then_t then{};

    struct upon_error_t {
        [[nodiscard]] constexpr auto operator()(sender auto&& input, auto&& invocable) const noexcept ->
            then_sender<set_error_t, std::decay_t<decltype(input)>, std::decay_t<decltype(invocable)>>
        {
            return { std::forward<decltype(input)>(input), std::forward<decltype(invocable)>(invocable) };
        }

        [[nodiscard]] constexpr auto operator()(auto&& invocable) const noexcept {
            return pipe{
                []<typename... arg_ts>(arg_ts&&... args) constexpr {
                    return then_sender<set_error_t, std::decay_t<arg_ts>...>{ std::forward<arg_ts>(args)... };
                }, std::make_tuple(std::forward<decltype(invocable)>(invocable)) };
        }
    };
    inline constexpr upon_error_t upon_error{};

    struct upon_stopped_t {
        [[nodiscard]] constexpr auto operator()(sender auto&& input, std::invocable auto&& invocable) const noexcept ->
            then_sender<set_stopped_t, std::decay_t<decltype(input)>, std::decay_t<decltype(invocable)>>
        {
            return { std::forward<decltype(input)>(input), std::forward<decltype(invocable)>(invocable) };
        }

        [[nodiscard]] constexpr auto operator()(std::invocable auto&& invocable) const noexcept {
            return pipe{
                []<typename... arg_ts>(arg_ts&&... args) constexpr {
                    return then_sender<set_stopped_t, std::decay_t<arg_ts>...>{ std::forward<arg_ts>(args)... };
                }, std::make_tuple(std::forward<decltype(invocable)>(invocable)) };
        }
    };
    inline constexpr upon_stopped_t upon_stopped{};

    class run_loop {
        struct operation_state_base {
            virtual ~operation_state_base() noexcept = default;

            virtual void run() = 0;
        };

        template<receiver ReceiverT>
        struct operation_state : operation_state_base {
            using operation_state_concept = operation_state_t;

            explicit operation_state(run_loop* loop, ReceiverT&& receiver) noexcept : loop{ loop }, receiver{ std::forward<ReceiverT>(receiver) } {}

            run_loop* loop;
            ReceiverT receiver;

            void run() override {
                if (get_stop_token(get_env(receiver)).stop_requested()) {
                    set_stopped(std::move(receiver));
                }
                else {
                    set_value(std::move(receiver));
                }
            }

            void start() {
                try {
                    loop->push_back(this);
                }
                catch (...) {
                    set_error(std::move(receiver), std::current_exception());
                }
            }
        };

        struct scheduler {
            struct sender {
                struct env {
                    run_loop* loop;

                    [[nodiscard]] constexpr auto query(get_completion_scheduler_t<set_value_t>) const noexcept {
                        return loop->get_scheduler();
                    }

                    [[nodiscard]] constexpr auto query(get_completion_scheduler_t<set_error_t>) const noexcept {
                        return loop->get_scheduler();
                    }

                    [[nodiscard]] constexpr auto query(get_completion_scheduler_t<set_stopped_t>) const noexcept {
                        return loop->get_scheduler();
                    }
                };

                using sender_concept = sender_t;

                using completion_signatures = completion_signatures<set_value_t(), set_error_t(std::exception_ptr), set_stopped_t()>;

                run_loop* loop;

                [[nodiscard]] auto query(get_env_t) const noexcept {
                    return env{ loop };
                }

                constexpr auto connect(receiver auto&& rcvr) noexcept {
                    return operation_state<std::decay_t<decltype(rcvr)>>(loop, std::forward<decltype(rcvr)>(rcvr));
                }
            };

            using scheduler_concept = scheduler_t;

            run_loop* loop;

            [[nodiscard]] constexpr sender schedule() const noexcept {
                return sender{ loop };
            }

        private:
            friend constexpr bool operator==(const scheduler& left, const scheduler& right) noexcept {
                return left.loop == right.loop;
            }

        };

    public:
        run_loop() noexcept : m_finished{ false }, m_worker_count{ 0 } {}

        run_loop(run_loop&&) = delete;

        ~run_loop() noexcept {
            size_t task_count{};
            bool stopped{};
            {
                std::scoped_lock lock(m_mutex);
                task_count = m_queue.size();
                stopped = m_finished;
            }

            if (task_count > 0 || !stopped) {
                std::terminate();
            }
        }

        constexpr scheduler get_scheduler() {
            return scheduler{ this };
        }

        void run() {
            while (auto* task = pop_front()) {
                task->run();
            }
        }

        void finish() noexcept {
            {
                std::scoped_lock lock(m_mutex);
                m_finished = true;
            }
            m_cv.notify_all();
        }

    private:
        void push_back(operation_state_base* task) {
            {
                std::scoped_lock lock(m_mutex);
                if (m_finished) {
                    throw std::runtime_error("Invalid operation on finished run loop.");
                }

                m_queue.push(task);
            }
            m_cv.notify_one();
        }

        operation_state_base* pop_front() {
            std::unique_lock lock(m_mutex);

            m_cv.wait(lock, [this]() -> bool { return m_finished || !m_queue.empty(); });
            if (m_finished && m_queue.empty()) return nullptr;

            auto* task = m_queue.front();
            m_queue.pop();

            return task;
        }

        mutable std::mutex m_mutex;

        bool m_finished;
        std::atomic_size_t m_worker_count;
        std::condition_variable m_cv;
        std::queue<operation_state_base*> m_queue;
    };

    namespace details {
        template<template<typename...> typename FallbackT>
        struct conditional_type_identify {
            template<typename... ts>
            using type = std::conditional_t<sizeof...(ts) != 1, FallbackT<>, std::tuple_element_t<0, std::tuple<ts..., int>>>;
        };

        template<typename... Ts>
        using decayed_variant = unique_template_t<std::variant, std::monostate, std::exception_ptr, std::decay_t<Ts>...>;

        template<typename SenderT, typename EnvT = empty_env>
        using sync_wait_error_type = error_types_of_t<SenderT, EnvT, decayed_variant>;

        template<typename SenderT, typename EnvT = empty_env>
        using sync_wait_result_type = std::optional<value_types_of_t<SenderT, EnvT, decayed_tuple, conditional_type_identify<std::tuple>::template type>>;

        struct sync_wait_error_handler_t {
            static void operator()(std::exception_ptr e) {
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
            [[no_unique_address]] sync_wait_error_type<SenderT, sync_wait_env> error;
            [[no_unique_address]] sync_wait_result_type<SenderT, sync_wait_env> result;
        };
    }

    template<typename SenderT>
    struct sync_wait_receiver {
        using receiver_concept = receiver_t;

        details::sync_wait_state<SenderT>* state;

        void set_value(auto&&... values) noexcept {
            try {
                state->result.emplace(std::forward<decltype(values)>(values)...);
            }
            catch (...) {
                state->error.template emplace<std::exception_ptr>(std::current_exception());
            }

            state->loop.finish();
        }

        template<typename ValueT>
        void set_error(ValueT&& value) noexcept {
            try {
                state->error.template emplace<std::decay_t<ValueT>>(std::forward<ValueT>(value));
            }
            catch (...) {
                state->error.template emplace<std::exception_ptr>(std::current_exception());
            }

            state->loop.finish();
        }

        void set_stopped() {
            state->result = std::nullopt;

            state->loop.finish();
        }
    };

    struct sync_wait_t {
        [[nodiscard]] constexpr auto operator()(sender auto&& input) const {
            details::sync_wait_state<std::decay_t<decltype(input)>> state{};

            auto op = exec::connect(std::forward<decltype(input)>(input), sync_wait_receiver<std::remove_cvref_t<decltype(input)>>{ &state });
            exec::start(op);

            state.loop.run();
            if (!std::holds_alternative<std::monostate>(state.error)) {
                std::visit(details::sync_wait_error_handler, std::move(state.error));
            }

            return std::move(state.result);
        }
    };
    inline constexpr sync_wait_t sync_wait{};
}

#endif // !EXEC_EXEC_HPP