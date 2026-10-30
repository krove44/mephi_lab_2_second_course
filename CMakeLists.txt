cmake_minimum_required(VERSION 4.0)
project(mephi_lab1)

set(CMAKE_CXX_STANDARD 23)

add_executable(mephi_lab1
        main.cpp
        avl_tree.h)

include(FetchContent)
FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

add_executable(avl_tests
        tests/avl_tree_tests.cpp)
target_link_libraries(avl_tests GTest::gtest_main)
target_compile_features(avl_tests PRIVATE cxx_std_20)

FetchContent_Declare(ftxui
        GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
        GIT_TAG main
)
FetchContent_MakeAvailable(ftxui)
add_executable(ui
        ui.cpp
)

target_link_libraries(ui
        PRIVATE ftxui::screen
        PRIVATE ftxui::dom
        PRIVATE ftxui::component
)
