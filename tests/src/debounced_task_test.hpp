#include <boost/ut.hpp>
#include <functional>
#include <iostream>
#include <pqrs/dispatcher.hpp>
#include <thread>
#include <utility>
#include <vector>

namespace {
class debounced_task_test final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  debounced_task_test(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher)
      : dispatcher_client(weak_dispatcher),
        debounced_task_(*this) {
  }

  ~debounced_task_test() override {
    debounced_task_.cancel();

    detach_from_dispatcher([] {
    });
  }

  bool debounce_at(std::function<void()> function,
                   pqrs::dispatcher::time_point when) {
    return debounced_task_.debounce_at(std::move(function), when);
  }

  bool debounce_after(std::function<void()> function,
                      pqrs::dispatcher::duration delay) {
    return debounced_task_.debounce_after(std::move(function), delay);
  }

  void cancel() {
    debounced_task_.cancel();
  }

  void wait_until_dispatched() {
    auto wait = pqrs::make_thread_wait();

    enqueue_to_dispatcher([wait] {
      wait->notify();
    });

    wait->wait_notice();
  }

  void wait_at(pqrs::dispatcher::time_point when) {
    auto wait = pqrs::make_thread_wait();

    enqueue_to_dispatcher(
        [wait] {
          wait->notify();
        },
        when);

    wait->wait_notice();
  }

private:
  pqrs::dispatcher::extra::debounced_task debounced_task_;
};
} // namespace

void run_debounced_task_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "dispatcher.debounced_task"_test = [] {
    std::cout << "dispatcher.debounced_task" << std::endl;

    auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
    time_source->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(0)));

    {
      auto d = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);
      auto t = std::make_unique<debounced_task_test>(d);
      std::string text;

      expect(t->debounce_at(
          [&] {
            text += "a";
          },
          pqrs::dispatcher::time_point(std::chrono::milliseconds(100))));

      expect(t->debounce_after(
          [&] {
            expect(t->dispatcher_thread());
            text += "b";
          },
          std::chrono::milliseconds(200)));

      t->wait_until_dispatched();

      time_source->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(100)));
      d->invoke();
      t->wait_at(pqrs::dispatcher::time_point(std::chrono::milliseconds(100)));
      expect(text == "");

      time_source->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(200)));
      d->invoke();
      t->wait_at(pqrs::dispatcher::time_point(std::chrono::milliseconds(200)));
      expect(text == "b");

      t.reset();
      d->terminate();
    }
  };

  "dispatcher.debounced_task.cancel"_test = [] {
    std::cout << "dispatcher.debounced_task.cancel" << std::endl;

    auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
    time_source->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(0)));

    {
      auto d = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);
      auto t = std::make_unique<debounced_task_test>(d);
      size_t count = 0;

      expect(t->debounce_after(
          [&] {
            ++count;
          },
          std::chrono::milliseconds(100)));

      t->wait_until_dispatched();
      t->cancel();
      t->wait_until_dispatched();

      time_source->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(100)));
      d->invoke();
      t->wait_at(pqrs::dispatcher::time_point(std::chrono::milliseconds(100)));
      expect(count == 0_ul);

      t.reset();
      d->terminate();
    }
  };

  "dispatcher.debounced_task.thread_safe_debounce"_test = [] {
    std::cout << "dispatcher.debounced_task.thread_safe_debounce" << std::endl;

    auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
    time_source->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(0)));

    {
      auto d = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);
      auto t = std::make_unique<debounced_task_test>(d);
      size_t count = 0;

      std::vector<std::thread> threads;
      for (int i = 0; i < 100; ++i) {
        threads.emplace_back([&] {
          expect(t->debounce_after(
              [&] {
                ++count;
              },
              std::chrono::milliseconds(100)));
        });
      }

      for (auto& thread : threads) {
        thread.join();
      }

      t->wait_until_dispatched();

      time_source->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(100)));
      d->invoke();
      t->wait_at(pqrs::dispatcher::time_point(std::chrono::milliseconds(100)));
      expect(count == 1_ul);

      t.reset();
      d->terminate();
    }
  };

  "dispatcher.debounced_task.debounce_after_uses_call_time"_test = [] {
    std::cout << "dispatcher.debounced_task.debounce_after_uses_call_time" << std::endl;

    auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
    time_source->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(0)));

    {
      auto d = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);
      auto t = std::make_unique<debounced_task_test>(d);
      auto wait = pqrs::make_thread_wait();
      size_t count = 0;

      expect(t->enqueue_to_dispatcher([wait] {
        wait->wait_notice();
      }));

      expect(t->debounce_after(
          [&] {
            ++count;
          },
          std::chrono::milliseconds(100)));

      time_source->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(100)));
      wait->notify();
      d->invoke();
      t->wait_at(pqrs::dispatcher::time_point(std::chrono::milliseconds(100)));
      expect(count == 1_ul);

      t.reset();
      d->terminate();
    }
  };
}
