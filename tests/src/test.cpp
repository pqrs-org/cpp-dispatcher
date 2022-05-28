#include "dispatcher_detach_test.hpp"
#include "dispatcher_recursive_test.hpp"
#include "dispatcher_test.hpp"
#include "dispatcher_when_test.hpp"
#include "hardware_time_source_test.hpp"
#include "object_id_test.hpp"
#include "timer_test.hpp"

int main(void) {
  run_dispatcher_detach_test();
  run_dispatcher_recursive_test();
  run_dispatcher_test();
  run_dispatcher_when_test();
  run_hardware_time_source_test();
  run_object_id_test();
  run_timer_test();
  return 0;
}
