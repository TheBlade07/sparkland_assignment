cmake_minimum_required(VERSION 3.14)
project(sparkland_assignment LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

include(FetchContent)

FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG  tags/v3.13.0
  GIT_SHALLOW TRUE)

# Fetch Google Test
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
  GIT_SHALLOW TRUE)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(simdjson googletest)

add_library(sparkland_lib
    src/coinbase_client.cpp
    src/tick_parser.cpp
    src/csv_logger.cpp
)

# Include directories
target_include_directories(sparkland_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/external/websocketpp
)

# Link libraries
target_link_libraries(sparkland_lib
    PUBLIC
        Threads::Threads
        OpenSSL::SSL
        OpenSSL::Crypto
        simdjson
)

add_executable(sparkland_app
    src/main.cpp
)

# Link executable to your library
target_link_libraries(sparkland_app
    PRIVATE
        sparkland_lib
)

# Enable testing
enable_testing()

# Add test executable
add_executable(sparkland_tests
    tests/test_ema.cpp
)

target_link_libraries(sparkland_tests
    PRIVATE
        sparkland_lib
        gtest_main
        gtest
)

# Include test directories
target_include_directories(sparkland_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/external/websocketpp
)

include(GoogleTest)
gtest_discover_tests(sparkland_tests)