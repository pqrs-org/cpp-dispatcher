#include <catch2/catch.hpp>

#include <iostream>
#include <pqrs/dispatcher.hpp>

namespace {
class timer_test final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  timer_test(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
             size_t& count,
             std::chrono::milliseconds duration) : dispatcher_client(weak_dispatcher),
                                                   timer_(*this) {
    timer_.start(
        [&count] {
          ++count;
        },
        duration);
  }

  void stop(void) {
    timer_.stop();
  }

  virtual ~timer_test(void) {
    timer_.stop();

    detach_from_dispatcher([] {
    });
  }

private:
  pqrs::dispatcher::extra::timer timer_;
};
} // namespace

TEST_CASE("dispatcher.timer") {
  std::cout << "dispatcher.timer" << std::endl;

  auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();

  {
    size_t count = 0;

    {
      auto d = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

      auto t = std::make_unique<timer_test>(d, count, std::chrono::milliseconds(100));

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      t->stop();
      t = nullptr;

      REQUIRE(count > 2);
      REQUIRE(count < 8);

      d->terminate();
    }
  }
}
