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

if (DEFINED LLVM_CONFIG)
	# Use --libdir for compatibility with older LLVM(<4)
	execute_process(COMMAND ${LLVM_CONFIG} --libdir OUTPUT_VARIABLE
		CUSTOM_LLVM_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	find_package(LLVM REQUIRED PATHS "${CUSTOM_LLVM_LIBDIR}/cmake/llvm"
		NO_DEFAULT_PATH)
else()
	find_package(LLVM REQUIRED)
endif()

message(STATUS "Found LLVM version: ${LLVM_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

find_package(PkgConfig REQUIRED)
pkg_check_modules(CGRAPH REQUIRED libcgraph)
pkg_check_modules(GVC REQUIRED libgvc)

set(SHARED_SRCS BasicBlock.cpp
		Function.cpp
	        Instruction.cpp
		Module.cpp)

add_executable(op-counter counter.cpp ${SHARED_SRCS})
add_executable(graph graph.cpp ${SHARED_SRCS})
add_executable(dag-graph dag-graph.cpp ${SHARED_SRCS})

llvm_map_components_to_libnames(llvm_libs support core irreader)

# Link against LLVM libraries
target_link_libraries(op-counter ${llvm_libs})

foreach(T IN ITEMS graph dag-graph)
	target_link_libraries(${T} ${llvm_libs})

	target_link_libraries(${T} ${CGRAPH_LIBRARIES})
	target_include_directories(${T} PUBLIC ${CGRAPH_INCLUDE_DIRS})
	target_compile_options(${T} PUBLIC ${CGRAPH_CFLAGS_OTHER})

	target_link_libraries(${T} ${GVC_LIBRARIES})
	target_include_directories(${T} PUBLIC ${GVC_INCLUDE_DIRS})
	target_compile_options(${T} PUBLIC ${GVC_CFLAGS_OTHER})
endforeach()
