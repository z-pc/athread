cmake_minimum_required(VERSION 3.20)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(BuildFunctions)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(UNIX)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

project(athread C CXX)
option(ATHREAD_TRACKING "Write activity of thread pool to console" ON)
option(BUILD_TEST "Build the test targets" OFF)
enable_testing()

if(ATHREAD_TRACKING)
    add_definitions(-DATHREAD_TRACKING)
endif()

add_library(athread src/athread/athread.cpp src/athread/threadpool.cpp src/athread/threadgraph.cpp 
src/athread/task.cpp src/athread/worker.cpp src/athread/executor.cpp)
target_include_directories(athread PUBLIC src)

list(APPEND ATHREAD_SAMPLES
  pool_simple
  pool_auto_shutdown
  graph_simple
  graph_pipeline
  graph_matrix_sum
  graph_fibonacci
  graph_calc_parallel
  graph_auto_shutdown
  graph_image_processing
  graph_document_processing
  graph_data_analysis
)

foreach(sample IN LISTS ATHREAD_SAMPLES)
  add_executable(${sample} samples/${sample}.cpp)
  target_link_libraries(${sample} athread)
  set_target_properties(${sample} PROPERTIES FOLDER "Samples")
endforeach()

if(BUILD_TEST)
  message(STATUS "Building tests...")    
  add_subdirectory(tests)
else()
  message(STATUS "Tests are disabled.")
endif()

target_install_lib(athread)
target_install_dir(src/athread)