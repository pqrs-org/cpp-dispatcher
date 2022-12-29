#include <boost/ut.hpp>
#include <iostream>
#include <pqrs/dispatcher.hpp>

namespace {
class timer_test final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  timer_test(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
             size_t& count,
             std::chrono::milliseconds duration,
             std::function<void(void)> function)
      : dispatcher_client(weak_dispatcher),
        timer_(*this) {
    timer_.start(
        [&count, function] {
          ++count;
          function();
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

void run_timer_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "dispatcher.timer"_test = [] {
    std::cout << "dispatcher.timer" << std::endl;

    auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();

    {
      size_t count = 0;

      {
        auto d = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

        auto t = std::make_unique<timer_test>(d, count, std::chrono::milliseconds(100), [] {});

        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        t->stop();
        t = nullptr;

        expect(count > 2);
        expect(count < 8);

        d->terminate();
      }
    }
  };

  "dispatcher.timer (destroy timer itself in callback)"_test = [] {
    std::cout << "dispatcher.timer (destroy timer itself in callback)" << std::endl;

    auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();

    {
      size_t count = 0;

      auto d = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

      std::shared_ptr<timer_test> t;

      t = std::make_shared<timer_test>(d, count, std::chrono::milliseconds(100), [&t] { t = nullptr; });

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      expect(count == 1_i);

      d->terminate();
    }
  };

  "dispatcher.timer (stop timer immediately)"_test = [] {
    std::cout << "dispatcher.timer (stop timer immediately)" << std::endl;

    auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();

    {
      size_t count = 0;

      auto d = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

      std::shared_ptr<timer_test> t;

      t = std::make_shared<timer_test>(d, count, std::chrono::milliseconds(100), [] {});
      t->stop();

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      expect(count == 1_i);

      d->terminate();
    }
  };
}
