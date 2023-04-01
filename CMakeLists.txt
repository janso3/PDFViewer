cmake_minimum_required(VERSION 3.16)
project(pdfviewer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
# See slide 100 of the following ppt :^)
# https://crascit.com/wp-content/uploads/2019/09/Deep-CMake-For-Library-Authors-Craig-Scott-CppCon-2019.pdf
if (NOT APPLE)
    set(CMAKE_INSTALL_RPATH $ORIGIN)
endif()
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

include (FetchContent)
include (FetchLagom.cmake)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

set(SOURCES
    main.cpp
    Viewer.cpp
)

add_executable(pdfviewer ${SOURCES})
target_link_libraries(pdfviewer Lagom::Core Lagom::Main Lagom::PDF Lagom::Gfx ${SDL2_LIBRARIES})
install(TARGETS pdfviewer RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
