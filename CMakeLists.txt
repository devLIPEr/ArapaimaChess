cmake_minimum_required(VERSION 3.10)
project(Arapaima VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Enable all warnings
add_compile_options(-W -Wall)

# Define preprocessor macro
# add_definitions(-DUSE_INTEL_PEXT)

# Enable OpenMP
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# Add BMI2 instruction flag
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mbmi2")

# Add pthreads instruction flag
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# AVX
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")

# Add -O3 optimization
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# Engine source
set(ENGINE_SOURCE
    src/magics/fixed.cpp
    src/magics/pext.cpp
    src/prng/cgw64.cpp
    src/board.cpp
    src/book.cpp
    src/engine.cpp
    src/entry.cpp
    src/evaluate.cpp
    src/main.cpp
    src/move_generator.cpp
    src/search.cpp
    src/transposition_table.cpp
    src/types.cpp
    src/uci.cpp
    src/utils.cpp
    src/zobrist.cpp
)

# Fathom source
set(FATHOM_SOURCE
    src/syzygy/tbprobe.c
)

# Add source files
set(SOURCES
    ${ENGINE_SOURCE}
    ${FATHOM_SOURCE}
)

# Add the executable
add_executable(arapaima ${SOURCES})

# Link with atomic library
target_link_libraries(arapaima atomic)
