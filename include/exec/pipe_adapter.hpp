#ifndef EXEC_PIPE_ADAPTER_HPP
#define EXEC_PIPE_ADAPTER_HPP

#include "exec/sender.hpp"

#include <tuple>
#include <utility>

namespace exec {
    template<typename CreatorT, typename... ArgTs>
    struct pipe_adapter {
        [[no_unique_address]] CreatorT creator;
        [[no_unique_address]] std::tuple<ArgTs...> args;

        [[nodiscard]] friend constexpr sender auto operator|(sender auto&& sndr, pipe_adapter&& self) noexcept {
            return std::apply([&]<typename... Ts>(Ts&&... args) constexpr {
                return std::forward_like<decltype(self)>(self.creator(std::forward<decltype(sndr)>(sndr), std::forward<Ts>(args)...));
            }, std::forward_like<decltype(self)>(self.args));
        }
    };
}

#endif // !EXEC_PIPE_ADAPTER_HPP