cmake_minimum_required( VERSION 3.9.2 )

project( inst-counter LANGUAGES CXX )
include( GNUInstallDirs )

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(op-counter
               main.cpp)

llvm_map_components_to_libnames(llvm_libs support core irreader)

# Link against LLVM libraries
target_link_libraries(op-counter ${llvm_libs})
