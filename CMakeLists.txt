cmake_minimum_required(VERSION 3.14)

project(StripPacking)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# SFML
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        3.0.2
)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Eingen
FetchContent_Declare(
        Eigen
        URL      https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.zip
)

FetchContent_Declare(
        Boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.tar.xz
)

# Boost is huge; we only want to build what we need
set(BOOST_INCLUDE_LIBRARIES serialization)
set(BOOST_ENABLE_CMAKE ON)

FetchContent_MakeAvailable(SFML Eigen Boost)


add_executable(strippacker
        main.cpp
        bin_reader/DataLoaderOdp.cpp
        bin_reader/DataLoaderOdp.h
        bin_drawer/BinDrawer.cpp
        bin_drawer/BinDrawer.h
        startegy/FFN.h
        bin/CornerPoints.cpp
        bin/CornerPoints.h
        startegy/EvolutionaryAlgorithm.h
        startegy/BinpackConstructionHeuristic.h
        bin/BinpackData.h
)

target_include_directories(strippacker PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(strippacker
        PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        Eigen3::Eigen
        Boost::serialization
)