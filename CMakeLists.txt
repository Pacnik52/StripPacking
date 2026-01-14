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
FetchContent_MakeAvailable(SFML Eigen)

# RectangleBinPack
FetchContent_Declare(
        RectangleBinPack
        GIT_REPOSITORY https://github.com/juj/RectangleBinPack.git
        GIT_TAG        master
)
FetchContent_GetProperties(RectangleBinPack)
if(NOT rectanglebinpack_POPULATED)
    FetchContent_Populate(RectangleBinPack)
endif()
set(RBP_SOURCES
        ${rectanglebinpack_SOURCE_DIR}/MaxRectsBinPack.cpp
        ${rectanglebinpack_SOURCE_DIR}/GuillotineBinPack.cpp
        ${rectanglebinpack_SOURCE_DIR}/SkylineBinPack.cpp
        ${rectanglebinpack_SOURCE_DIR}/Rect.cpp
)
# c-cmaes
FetchContent_Declare(
        c_cmaes
        GIT_REPOSITORY https://github.com/CMA-ES/c-cmaes.git
        GIT_TAG        master
)
FetchContent_GetProperties(c_cmaes)
if(NOT c_cmaes_POPULATED)
    FetchContent_Populate(c_cmaes)
endif()
set(CMAES_SOURCES
        ${c_cmaes_SOURCE_DIR}/src/cmaes.c
        StripBoard.cpp
        StripBoard.h
        FeatureExtractor.cpp
        FeatureExtractor.h
        NeuralStrategy.cpp
        NeuralStrategy.h
        Trainer.cpp
        Trainer.h
)

add_executable(strippacker
        main.cpp
        DataLoaderOdp.cpp
        DataLoaderOdp.h
        bin_drawer/BinDrawer.cpp
        bin_drawer/BinDrawer.h
        FeatureExtractor.cpp
        FeatureExtractor.h
        NeuralNetwork.cpp
        NeuralNetwork.h
        Trainer.cpp
        Trainer.h
        NeuralStrategy.cpp
        NeuralStrategy.h
        StripBoard.cpp
        StripBoard.h

        ${RBP_SOURCES}
        ${CMAES_SOURCES}
)

target_include_directories(strippacker PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${rectanglebinpack_SOURCE_DIR}
        ${c_cmaes_SOURCE_DIR}/src
)

target_link_libraries(strippacker
        PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        Eigen3::Eigen
)