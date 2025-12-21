cmake_minimum_required(VERSION 3.14)

project(StripPacking)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------
# AUTOMATICALLY DOWNLOAD AND BUILD SFML (Static Linking)
# ------------------------------------------------------------
include(FetchContent)

# We ask for SFML 3.0.2
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        3.0.2
)

# Force Static Linking (No DLLs needed!)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) # <--- This is the magic key

FetchContent_MakeAvailable(SFML)
# ------------------------------------------------------------

add_executable(strippacker
        main.cpp
        StripPacker.cpp
        placement_strategies/BottomLeftStrategy.cpp
        placement_strategies/CornerPoints.cpp
        DataLoaderOdp.cpp
        bin_drawer/BinDrawer.cpp
        placement_strategies/MaxRectsStrategy.cpp
        placement_strategies/MaxRectsStrategy.h
)

target_include_directories(strippacker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link against the SFML targets we just built
target_link_libraries(strippacker PRIVATE SFML::Graphics SFML::Window SFML::System)