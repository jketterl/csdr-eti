cmake_minimum_required(VERSION 3.0...3.27.7)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_BUILD_TYPE STREQUAL None)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

project (csdr-eti VERSION 0.1.0)
add_definitions(-DVERSION="${PROJECT_VERSION}-dev")

enable_language(CXX)
set(CMAKE_CXX_STANDARD 11)

include(GNUInstallDirs)

find_package(Csdr REQUIRED)

include(FindPkgConfig)
pkg_check_modules(FFTW3 REQUIRED fftw3f)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(src)
