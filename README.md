# Exec
This library was created as part of my work to understand the [P2300](https://cplusplus.github.io/sender-receiver/execution.html) proposal. 
It might look messy or terrible, but it works well for my use cases. (Probably) ¯\\_(ツ)_/¯

### Requirements
* C++23
* MSVC 19.34+
* GCC 14+

### Example
```c++
#include <exec.hpp>

#include <print>
#include <format>
#include <string>
#include <string_view>

int main() {
    exec::sender auto sndr = exec::just("Hello") |
                             exec::then([](std::string_view str) { return std::format("{} world!", str); });

    auto [result] = exec::sync_wait(sndr).value();
    std::println("{}", result);

    return 0;
}
```
