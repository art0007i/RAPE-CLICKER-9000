cmake_minimum_required(VERSION 3.10)

project(RAPE-CLICKER-9000 C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(APP_NAME "RAPE CLICKER 9000")
set(APP_VERSION "0.1.0")
string(TIMESTAMP APP_BUILD_DATE "%Y-%m-%d %H:%M:%S UTC" UTC)

execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE APP_GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT APP_GIT_COMMIT)
    set(APP_GIT_COMMIT "unknown")
endif()

configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/version.h.in
    ${CMAKE_BINARY_DIR}/generated/version.h
)

#set(WAYLAND_PROTOCOLS_DIR ${CMAKE_SOURCE_DIR}/protocols)

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.8
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.12
    GIT_SHALLOW TRUE
)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp

    ${imgui_SOURCE_DIR}/imgui_demo.cpp

    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

#find_package(PkgConfig REQUIRED)
#find_package(SDL3 REQUIRED)

#pkg_check_modules(WAYLAND REQUIRED wayland-client)

#pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)

function(wayland_generate protocol_file base_name)
    set(out_dir "${CMAKE_BINARY_DIR}/generated")
    file(MAKE_DIRECTORY ${out_dir})

    set(header "${out_dir}/${base_name}-client-protocol.h")
    set(source "${out_dir}/${base_name}-protocol.c")

    add_custom_command(
        OUTPUT ${header}
        COMMAND ${WAYLAND_SCANNER} client-header ${protocol_file} ${header}
        DEPENDS ${protocol_file}
        VERBATIM
    )

    add_custom_command(
        OUTPUT ${source}
        COMMAND ${WAYLAND_SCANNER} private-code ${protocol_file} ${source}
        DEPENDS ${protocol_file} ${header}
        VERBATIM
    )

    set(${base_name}_SOURCES ${source} ${header} PARENT_SCOPE)
endfunction()

#wayland_generate(
#    "${WAYLAND_PROTOCOLS_DIR}/xdg-output-unstable-v1.xml"
#    xdg-output
#)

add_executable(${PROJECT_NAME}
    assets/assets.h
    fake_mouse.h
    main_window.h
    app_state.h
    global_keybind.h
    evtest_key.h
    config.h
    licenses.h
    ini.h

    assets/assets.c
    main.c
    fake_mouse.c
    main_window_imgui.cpp
    app_state.c
    global_keybind.c
    evtest_key.c
    config.c
    licenses.c
    ini.c
#    ${xdg-output_SOURCES}
#    wayland.h
#    wayland.c
#    overlay_window.h
#    overlay_window_sdl.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/generated
#    ${WAYLAND_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3-static
    imgui
#    ${WAYLAND_LIBRARIES}
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wno-initializer-overrides)

# TODO: allow static linking (have you installed the static version of the wayland-client library?)
#target_link_options(${PROJECT_NAME} PRIVATE
#    -static
#)
