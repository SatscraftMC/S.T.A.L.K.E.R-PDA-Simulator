cmake_minimum_required(VERSION 3.16)
project(pda LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Python COMPONENTS Interpreter REQUIRED)
find_package(Threads REQUIRED)

set(LV_BUILD_SET_CONFIG_OPTS ON CACHE BOOL "" FORCE)
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(LV_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/lvgl)

file(GLOB_RECURSE PDA_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_executable(pda ${PDA_SOURCES})
target_include_directories(pda PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(pda PRIVATE lvgl::lvgl Threads::Threads)

# an undeclared function is assumed to return int - which silently truncates
# a pointer (getenv without <stdlib.h> did exactly that)
target_compile_options(pda PRIVATE -Werror=implicit-function-declaration)

if(UNIX)
  target_link_libraries(pda PRIVATE m)
endif()

# SDL2 is used directly for the mouse wheel (LVGL 9 only routes the wheel to
# the focused object, but we want to zoom towards the cursor under the mouse)
find_package(SDL2 QUIET)
if(SDL2_FOUND)
  target_include_directories(pda PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(pda PRIVATE ${SDL2_LIBRARIES})
endif()

# GLib/GIO for BlueZ D-Bus API (BIT carrier)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0 gobject-2.0 gio-unix-2.0)
target_include_directories(pda PRIVATE ${GLIB_INCLUDE_DIRS})
target_link_libraries(pda PRIVATE ${GLIB_LIBRARIES})
