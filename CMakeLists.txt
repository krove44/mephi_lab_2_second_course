cmake_minimum_required(VERSION 4.2)
project(mephi_lab_2_second_course)

set(CMAKE_CXX_STANDARD 23)

add_executable(mephi_lab_2_second_course
        main.cpp
        person.h
        histogram.h
        )

include(FetchContent)
FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

add_executable(tests
        test.cpp)
target_link_libraries(tests GTest::gtest_main)
target_compile_features(tests PRIVATE cxx_std_20)



