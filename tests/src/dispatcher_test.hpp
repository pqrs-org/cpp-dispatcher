#include <boost/ut.hpp>
#include <iostream>
#include <pqrs/dispatcher.hpp>

void run_dispatcher_test(void) {
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
      d.attach(object_id);

      expect(d.dispatcher_thread() == false);

      auto start = std::chrono::steady_clock::now();

      for (int i = 0; i < 10000; ++i) {
        d.enqueue(
            object_id,
            [&d, &count, i] {
              ++count;
              if (i % 1000 == 0) {
                std::cout << "." << std::flush;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
              }

              expect(d.dispatcher_thread() == true);
            });
      }

      auto wait = pqrs::make_thread_wait();
      d.enqueue(
          object_id,
          [wait] {
            wait->notify();
          });
      wait->wait_notice();

      auto elapsed = std::chrono::steady_clock::now() - start;
      expect(elapsed > std::chrono::milliseconds(400));

      d.detach(object_id);
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
      d.attach(object_id);

      d.enqueue(
          object_id,
          [&] {
            text += "a";
          });
      d.enqueue(
          object_id,
          [&] {
            text += "b";
          });
      d.enqueue(
          object_id,
          [&] {
            text += "c";
          });

      auto wait = pqrs::make_thread_wait();
      d.enqueue(
          object_id,
          [wait] {
            wait->notify();
          });
      wait->wait_notice();

      d.detach(object_id);
      d.terminate();

      expect(text == "abc");
    }
  };

  "dispatcher.ignore_enqueue_after_terminate"_test = [] {
    std::cout << "dispatcher.ignore_enqueue_after_terminate" << std::endl;

    auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();

    {
      size_t count = 0;

      pqrs::dispatcher::dispatcher d(time_source);

      auto object_id = pqrs::dispatcher::make_new_object_id();
      d.attach(object_id);

      d.terminate();

      d.enqueue(
          object_id,
          [&count] {
            ++count;
          });

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      expect(count == 0);
    }
  };
}
