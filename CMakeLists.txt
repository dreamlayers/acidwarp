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

# Windows icon, built by converting PNG to ICO
# and then embedding using resource file.
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    find_program(ICOTOOL icotool)
    if(ICOTOOL)
        add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/acidwarp.ico
                           COMMAND icotool -c -o ${CMAKE_SOURCE_DIR}/acidwarp.ico ${CMAKE_SOURCE_DIR}/acidwarp.png
                           MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/acidwarp.png)
        set(SOURCES ${SOURCES} acid_res.rc)
        set_source_files_properties(acid_res.rc PROPERTIES OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/acidwarp.ico)
    else(ICOTOOL)
        message(WARNING "Not adding icon to executable because icotool not found.")
    endif(ICOTOOL)
endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")

# Icon set via SDL
find_program(XXD xxd)
find_program(CONVERT convert)
# TODO Make sure convert is ImageMagick
if(XXD AND CONVERT)
    add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/acidwarp.rgb
                       COMMAND convert ${CMAKE_SOURCE_DIR}/acidwarp.png -sample 64x64 ${CMAKE_SOURCE_DIR}/acidwarp.rgb
                       MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/acidwarp.png)
    add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/acid_ico.c
                       COMMAND xxd -i acidwarp.rgb > acid_ico.c
                       MAIN_DEPENDENCY ${CMAKE_SOURCE_DIR}/acidwarp.rgb
                       WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    add_definitions(-DADDICON)
    set(SOURCES ${SOURCES} acid_ico.c)
else(XXD AND CONVERT)
    if(NOT CONVERT)
        message(WARNING "ImageMagick convert is required resize icon for embedding")
    endif(NOT CONVERT)
    if(NOT XXD)
        message(WARNING "xxd is required to embed icon into code")
    endif(NOT XXD)
endif(XXD AND CONVERT)

add_executable(acidwarp ${SOURCES})

if(STATIC)
    # FIXME depends on GCC
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif(STATIC)

if(GL)
    if (NOT SDL2)
        message(FATAL_ERROR "OpenGL only supported with SDL 2." )
    endif (NOT SDL2)
    add_definitions(-DWITH_GL)
    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        if(STATIC)
            add_definitions(-DGLEW_STATIC)
        endif(STATIC)
        target_link_libraries(acidwarp glew32 opengl32)
    else(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        find_package(OpenGL REQUIRED)
        target_link_libraries(acidwarp ${OPENGL_gl_LIBRARY})
    endif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
endif(GL)

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
