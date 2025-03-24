cmake_minimum_required(VERSION 3.7.2)

project(txl)
#set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -fsanitize=address -static-libasan")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

enable_testing()

add_subdirectory(src)
add_subdirectory(tests)
