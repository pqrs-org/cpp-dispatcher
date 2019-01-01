#include <catch2/catch.hpp>

#include <iostream>
#include <pqrs/dispatcher.hpp>

namespace {
class multiple_thread_test1 final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  multiple_thread_test1(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher) {
  }

  virtual ~multiple_thread_test1(void) {
    detach_from_dispatcher([] {
    });
  }
};

class multiple_thread_test2 final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  multiple_thread_test2(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher) {
  }

  virtual ~multiple_thread_test2(void) {
    detach_from_dispatcher([this] {
      auto thread = std::thread([this] {
        auto t1 = std::make_unique<multiple_thread_test1>(weak_dispatcher_);
        t1 = nullptr;
      });
      thread.join();
    });
  }
};
} // namespace

TEST_CASE("dispatcher.multiple_thread_test") {
  std::cout << "dispatcher.multiple_thread_test" << std::endl;

  auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();
  auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

  auto t2 = std::make_unique<multiple_thread_test2>(dispatcher);
  t2 = nullptr;

  dispatcher->terminate();
  dispatcher = nullptr;
}
