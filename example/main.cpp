#include <iostream>
#include <pqrs/dispatcher.hpp>
#include <vector>

class object1 final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  object1(int index) : dispatcher_client(),
                       index_(index) {
  }

  virtual ~object1(void) {
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

  pqrs::dispatcher::extra::initialize_shared_dispatcher();

  for (int i = 0; i < 20; ++i) {
    objects.emplace_back(std::make_unique<object1>(i));
    objects.back()->hello();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  objects.clear();

  pqrs::dispatcher::extra::terminate_shared_dispatcher();
}
