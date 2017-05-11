cmake_minimum_required(VERSION 2.8)

project(acidwarp C)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

include_directories("${PROJECT_SOURCE_DIR}")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    SET(STATIC_DEFAULT ON)
else(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    SET(STATIC_DEFAULT OFF)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

option(SDL2 "Version of SDL to use (1 or 2)" ON)
option(GL "Use OpenGL" OFF)
option(STATIC "Link statically" ${STATIC_DEFAULT})

set(CORE_SOURCES acidwarp.c palinit.c rolnfade.c display.c)
set(IMGGEN_SOURCES bit_map.c lut.c img_int.c img_float.c)
set(SOURCES ${CORE_SOURCES} ${IMGGEN_SOURCES} draw.c)
add_executable(acidwarp ${SOURCES})

if(STATIC)
    # FIXME depends on GCC
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif(STATIC)

if(SDL2)
    # FindSDL2 probably won't be available
    include(FindPkgConfig)
    pkg_search_module(SDL REQUIRED sdl2)
    include_directories(${SDL_INCLUDE_DIRS})
    if (STATIC)
        target_link_libraries(acidwarp ${SDL_STATIC_LDFLAGS})
    else (STATIC)
        target_link_libraries(acidwarp ${SDL_LDFLAGS})
    endif (STATIC)
else(SDL2)
    find_package(SDL REQUIRED)
    include_directories(${SDL_INCLUDE_DIR})
    target_link_libraries(acidwarp ${SDL_LIBRARY})
endif(SDL2)
