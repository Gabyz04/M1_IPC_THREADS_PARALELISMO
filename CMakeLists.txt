cmake_minimum_required(VERSION 3.10)
project(img_parallel_fifo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} include)

add_executable(img_sender src/sender.cpp)
target_link_libraries(img_sender ${OpenCV_LIBS} Threads::Threads)

add_executable(img_worker src/worker.cpp)
target_link_libraries(img_worker ${OpenCV_LIBS} Threads::Threads)
