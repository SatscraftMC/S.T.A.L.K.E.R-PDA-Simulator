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

# libsecp256k1 (vendored, third_party/secp256k1): BIP-340 schnorrsig for the
# signed chat events. Static, tests/bench/examples off, only the modules the
# app uses (schnorrsig pulls extrakeys in for x-only keys).
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_EXHAUSTIVE_TESTS OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_CTIME_TESTS OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_BENCHMARK OFF CACHE BOOL "" FORCE)
set(SECP256K1_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SECP256K1_ENABLE_MODULE_ECDH OFF CACHE BOOL "" FORCE)
set(SECP256K1_ENABLE_MODULE_RECOVERY OFF CACHE BOOL "" FORCE)
set(SECP256K1_ENABLE_MODULE_MUSIG OFF CACHE BOOL "" FORCE)
set(SECP256K1_ENABLE_MODULE_ELLSWIFT OFF CACHE BOOL "" FORCE)
set(SECP256K1_ENABLE_MODULE_SILENTPAYMENTS OFF CACHE BOOL "" FORCE)
set(SECP256K1_ENABLE_MODULE_EXTRAKEYS ON CACHE BOOL "" FORCE)
set(SECP256K1_ENABLE_MODULE_SCHNORRSIG ON CACHE BOOL "" FORCE)
set(SECP256K1_VALGRIND "OFF" CACHE STRING "" FORCE)
# secp256k1 probes optional warning flags and caches the answers, and this
# build/ directory is shared by two compilers: the sandbox's gcc 15 answers
# "yes" to -Wleading-whitespace / -Wtrailing-whitespace (GCC >= 15 only),
# the host's gcc 13 compiles from the same cache and dies with
# "unrecognized command-line option". Both are cosmetic warnings, so the
# probes are pinned off here: an older compiler is never handed a flag a
# newer one agreed to.
set(C_SUPPORTS__WLEADING_WHITESPACE_SPACES 0 CACHE INTERNAL "" FORCE)
set(C_SUPPORTS__WTRAILING_WHITESPACE_ANY 0 CACHE INTERNAL "" FORCE)
add_subdirectory(third_party/secp256k1)

file(GLOB_RECURSE PDA_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_executable(pda ${PDA_SOURCES})
target_include_directories(pda PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(pda PRIVATE lvgl::lvgl secp256k1 Threads::Threads)

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
