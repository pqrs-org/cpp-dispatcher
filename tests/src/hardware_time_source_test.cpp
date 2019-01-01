#include <catch2/catch.hpp>

#include <iostream>
#include <pqrs/dispatcher.hpp>

namespace {
class when_hardware_time_source_test final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  when_hardware_time_source_test(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
                                 size_t& count,
                                 pqrs::dispatcher::duration duration) : dispatcher_client(weak_dispatcher) {
    enqueue_to_dispatcher(
        [&count] {
          std::cout << "." << std::flush;
          ++count;
        },
        when_now() + duration);
  }

  virtual ~when_hardware_time_source_test(void) {
    detach_from_dispatcher([] {
    });
  }
};
} // namespace

TEST_CASE("dispatcher.when_hardware_time_source") {
  std::cout << "dispatcher.when_hardware_time_source" << std::endl;

  auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();

  {
    size_t count = 0;

    {
      auto d = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

      std::vector<std::unique_ptr<when_hardware_time_source_test>> objects;
      for (int i = 0; i < 10; ++i) {
        objects.push_back(std::make_unique<when_hardware_time_source_test>(
            d,
            count,
            std::chrono::milliseconds(i * 100)));
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      REQUIRE(count > 2);
      REQUIRE(count < 8);

      d->terminate();

      std::cout << std::endl;
    }
  }
}
