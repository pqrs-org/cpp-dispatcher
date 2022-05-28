#include <boost/ut.hpp>
#include <iostream>
#include <pqrs/dispatcher.hpp>

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

void run_dispatcher_recursive_test(void) {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "dispatcher.recursive_detach"_test = [] {
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
  };

  "dispatcher.detach_recursive"_test = [] {
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
  };

  "dispatcher.recursive"_test = [] {
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

      expect(count == 5);
    }

    {
      size_t count = 0;

      {
        dispatcher_recursive_class dispatcher_recursive_class(count);

        dispatcher_recursive_class.enqueue();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }

      expect(count == 1);
    }
  };
}
