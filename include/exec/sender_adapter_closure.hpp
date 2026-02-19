#ifndef EXEC_SENDER_ADAPTER_CLOSURE_HPP
#define EXEC_SENDER_ADAPTER_CLOSURE_HPP

#include "exec/details/pipe.hpp"

namespace exec {
    template<typename>
    struct sender_adapter_closure : details::pipe_operator {};
}

#endif // !EXEC_SENDER_ADAPTER_CLOSURE_HPP