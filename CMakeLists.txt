cmake_minimum_required( VERSION 3.9.2 )

project( llvm-tools LANGUAGES CXX )
set( CXX_STANDARD_REQUIRED ON )
include( GNUInstallDirs )
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif(NOT CMAKE_BUILD_TYPE)

if (DEFINED LLVM_CONFIG)
	execute_process(COMMAND ${LLVM_CONFIG} --cmakedir OUTPUT_VARIABLE
		CUSTOM_LLVM_CMAKEDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	find_package(LLVM REQUIRED PATHS "${CUSTOM_LLVM_CMAKEDIR}" NO_DEFAULT_PATH)
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
pkg_check_modules(GINAC REQUIRED ginac)

set(SHARED_SRCS BasicBlock.cpp
		Function.cpp
	        Instruction.cpp
		Module.cpp)

set(CPP_SHARED trace_gep.cpp)

add_executable(op-counter counter.cpp ${SHARED_SRCS})
add_executable(graph graph.cpp ${SHARED_SRCS})
add_executable(dag-graph dag-graph.cpp ${SHARED_SRCS})
add_executable(param-restrict param-restrict.cpp ${CPP_SHARED})
add_executable(result-range result-range.cpp ${CPP_SHARED})
add_executable(result-symbolic result-symbolic.cpp ${CPP_SHARED})
add_executable(clone-checker clone-checker.cpp ${CPP_SHARED})

# Work around an LLVM cmake bug that checks DYLIB_COMPONENTS instead of
# LINK_LLVM_DYLIB
if(DEFINED LLVM_LINK_LLVM_DYLIB)
	set(USE_SHARED "USE_SHARED")
else()
	set(USE_SHARED "")
endif()

# Link with LLVM libraries
foreach(T IN ITEMS dag-graph graph op-counter param-restrict result-range
		result-symbolic clone-checker)
	llvm_config(${T} ${USE_SHARED} support core irreader analysis passes)
	set_property(TARGET ${T} PROPERTY CXX_STANDARD 17)
endforeach()

# Link graph tools with cgraph
foreach(T IN ITEMS graph dag-graph)
	target_link_libraries(${T} PRIVATE ${CGRAPH_LIBRARIES})
	target_include_directories(${T} PRIVATE ${CGRAPH_INCLUDE_DIRS})
	target_compile_options(${T} PRIVATE ${CGRAPH_CFLAGS_OTHER})

	target_link_libraries(${T} PRIVATE ${GVC_LIBRARIES})
	target_include_directories(${T} PRIVATE ${GVC_INCLUDE_DIRS})
	target_compile_options(${T} PRIVATE ${GVC_CFLAGS_OTHER})
endforeach()

target_link_libraries(result-symbolic PRIVATE ${GINAC_LIBRARIES})
target_include_directories(result-symbolic PRIVATE ${GINAC_INCLUDE_DIRS})
target_compile_options(result-symbolic PRIVATE ${GINAC_CFLAGS_OTHER})

configure_file(tests/lit.site.cfg.py.in tests/lit.site.cfg.py @ONLY)
configure_file(extract_model_code.sh.in extract_model_code.sh @ONLY)
