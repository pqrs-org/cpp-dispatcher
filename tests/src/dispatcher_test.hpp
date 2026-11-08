#include <atomic>
#include <boost/ut.hpp>
#include <iostream>
#include <pqrs/dispatcher.hpp>
#include <stdexcept>

void run_dispatcher_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "dispatcher"_test = [] {
    std::cout << "dispatcher" << std::endl;

    auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();

    {
      for (int i = 0; i < 10000; ++i) {
        pqrs::dispatcher::dispatcher d(time_source);
        d.terminate();
      }
    }

    {
      size_t count = 0;

      pqrs::dispatcher::dispatcher d(time_source);

      auto object_id = pqrs::dispatcher::make_new_object_id();
      expect(d.attach(object_id));

      expect(!d.dispatcher_thread());

      auto start = std::chrono::steady_clock::now();

      for (int i = 0; i < 10000; ++i) {
        expect(d.enqueue(
            object_id,
            [&d, &count, i] {
              ++count;
              if (i % 1000 == 0) {
                std::cout << "." << std::flush;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
              }

              expect(d.dispatcher_thread());
            }));
      }

      auto wait = pqrs::make_thread_wait();
      expect(d.enqueue(
          object_id,
          [wait] {
            wait->notify();
          }));
      wait->wait_notice();

      auto elapsed = std::chrono::steady_clock::now() - start;
      expect(elapsed > std::chrono::milliseconds(400));

      expect(d.detach(object_id));
      d.terminate();
    }

    std::cout << std::endl;
  };

  "dispatcher.preserve_the_order_of_entries"_test = [] {
    std::cout << "dispatcher.preserve_the_order_of_entries" << std::endl;

    auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();

    {
      std::string text;

      pqrs::dispatcher::dispatcher d(time_source);

      auto object_id = pqrs::dispatcher::make_new_object_id();
      expect(d.attach(object_id));

      expect(d.enqueue(
          object_id,
          [&] {
            text += "a";
          }));
      expect(d.enqueue(
          object_id,
          [&] {
            text += "b";
          }));
      expect(d.enqueue(
          object_id,
          [&] {
            text += "c";
          }));

      auto wait = pqrs::make_thread_wait();
      expect(d.enqueue(
          object_id,
          [wait] {
            wait->notify();
          }));
      wait->wait_notice();

      expect(d.detach(object_id));
      d.terminate();

      expect(text == "abc");
    }
  };

  "dispatcher.execution_wrapper"_test = [] {
    std::cout << "dispatcher.execution_wrapper" << std::endl;

    auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
    auto wrapper_finished = pqrs::make_thread_wait();
    std::atomic<bool> inside_wrapper{false};
    std::atomic<bool> captured_resource_released_inside_wrapper{false};
    std::atomic<bool> exception_caught_inside_wrapper{false};
    std::atomic<size_t> wrapper_count{0};
    std::atomic<size_t> function_count{0};

    pqrs::dispatcher::dispatcher d(
        time_source,
        [&](const std::function<void()>& function) {
          inside_wrapper = true;

          try {
            function();
          } catch (const std::runtime_error&) {
            exception_caught_inside_wrapper = inside_wrapper.load();
          }

          inside_wrapper = false;

          ++wrapper_count;
          wrapper_finished->notify();
        });

    auto object_id = pqrs::dispatcher::make_new_object_id();
    expect(d.attach(object_id));

    auto captured_resource = std::shared_ptr<int>(
        new int(0),
        [&](int* p) {
          captured_resource_released_inside_wrapper = inside_wrapper.load();
          delete p;
        });

    expect(d.enqueue(
        object_id,
        [&, captured_resource] {
          expect(inside_wrapper.load());
          ++function_count;
          throw std::runtime_error("test");
        }));
    captured_resource.reset();

    wrapper_finished->wait_notice();

    expect(wrapper_count.load() == 1);
    expect(function_count.load() == 1);
    expect(!inside_wrapper.load());
    expect(captured_resource_released_inside_wrapper.load());
    expect(exception_caught_inside_wrapper.load());

    expect(d.detach(object_id));
    d.terminate();
  };

  "shared_dispatcher.execution_wrapper"_test = [] {
    std::cout << "shared_dispatcher.execution_wrapper" << std::endl;

    auto wrapper_finished = pqrs::make_thread_wait();
    std::atomic<bool> inside_wrapper{false};
    std::atomic<size_t> wrapper_count{0};
    std::atomic<size_t> function_count{0};

    pqrs::dispatcher::extra::initialize_shared_dispatcher(
        [&](const std::function<void()>& function) {
          inside_wrapper = true;
          function();
          inside_wrapper = false;

          ++wrapper_count;
          wrapper_finished->notify();
        });

    auto d = pqrs::dispatcher::extra::get_shared_dispatcher();
    auto object_id = pqrs::dispatcher::make_new_object_id();
    expect(d->attach(object_id));

    expect(d->enqueue(
        object_id,
        [&] {
          expect(inside_wrapper.load());
          ++function_count;
        }));

    wrapper_finished->wait_notice();

    expect(wrapper_count.load() == 1);
    expect(function_count.load() == 1);
    expect(!inside_wrapper.load());

    expect(d->detach(object_id));
    pqrs::dispatcher::extra::terminate_shared_dispatcher();
  };

  "dispatcher.ignore_enqueue_after_terminate"_test = [] {
    std::cout << "dispatcher.ignore_enqueue_after_terminate" << std::endl;

    auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();

    {
      size_t count = 0;

      pqrs::dispatcher::dispatcher d(time_source);

      auto object_id = pqrs::dispatcher::make_new_object_id();
      expect(d.attach(object_id));

      d.terminate();

      auto result = d.enqueue(
          object_id,
          [&count] {
            ++count;
          });

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      expect(!result);
      expect(count == 0);
    }
  };

  "dispatcher.ignore_enqueue_after_detach"_test = [] {
    std::cout << "dispatcher.ignore_enqueue_after_detach" << std::endl;

    auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();

    {
      size_t count = 0;

      pqrs::dispatcher::dispatcher d(time_source);

      auto object_id = pqrs::dispatcher::make_new_object_id();
      expect(d.attach(object_id));

      expect(d.detach(object_id));

      auto result = d.enqueue(
          object_id,
          [&count] {
            ++count;
          });

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      expect(!result);
      expect(count == 0);

      d.terminate();
    }
  };

  "dispatcher.ignore_attach_after_terminate"_test = [] {
    std::cout << "dispatcher.ignore_attach_after_terminate" << std::endl;

    auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();

    {
      pqrs::dispatcher::dispatcher d(time_source);

      d.terminate();

      auto object_id = pqrs::dispatcher::make_new_object_id();
      auto result = d.attach(object_id);

      expect(!result);
      expect(!d.attached(object_id));
    }
  };
}
