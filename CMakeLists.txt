cmake_minimum_required(VERSION 3.20)
project(recas VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(recas_core
  src/client.cpp
  src/codec.cpp
  src/command_handler.cpp
  src/file_descriptor.cpp
  src/store.cpp
)
target_include_directories(recas_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_options(recas_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

add_executable(recas-server
  src/main.cpp
  src/server.cpp
)
target_link_libraries(recas-server PRIVATE recas_core)
target_compile_options(recas-server PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

include(CTest)
if(BUILD_TESTING)
  add_executable(recas-tests tests/tests.cpp)
  target_link_libraries(recas-tests PRIVATE recas_core)
  target_compile_options(recas-tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
  add_test(NAME codec-tests COMMAND recas-tests codec)
  add_test(NAME command-tests COMMAND recas-tests commands)
endif()
