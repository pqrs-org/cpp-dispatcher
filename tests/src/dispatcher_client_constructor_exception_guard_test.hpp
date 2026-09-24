#include <boost/ut.hpp>
#include <pqrs/dispatcher.hpp>
#include <stdexcept>
#include <string_view>

namespace dispatcher_client_constructor_exception_guard_test {
class client final : public pqrs::dispatcher::extra::dispatcher_client {
private:
  pqrs::dispatcher::extra::dispatcher_client_constructor_exception_guard guard_{*this};

public:
  client(std::weak_ptr<pqrs::dispatcher::dispatcher> dispatcher,
         bool fail_member,
         bool fail_body,
         bool& cleaned_up,
         bool& cleanup_on_dispatcher_thread)
      : dispatcher_client(dispatcher),
        value_(make_value(fail_member)),
        timer_(*this) {
    guard_.initialize(
        [&] {
          if (fail_body) {
            throw std::runtime_error("body");
          }
        },
        [&] {
          cleaned_up = value_ == 42;
          cleanup_on_dispatcher_thread = dispatcher_thread();
        });
  }

  ~client() override {
    detach_from_dispatcher();
  }

private:
  static int make_value(bool fail) {
    if (fail) {
      throw std::runtime_error("member");
    }
    return 42;
  }

  int value_;
  pqrs::dispatcher::extra::timer timer_;
};
} // namespace dispatcher_client_constructor_exception_guard_test

void run_dispatcher_client_constructor_exception_guard_test() {
  using namespace boost::ut;
  using namespace pqrs::dispatcher::extra;

  "dispatcher_client_constructor_exception_guard.initialize"_test = [] {
    auto source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
    auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(source);
    dispatcher_client client(dispatcher);
    {
      dispatcher_client_constructor_exception_guard guard(client);
      guard.initialize();
    }
    expect(client.attached());

    bool initialized = false;
    {
      dispatcher_client_constructor_exception_guard guard(client);
      guard.initialize([&] { initialized = true; });
    }
    expect(initialized);
    expect(client.attached());
    client.detach_from_dispatcher();
  };

  "dispatcher_client_constructor_exception_guard.initialize_exception"_test = [] {
    auto source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
    auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(source);
    dispatcher_client client(dispatcher);
    dispatcher_client_constructor_exception_guard guard(client);
    expect(throws<std::runtime_error>([&] {
      guard.initialize([] { throw std::runtime_error("body"); });
    }));
    expect(!client.attached());
  };

  "dispatcher_client_constructor_exception_guard.constructor_unwinding"_test = [] {
    auto source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
    auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(source);
    for (int failure = 0; failure < 3; ++failure) {
      bool cleaned_up = false;
      bool cleanup_on_dispatcher_thread = false;
      bool caught = false;
      try {
        dispatcher_client_constructor_exception_guard_test::client client(
            dispatcher, failure == 1, failure == 2,
            cleaned_up, cleanup_on_dispatcher_thread);
        expect(client.attached());
      } catch (const std::runtime_error& e) {
        caught = true;
        expect(std::string_view(e.what()) == (failure == 1 ? "member" : "body"));
      }
      expect(caught == (failure != 0));
      expect(cleaned_up == (failure == 2));
      expect(cleanup_on_dispatcher_thread == (failure == 2));
    }
  };

  "dispatcher_client_constructor_exception_guard.default_member_initializer_exception"_test = [] {
    struct failing_client final : dispatcher_client {
      dispatcher_client_constructor_exception_guard guard{*this};

      // This throws before the constructor body can call guard.initialize().
      int value = []() -> int {
        throw std::runtime_error("default member initializer");
      }();

      failing_client(std::weak_ptr<pqrs::dispatcher::dispatcher> dispatcher,
                     bool& body_entered)
          : dispatcher_client(dispatcher) {
        body_entered = true;
        guard.initialize();
      }

      ~failing_client() override {
        detach_from_dispatcher();
      }
    };

    auto source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
    auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(source);
    bool body_entered = false;
    bool caught = false;
    try {
      failing_client client(dispatcher, body_entered);
    } catch (const std::runtime_error& e) {
      caught = true;
      expect(std::string_view(e.what()) == "default member initializer");
    }
    // Without the guard's detach, dispatcher_client's destructor would abort
    // during unwinding, before control could reach this point.
    expect(caught);
    expect(!body_entered);
  };

  "dispatcher_client_constructor_exception_guard.cleanup_copy_exception"_test = [] {
    struct cleanup {
      int& copies;
      int fail_on_copy;
      bool& called;

      cleanup(int& copies, int fail_on_copy, bool& called)
          : copies(copies), fail_on_copy(fail_on_copy), called(called) {
      }

      cleanup(const cleanup& other)
          : copies(other.copies), fail_on_copy(other.fail_on_copy), called(other.called) {
        if (++copies == fail_on_copy) {
          throw std::runtime_error("cleanup copy");
        }
      }

      void operator()() const noexcept {
        called = true;
      }
    };

    class failing_client final : public dispatcher_client {
    private:
      dispatcher_client_constructor_exception_guard guard_{*this};

    public:
      failing_client(std::weak_ptr<pqrs::dispatcher::dispatcher> dispatcher, cleanup& cleanup)
          : dispatcher_client(dispatcher), timer_(*this) {
        guard_.initialize(
            [] { throw std::runtime_error("initialization"); },
            cleanup);
      }

      ~failing_client() override {
        detach_from_dispatcher();
      }

    private:
      // Its destructor aborts unless the guard detaches before unwinding members.
      pqrs::dispatcher::extra::timer timer_;
    };

    auto source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();
    auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(source);
    // Exercise conversion to std::function and subsequent copies in detach,
    // including a copy after the client has already detached.
    for (int fail_on_copy = 1; fail_on_copy <= 3; ++fail_on_copy) {
      int copies = 0;
      bool called = false;
      bool caught = false;
      cleanup cleanup(copies, fail_on_copy, called);
      try {
        failing_client client(dispatcher, cleanup);
      } catch (const std::runtime_error& e) {
        caught = true;
        expect(std::string_view(e.what()) == "initialization");
      }
      expect(caught);
      expect(copies == fail_on_copy);
      expect(!called);
    }
  };
}
