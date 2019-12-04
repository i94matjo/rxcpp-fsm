cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(rxcpp-fsm-doxygen LANGUAGES C CXX)

find_package(Doxygen)

if(DOXYGEN_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/doxygen.conf.in ${CMAKE_CURRENT_BINARY_DIR}/doxygen.conf)

    # Target to generate docs
    add_custom_target(RxCppFSM-doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxygen.conf
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/doxygen.conf
        COMMENT "Generating documentation with Doxygen"
        VERBATIM)
endif()
