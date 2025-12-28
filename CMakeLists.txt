cmake_minimum_required(VERSION 3.20)
project(dashcam_perception LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(DCP_BUILD_TESTS "Build tests" ON)
option(DCP_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

# Dependencies
find_package(yaml-cpp REQUIRED)
find_package(Threads REQUIRED)

# OpenCV
find_package(OpenCV REQUIRED)

# Core library
add_library(dashcam_core
  src/core/config_loader.cpp
  src/infra/thread_runner.cpp
  # Add more .cpp files once created
  # ...
)

target_include_directories(dashcam_core
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(dashcam_core
  PUBLIC
    yaml-cpp::yaml-cpp
    Threads::Threads
    ${OpenCV_LIBS}
)

# Warnings
if (MSVC)
  target_compile_options(dashcam_core PRIVATE /W4)
else()
  target_compile_options(dashcam_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (DCP_WARNINGS_AS_ERRORS)
  if (MSVC)
    target_compile_options(dashcam_core PRIVATE /WX)
  else()
    target_compile_options(dashcam_core PRIVATE -Werror)
  endif()
endif()

# Apps
add_executable(live_pipeline apps/live_pipeline.cpp)
target_link_libraries(live_pipeline PRIVATE dashcam_core)

add_executable(live_viewer apps/live_viewer.cpp)
target_link_libraries(live_viewer PRIVATE dashcam_core)

add_executable(offline_replay apps/offline_replay.cpp)
target_link_libraries(offline_replay PRIVATE dashcam_core)

add_executable(thread_runner_test apps/thread_runner_test.cpp)
target_link_libraries(thread_runner_test PRIVATE dashcam_core)

add_executable(bounded_queue_test apps/bounded_queue_test.cpp)
target_link_libraries(bounded_queue_test PRIVATE dashcam_core)

# Tests
if (DCP_BUILD_TESTS)
  enable_testing()
  # Later on, add Catch2 or GoogleTest if using them
  # add_executable(test_bounded_queue tests/test_bounded_queue.cpp)
  # target_link_libraries(test_bounded_queue PRIVATE dashcam_core)
  # add_test(NAME test_bounded_queue COMMAND test_bounded_queue)
endif()
