cmake_minimum_required(VERSION 3.20)
project(DORN VERSION 1.0.0 LANGUAGES C)

include(CTest)
option(DORN_BUILD_TESTS "Build the game-logic tests" ON)
option(DORN_BUILD_APP "Build the SDL2 game" ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(dorn_game STATIC src/game.c src/board.c)
target_include_directories(dorn_game PUBLIC include)
if(MSVC)
    target_compile_options(dorn_game PRIVATE /W4)
else()
    target_compile_options(dorn_game PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(DORN_BUILD_TESTS)
    add_executable(dorn_game_tests tests/test_game.c)
    target_link_libraries(dorn_game_tests PRIVATE dorn_game)
    add_test(NAME dorn_game_tests COMMAND dorn_game_tests)
endif()

if(DORN_BUILD_APP)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)

    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL/CMakeLists.txt")
        message(FATAL_ERROR "No se encontro SDL2 en native/vendor/SDL.")
    endif()
    add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

    add_executable(dorn src/main.c src/image.c)
    target_include_directories(dorn PRIVATE include vendor)
    if(MSVC)
        target_compile_options(dorn PRIVATE /W4)
    else()
        target_compile_options(dorn PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        set_source_files_properties(src/image.c PROPERTIES
            COMPILE_OPTIONS "-Wno-unused-but-set-variable")
    endif()
    target_link_libraries(dorn PRIVATE dorn_game SDL2-static)
    if(TARGET SDL2main)
        target_link_libraries(dorn PRIVATE SDL2main)
    endif()
    if(WIN32)
        target_link_libraries(dorn PRIVATE winmm imm32 version setupapi)
    endif()

    set(DORN_ASSET_SOURCE
        "${CMAKE_CURRENT_SOURCE_DIR}/../src/com/dorn/assets")
    add_custom_command(TARGET dorn POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${DORN_ASSET_SOURCE}" "$<TARGET_FILE_DIR:dorn>/assets")
    if(DORN_BUILD_TESTS AND UNIX)
        add_test(NAME dorn_sdl_smoke COMMAND dorn --smoke-test)
        set_tests_properties(dorn_sdl_smoke PROPERTIES
            ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    endif()
endif()
