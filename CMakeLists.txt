cmake_minimum_required( VERSION 3.9.2 )

project( inst-counter LANGUAGES CXX )
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include( GNUInstallDirs )
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif(NOT CMAKE_BUILD_TYPE)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

message(STATUS "Found LLVM version: ${LLVM_VERSION}")

find_package(PkgConfig REQUIRED)
pkg_check_modules(CGRAPH REQUIRED libcgraph)
pkg_check_modules(GVC REQUIRED libgvc)

set(SHARED_SRCS Function.cpp
	        Instruction.cpp
		Module.cpp)

add_executable(op-counter counter.cpp ${SHARED_SRCS})
add_executable(graph graph.cpp ${SHARED_SRCS})

llvm_map_components_to_libnames(llvm_libs support core irreader)

# Link against LLVM libraries
target_link_libraries(op-counter ${llvm_libs})
target_link_libraries(graph ${llvm_libs})
target_link_libraries(graph ${CGRAPH_LIBRARIES})
target_include_directories(graph PUBLIC ${CGRAPH_INCLUDE_DIRS})
target_compile_options(graph PUBLIC ${CGRAPH_CFLAGS_OTHER})
target_link_libraries(graph ${GVC_LIBRARIES})
target_include_directories(graph PUBLIC ${GVC_INCLUDE_DIRS})
target_compile_options(graph PUBLIC ${GVC_CFLAGS_OTHER})
