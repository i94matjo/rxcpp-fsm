cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(rxcpp_fsm_examples LANGUAGES CXX)

include(${RXCPP_DIR}/projects/CMake/shared.cmake)

add_executable(dining_philosophers dpp.cpp)
target_compile_options(dining_philosophers PUBLIC ${RX_COMPILE_OPTIONS})
target_compile_features(dining_philosophers PUBLIC ${RX_COMPILE_FEATURES})
target_include_directories(dining_philosophers
    PUBLIC ${RX_SRC_DIR}
)
target_link_libraries(dining_philosophers ${CMAKE_THREAD_LIBS_INIT} RxCppFSM)
