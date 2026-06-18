#include <boost/ut.hpp>
#include <iostream>
#include <pqrs/dispatcher.hpp>
#include <utility>

void run_object_id_test() {
  using namespace boost::ut;
  using namespace boost::ut::literals;

  "object_id"_test = [] {
    std::cout << "object_id" << std::endl;

    auto active_object_id_count = pqrs::dispatcher::active_object_id_count();
    int id1 = 0;

    {
      pqrs::dispatcher::object_id object_id1(pqrs::dispatcher::make_new_object_id());
      pqrs::dispatcher::object_id object_id2(pqrs::dispatcher::make_new_object_id());

      id1 = object_id1.get();

      expect(object_id2.get() == id1 + 1);
      expect(pqrs::dispatcher::active_object_id_count() == active_object_id_count + 2);
    }

    expect(pqrs::dispatcher::active_object_id_count() == active_object_id_count);

    {
      pqrs::dispatcher::object_id object_id3(pqrs::dispatcher::make_new_object_id());
      pqrs::dispatcher::object_id object_id4(pqrs::dispatcher::make_new_object_id());

      expect(object_id3.get() == id1 + 2);
      expect(object_id4.get() == id1 + 3);
      expect(pqrs::dispatcher::active_object_id_count() == active_object_id_count + 2);
    }
  };

  "object_id.move"_test = [] {
    std::cout << "object_id.move" << std::endl;

    auto active_object_id_count = pqrs::dispatcher::active_object_id_count();

    {
      auto object_id1 = pqrs::dispatcher::make_new_object_id();
      auto value = object_id1.get();

      {
        pqrs::dispatcher::object_id object_id2(std::move(object_id1));

        expect(object_id2.get() == value);
        expect(pqrs::dispatcher::active_object_id_count() == active_object_id_count + 1);
      }

      expect(pqrs::dispatcher::active_object_id_count() == active_object_id_count);
    }

    expect(pqrs::dispatcher::active_object_id_count() == active_object_id_count);
  };
}
