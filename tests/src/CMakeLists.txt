cmake_minimum_required (VERSION 3.9)

set(CMAKE_CXX_STANDARD 14)

add_compile_options(-Wall)
add_compile_options(-Werror)
add_compile_options(-O2)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../../include)
include_directories(SYSTEM ${CMAKE_CURRENT_LIST_DIR}/../vendor/include)

project (test)

add_executable(
  test
  dispatcher_detach_test.cpp
  dispatcher_recursive_test.cpp
  dispatcher_test.cpp
  dispatcher_when_test.cpp
  hardware_time_source_test.cpp
  object_id_test.cpp
  test.cpp
  timer_test.cpp
)
