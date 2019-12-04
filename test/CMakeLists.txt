cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(rxcpp_fsm_test LANGUAGES CXX)

# define some folders

include(${RXCPP_DIR}/projects/CMake/shared.cmake)

# configure unit tests via CTest
enable_testing()

# define the sources of the self test
set(TEST_SOURCES
   pseudostate.cpp
   region.cpp
   state.cpp
   state_machine.cpp
   threads.cpp
)

add_executable(rxcpp_fsm_test test.cpp ${TEST_SOURCES})
add_executable(rxcpp::fsm::tests ALIAS rxcpp_fsm_test)
set_target_properties(
    rxcpp_fsm_test PROPERTIES
    EXCLUDE_FROM_DEFAULT_BUILD TRUE
    EXCLUDE_FROM_ALL TRUE
)

target_compile_options(rxcpp_fsm_test PUBLIC ${RX_COMPILE_OPTIONS})
target_compile_features(rxcpp_fsm_test PUBLIC ${RX_COMPILE_FEATURES})
target_include_directories(rxcpp_fsm_test
    PUBLIC ${RX_SRC_DIR} ${RX_CATCH_DIR}
)
target_link_libraries(rxcpp_fsm_test ${CMAKE_THREAD_LIBS_INIT} RxCppFSM)

foreach(ONE_TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(ONE_TEST_NAME "${ONE_TEST_SOURCE}" NAME)
    string( REPLACE ".cpp" "" ONE_TEST_NAME ${ONE_TEST_NAME})
    set(ONE_TEST_FULL_NAME "rxcpp_fsm_test_${ONE_TEST_NAME}")
    add_executable( ${ONE_TEST_FULL_NAME} ${ONE_TEST_SOURCE} )
    add_executable( rxcpp::fsm::${ONE_TEST_NAME} ALIAS ${ONE_TEST_FULL_NAME})
    target_compile_definitions(${ONE_TEST_FULL_NAME} PUBLIC "CATCH_CONFIG_MAIN")
    target_compile_options(${ONE_TEST_FULL_NAME} PUBLIC ${RX_COMPILE_OPTIONS})
    target_compile_features(${ONE_TEST_FULL_NAME} PUBLIC ${RX_COMPILE_FEATURES})
    target_include_directories(${ONE_TEST_FULL_NAME}
        PUBLIC ${RX_SRC_DIR} ${RX_CATCH_DIR} ../src
        )
    target_link_libraries(${ONE_TEST_FULL_NAME} ${CMAKE_THREAD_LIBS_INIT} RxCppFSM)

    add_test(NAME ${ONE_TEST_NAME} COMMAND ${ONE_TEST_FULL_NAME})
endforeach(ONE_TEST_SOURCE ${TEST_SOURCES})
