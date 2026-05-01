cmake_minimum_required(VERSION 3.0...3.30)
project(myproject)
 
# First try Eigen [5.0, 6.0).
find_package(Eigen3 5.0 QUIET NO_MODULE)

# If not found, accept Eigen [3.3, 4.0). There is no Eigen3 version 4.x.
if (NOT Eigen3_FOUND)
  find_package(Eigen3 3.3 REQUIRED NO_MODULE)
endif()
 
add_executable(example example.cpp)
target_link_libraries(example Eigen3::Eigen)
