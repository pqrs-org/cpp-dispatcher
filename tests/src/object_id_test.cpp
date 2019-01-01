#include <catch2/catch.hpp>

#include <iostream>
#include <pqrs/dispatcher.hpp>

TEST_CASE("object_id") {
  std::cout << "object_id" << std::endl;

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
