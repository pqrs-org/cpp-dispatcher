#include "dispatcher.hpp"
#include <iostream>
#include <vector>

class object1 final : public pqrs::dispatcher::dispatcher_client {
public:
  object1(std::weak_ptr<pqrs::dispatcher::dispatcher> weak_dispatcher,
          int index) : dispatcher_client(weak_dispatcher),
                       index_(index) {
  }

  ~object1(void) {
    detach_from_dispatcher([this] {
      std::cout << "detached " << index_ << std::endl;
    });
  }

  void hello(void) {
    enqueue_to_dispatcher([this] {
      std::cout << "hello " << index_ << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
  }

private:
  int index_;
};

int main(void) {
  std::vector<std::unique_ptr<object1>> objects;

  auto time_source = std::make_shared<pqrs::dispatcher::hardware_time_source>();
  auto dispatcher = std::make_shared<pqrs::dispatcher::dispatcher>(time_source);

  for (int i = 0; i < 20; ++i) {
    objects.emplace_back(std::make_unique<object1>(dispatcher, i));
    objects.back()->hello();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  objects.clear();

  dispatcher->terminate();
  dispatcher = nullptr;
}
