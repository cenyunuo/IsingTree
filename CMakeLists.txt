cmake_minimum_required(VERSION 3.10)
project(IsingTree)

# Find Python3 with Interpreter and Development components
find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

# Find Pybind11
find_package(pybind11 REQUIRED)

# Add the library
add_library(IsingTree MODULE IsingTree.cpp bindings.cpp)

# Include directories for Python and Pybind11
target_include_directories(IsingTree PRIVATE 
    ${Python3_INCLUDE_DIRS}       # Include Python headers
    ${pybind11_INCLUDE_DIRS}      # Include Pybind11 headers
)

# Link Python libraries
target_link_libraries(IsingTree PRIVATE ${Python3_LIBRARIES})

# Set output properties
set_target_properties(IsingTree PROPERTIES PREFIX "" SUFFIX ".so")
