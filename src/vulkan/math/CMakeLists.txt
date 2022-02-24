cmake_minimum_required(VERSION 3.0)
project(myproject)
 
find_package(Eigen3 3.3 REQUIRED NO_MODULE)
 
add_executable(example example.cpp)
target_link_libraries(example Eigen3::Eigen)
