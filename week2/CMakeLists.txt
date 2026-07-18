cmake_minimum_required(VERSION 3.16)
project(rgbd_pointcloud_week2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(rgbd_pointcloud_cli src/rgbd_pointcloud.cpp)
target_link_libraries(rgbd_pointcloud_cli PRIVATE ${OpenCV_LIBS})

enable_testing()
include(FetchContent)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

add_executable(test_rgbd_pointcloud_cpp tests/test_rgbd_pointcloud.cpp)
target_link_libraries(test_rgbd_pointcloud_cpp PRIVATE ${OpenCV_LIBS} GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(
  test_rgbd_pointcloud_cpp
  DISCOVERY_TIMEOUT 60
  DISCOVERY_MODE PRE_TEST
)
