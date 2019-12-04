cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(rxcpp-fsm VERSION 0.0.1 LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(RXCPP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rxcpp)
include(${RXCPP_DIR}/projects/CMake/shared.cmake)

option(RXCPP_FSM_BUILD_TESTS "Build rxcpp-fms unit tests" ON)
option(RXCPP_FSM_BUILD_DOC "Build rxcpp-fms documentation" ON)
option(RXCPP_FSM_BUILD_EXAMPLES "Build rxcpp-fms examples" ON)

add_subdirectory(rxcpp)
if(RXCPP_FSM_BUILD_DOC)
   add_subdirectory(doc)
endif()
if(RXCPP_FSM_BUILD_TESTS)
   add_subdirectory(test)
endif()
if(RXCPP_FSM_BUILD_EXAMPLES)
   add_subdirectory(examples)
endif()

set(FSM_SOURCES
   include/rxcpp/rx-fsm.hpp
   include/rxcpp/fsm/rx-fsm-delegates.hpp
   include/rxcpp/fsm/rx-fsm-includes.hpp
   include/rxcpp/fsm/rx-fsm-predef.hpp
   include/rxcpp/fsm/rx-fsm-pseudostate.hpp
   include/rxcpp/fsm/rx-fsm-region.hpp
   include/rxcpp/fsm/rx-fsm-state.hpp
   include/rxcpp/fsm/rx-fsm-state_machine.hpp
   include/rxcpp/fsm/rx-fsm-transition.hpp
   include/rxcpp/fsm/rx-fsm-vertex.hpp
   src/rxcpp/fsm/rx-fsm-delegates.cpp
   src/rxcpp/fsm/rx-fsm-predef.cpp
   src/rxcpp/fsm/rx-fsm-pseudostate.cpp
   src/rxcpp/fsm/rx-fsm-region.cpp
   src/rxcpp/fsm/rx-fsm-state.cpp
   src/rxcpp/fsm/rx-fsm-state_machine.cpp
   src/rxcpp/fsm/rx-fsm-transition.cpp
)

source_group("fsm" FILES ${FSM_SOURCES})

add_library(RxCppFSM SHARED ${FSM_SOURCES})
target_include_directories(RxCppFSM PUBLIC include ${RX_SRC_DIR})
target_link_libraries(RxCppFSM RxCpp)
set_target_properties(RxCppFSM PROPERTIES LINKER_LANGUAGE CXX)
