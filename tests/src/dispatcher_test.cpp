#include <catch2/catch.hpp>

#include <iostream>
#include <pqrs/dispatcher.hpp>

TEST_CASE("dispatcher") {
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

    REQUIRE(d.dispatcher_thread() == false);

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

            REQUIRE(d.dispatcher_thread() == true);
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
    REQUIRE(elapsed > std::chrono::milliseconds(400));

    d.detach(object_id);
    d.terminate();
  }

  std::cout << std::endl;
}

TEST_CASE("dispatcher.preserve_the_order_of_entries") {
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

    REQUIRE(text == "abc");
  }
}

TEST_CASE("dispatcher.ignore_enqueue_after_terminate") {
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

    REQUIRE(count == 0);
  }
}
