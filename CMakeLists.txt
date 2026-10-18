cmake_minimum_required(VERSION 3.14)
project(bulkren VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bulkren_lib src/renamer.cpp)
target_include_directories(bulkren_lib PUBLIC include)
if(MSVC)
  target_compile_options(bulkren_lib PRIVATE /W4)
else()
  target_compile_options(bulkren_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(bulkren src/main.cpp)
target_link_libraries(bulkren PRIVATE bulkren_lib)

enable_testing()
add_executable(bulkren_tests tests/test_bulkren.cpp)
target_link_libraries(bulkren_tests PRIVATE bulkren_lib)
add_test(NAME bulkren_tests COMMAND bulkren_tests)

install(TARGETS bulkren RUNTIME DESTINATION bin)
