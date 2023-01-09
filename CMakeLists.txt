cmake_minimum_required(VERSION 3.10)
project(9cc)

add_executable(9cc main.c parse.c codegen.c 9cc.h)

target_compile_options(9cc PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /utf-8 /Zc:__cplusplus /Zc:preprocessor>
)

target_compile_features(9cc PRIVATE c_std_11)
target_link_libraries(9cc -static)

# Enable the testing features.
enable_testing()

find_program (BASH_PROGRAM bash)
if (BASH_PROGRAM)
  add_test (mytest ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/test.sh)
endif (BASH_PROGRAM)
