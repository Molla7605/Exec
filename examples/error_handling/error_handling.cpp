#include <exec.hpp>

#include <exception>
#include <print>
#include <string>
#include <string_view>

int main() {
    exec::sender auto sndr = exec::just("Some value") |
                             exec::then([](std::string_view str) -> std::string {
                                 throw std::runtime_error("Booom! error");

                                 return std::format("Receive {}", str);
                             });

    try {
        auto [value] = exec::sync_wait(sndr).value();
    }
    catch (const std::exception& e) {
        std::println("Catch exception: {}", e.what());
    }

    exec::sender auto sndr2 = sndr |
                              exec::upon_error([](std::exception_ptr e_ptr) -> std::string {
                                  try {
                                      std::rethrow_exception(e_ptr);
                                  }
                                  catch (const std::exception& e) {
                                      std::println("Catch exception and recover context: {}", e.what());
                                  }

                                  return "Invalid value";
                              });

    auto [value2] = exec::sync_wait(sndr2).value();
    std::println("Result: {}", value2);

    return 0;
}