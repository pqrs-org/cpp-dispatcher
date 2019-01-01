#include <catch2/catch.hpp>

#include <iostream>
#include <pqrs/dispatcher.hpp>

TEST_CASE("dispatcher.detach") {
  std::cout << "dispatcher.detach" << std::endl;

  auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();

  {
    size_t count = 0;

    pqrs::dispatcher::dispatcher d(time_source);

    auto object_id = pqrs::dispatcher::make_new_object_id();
    d.attach(object_id);

    d.enqueue(
        object_id,
        [&] {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        });

    d.enqueue(
        object_id,
        [&] {
          ++count;
        });

    // `detach` cancel queued functions.

    d.detach(object_id);

    REQUIRE(count == 0);

    d.terminate();
  }
}

TEST_CASE("dispatcher.detach_with_function") {
  std::cout << "dispatcher.detach_with_function" << std::endl;

  auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();

  {
    size_t count = 0;

    pqrs::dispatcher::dispatcher d(time_source);

    auto object_id = pqrs::dispatcher::make_new_object_id();
    d.attach(object_id);

    REQUIRE(count == 0);

    d.detach(
        object_id,
        [&] {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          ++count;
        });

    REQUIRE(count == 1);

    d.terminate();
  }

  // Call `detach` in the dispatcher thread.

  {
    size_t count = 0;

    pqrs::dispatcher::dispatcher d(time_source);

    auto object_id = pqrs::dispatcher::make_new_object_id();
    d.attach(object_id);

    d.enqueue(
        object_id,
        [&] {
          d.detach(
              object_id,
              [&] {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                ++count;
              });
        });

    REQUIRE(count == 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(count == 1);

    d.terminate();
  }

  // Call `detach` frequently

  {
    pqrs::dispatcher::dispatcher d(time_source);

    for (int i = 0; i < 1000; ++i) {
      if (i % 10 == 0) {
        std::cout << "." << std::flush;
      }

      auto object_id = pqrs::dispatcher::make_new_object_id();
      d.attach(object_id);

      d.detach(
          object_id,
          [&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
          });
    }

    std::cout << std::endl;

    d.terminate();
  }
}

TEST_CASE("dispatcher.wait_current_running_function_in_detach") {
  std::cout << "dispatcher.wait_current_running_function_in_detach" << std::endl;

  auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();

  {
    size_t count = 0;

    pqrs::dispatcher::dispatcher d(time_source);

    auto object_id = pqrs::dispatcher::make_new_object_id();
    d.attach(object_id);

    d.enqueue(
        object_id,
        [&count] {
          std::this_thread::sleep_for(std::chrono::milliseconds(200));
          ++count;
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    d.detach(object_id);

    REQUIRE(count == 1);

    d.terminate();
  }

  // Call `detach` in the dispatcher thread.

  {
    pqrs::dispatcher::dispatcher d(time_source);

    auto object_id = pqrs::dispatcher::make_new_object_id();
    d.attach(object_id);

    d.enqueue(
        object_id,
        [&] {
          d.detach(object_id);
        });

    d.terminate();
  }
}
