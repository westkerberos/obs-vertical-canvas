cmake_minimum_required(VERSION 3.20)

project(west-tiktok-relay VERSION 0.1.0 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(west-tiktok-relay
  src/main.cpp
)

target_compile_features(west-tiktok-relay PRIVATE cxx_std_20)
target_link_libraries(west-tiktok-relay PRIVATE Qt6::Core)

if(MSVC)
  target_compile_options(west-tiktok-relay PRIVATE /W4 /permissive-)
else()
  target_compile_options(west-tiktok-relay PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
  add_test(
    NAME west-tiktok-relay-control-protocol
    COMMAND ${CMAKE_COMMAND}
      -DRELAY_EXECUTABLE=$<TARGET_FILE:west-tiktok-relay>
      -DINPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/tests/protocol-input.jsonl
      -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/protocol-smoke.cmake
  )
endif()
