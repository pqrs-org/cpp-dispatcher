#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <pqrs/dispatcher.hpp>

TEST_CASE("object_id") {
  auto active_object_id_count = pqrs::dispatcher::active_object_id_count();
  int id1 = 0;

  {
    pqrs::dispatcher::object_id object_id1(pqrs::dispatcher::make_new_object_id());
    pqrs::dispatcher::object_id object_id2(pqrs::dispatcher::make_new_object_id());

    id1 = object_id1.get();

    REQUIRE(object_id2.get() == id1 + 1);
    REQUIRE(pqrs::dispatcher::active_object_id_count() == active_object_id_count + 2);
  }

  REQUIRE(pqrs::dispatcher::active_object_id_count() == active_object_id_count);

  {
    pqrs::dispatcher::object_id object_id3(pqrs::dispatcher::make_new_object_id());
    pqrs::dispatcher::object_id object_id4(pqrs::dispatcher::make_new_object_id());

    REQUIRE(object_id3.get() == id1 + 2);
    REQUIRE(object_id4.get() == id1 + 3);
    REQUIRE(pqrs::dispatcher::active_object_id_count() == active_object_id_count + 2);
  }
}

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

namespace {
class recursive_detach_test final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  recursive_detach_test(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher) : dispatcher_client(weak_dispatcher) {
  }

  virtual ~recursive_detach_test(void) {
    detach_from_dispatcher([] {
    });
  }

  void f(void) {
    enqueue_to_dispatcher([this] {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      detach_from_dispatcher([] {
      });
    });
  }
};
} // namespace

TEST_CASE("dispatcher.recursive_detach") {
  std::cout << "dispatcher.recursive_detach" << std::endl;

  auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();

  {
    auto d = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

    std::vector<std::unique_ptr<recursive_detach_test>> objects;
    for (int i = 0; i < 10; ++i) {
      objects.push_back(std::make_unique<recursive_detach_test>(d));
      objects.back()->f();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    objects.clear();

    d->terminate();
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

TEST_CASE("dispatcher.detach_recursive") {
  std::cout << "dispatcher.detach_recursive" << std::endl;

  auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();

  {
    pqrs::dispatcher::dispatcher d(time_source);

    auto object_id1 = pqrs::dispatcher::make_new_object_id();
    auto object_id2 = pqrs::dispatcher::make_new_object_id();
    d.attach(object_id1);
    d.attach(object_id2);

    d.detach(object_id1, [&] {
      d.detach(object_id2);
    });

    d.terminate();
  }

  // Call `detach` in the queued function.

  {
    pqrs::dispatcher::dispatcher d(time_source);

    auto object_id1 = pqrs::dispatcher::make_new_object_id();
    auto object_id2 = pqrs::dispatcher::make_new_object_id();
    auto object_id3 = pqrs::dispatcher::make_new_object_id();
    d.attach(object_id1);
    d.attach(object_id2);
    d.attach(object_id3);

    d.enqueue(
        object_id3,
        [&] {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));

          d.detach(object_id2);
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    d.detach(object_id1);

    d.terminate();
  }
}

namespace {
void dispatcher_recursive_function(pqrs::dispatcher::dispatcher& d,
                                   const pqrs::dispatcher::object_id& id,
                                   size_t& count) {
  ++count;
  if (count < 5) {
    d.enqueue(
        id,
        [&d, &id, &count] {
          dispatcher_recursive_function(d, id, count);
        });
  } else if (count == 5) {
    d.enqueue(
        id,
        [] {
          std::cout << "dispatcher_recursive_function finished" << std::endl;
        });
  }
}

class dispatcher_recursive_class final {
public:
  dispatcher_recursive_class(size_t& count) : count_(count),
                                              object_id_(pqrs::dispatcher::make_new_object_id()) {
    time_source_ = std::make_shared<pqrs::dispatcher::pseudo_time_source>();

    dispatcher_ = std::make_unique<pqrs::dispatcher::dispatcher>(time_source_);

    dispatcher_->attach(object_id_);
  }

  ~dispatcher_recursive_class(void) {
    dispatcher_->terminate();
    dispatcher_ = nullptr;
  }

  void enqueue(void) {
    dispatcher_->enqueue(
        object_id_,
        [this] {
          dispatcher_->enqueue(
              object_id_,
              [this] {
                ++count_;
                std::cout << "dispatcher_recursive_class finished" << std::endl;
              });
        });
  }

private:
  size_t& count_;

  std::shared_ptr<pqrs::dispatcher::pseudo_time_source> time_source_;
  std::unique_ptr<pqrs::dispatcher::dispatcher> dispatcher_;
  pqrs::dispatcher::object_id object_id_;
};
} // namespace

TEST_CASE("dispatcher.recursive") {
  std::cout << "dispatcher.recursive" << std::endl;

  auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();

  // Call `enqueue` in dispatcher's thread.

  {
    size_t count = 0;

    {
      pqrs::dispatcher::dispatcher d(time_source);

      auto object_id = pqrs::dispatcher::make_new_object_id();
      d.attach(object_id);

      d.enqueue(
          object_id,
          [&] {
            dispatcher_recursive_function(d, object_id, count);
          });

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      d.terminate();
    }

    REQUIRE(count == 5);
  }

  {
    size_t count = 0;

    {
      dispatcher_recursive_class dispatcher_recursive_class(count);

      dispatcher_recursive_class.enqueue();

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    REQUIRE(count == 1);
  }
}
