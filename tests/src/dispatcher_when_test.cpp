#include <catch2/catch.hpp>

#include <iostream>
#include <pqrs/dispatcher.hpp>

TEST_CASE("dispatcher.when") {
  std::cout << "dispatcher.when" << std::endl;

  auto time_source = std::make_shared<pqrs::dispatcher::pseudo_time_source>();

  {
    std::string string;

    {
      pqrs::dispatcher::dispatcher d(time_source);

      auto object_id = pqrs::dispatcher::make_new_object_id();
      d.attach(object_id);

      time_source->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(0)));

      d.enqueue(
          object_id,
          [&string] {
            string += "a";
          });

      d.enqueue(
          object_id,
          [&string] {
            string += "b";
          });

      auto wait_c = pqrs::make_thread_wait();
      d.enqueue(
          object_id,
          [&string, wait_c] {
            string += "c";
            wait_c->notify();
          },
          pqrs::dispatcher::time_point(std::chrono::milliseconds(500)));

      auto wait_d = pqrs::make_thread_wait();
      d.enqueue(
          object_id,
          [&string, wait_d] {
            string += "d";
            wait_d->notify();
          });

      wait_d->wait_notice();
      REQUIRE(string == "abd");

      // Enqueue new entry while dispatcher is waiting.

      d.enqueue(
          object_id,
          [&string] {
            string += "e";
          },
          pqrs::dispatcher::time_point(std::chrono::milliseconds(100)));

      auto wait_f = pqrs::make_thread_wait();
      d.enqueue(
          object_id,
          [&string, wait_f] {
            string += "f";
            wait_f->notify();
          },
          pqrs::dispatcher::time_point(std::chrono::milliseconds(200)));

      auto wait_g = pqrs::make_thread_wait();
      d.enqueue(
          object_id,
          [&string, wait_g] {
            string += "g";
            wait_g->notify();
          },
          pqrs::dispatcher::time_point(std::chrono::milliseconds(600)));

      time_source->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(200)));
      d.invoke();

      wait_f->wait_notice();
      REQUIRE(string == "abdef");

      time_source->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(499)));
      d.invoke();

      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      REQUIRE(string == "abdef");

      time_source->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(500)));
      d.invoke();

      wait_c->wait_notice();
      REQUIRE(string == "abdefc");

      time_source->set_now(pqrs::dispatcher::time_point(std::chrono::milliseconds(600)));
      d.invoke();

      wait_g->wait_notice();
      REQUIRE(string == "abdefcg");

      d.terminate();
    }
  }
}
