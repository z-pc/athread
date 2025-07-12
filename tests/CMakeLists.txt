find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

list(APPEND ATHREAD_TEST
  test_graph_operators
  test_graph_calc_parallel
  test_task_iterator
  test_graph_wait
  test_catch_exception
)

include(GoogleTest)

foreach(test IN LISTS ATHREAD_TEST)
  add_executable(${test} ${test}.cpp)
  target_link_libraries(${test} ${GTEST_LIBRARIES} GTest::gtest GTest::gtest_main athread)
  target_include_directories(${test} PUBLIC tests)
  gtest_discover_tests(${test})
  set_target_properties(${test} PROPERTIES FOLDER "Tests")
endforeach()

