include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9fd7188ec8e5a0974c03b8.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

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
  target_link_libraries(${test} gtest_main athread)
  gtest_discover_tests(${test})
  set_target_properties(${test} PROPERTIES FOLDER "Tests")
endforeach()

