#ifndef EXEC_APPLY_SENDER_HPP
#define EXEC_APPLY_SENDER_HPP

#include "exec/domain.hpp"
#include "exec/sender.hpp"

namespace exec {
    template<typename DomainT, typename TagT, sender SenderT, typename... ArgTs>
    [[nodiscard]] constexpr decltype(auto) apply_sender(DomainT domain, TagT tag, SenderT&& sender, ArgTs&&... args) {
        if constexpr (requires { std::declval<DomainT>().apply_sender(tag, std::declval<SenderT>(), std::declval<ArgTs>()...); }) {
            return domain.apply_sender(tag, std::forward<SenderT>(sender), std::forward<ArgTs>(args)...);
        }
        else {
            return default_domain{}.apply_sender(tag, std::forward<SenderT>(sender), std::forward<ArgTs>(args)...);
        }
    }
}

#endif //EXEC_APPLY_SENDER_HPP