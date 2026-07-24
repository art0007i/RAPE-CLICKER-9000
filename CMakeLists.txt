cmake_minimum_required(VERSION 3.10)

project(RAPE-CLICKER-9000 C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(WAYLAND_PROTOCOLS_DIR ${CMAKE_SOURCE_DIR}/protocols)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)

add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp

    ${IMGUI_DIR}/imgui_demo.cpp

    ${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

find_package(PkgConfig REQUIRED)
find_package(SDL3 REQUIRED)

pkg_check_modules(WAYLAND REQUIRED wayland-client)

pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)

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

wayland_generate(
    "${WAYLAND_PROTOCOLS_DIR}/xdg-output-unstable-v1.xml"
    xdg-output
)

add_executable(${PROJECT_NAME}
    stb_image.h
    ${xdg-output_SOURCES}

    main.c
    wayland.h
    wayland.c
    overlay_window.h
    overlay_window_sdl.c
    fake_mouse.h
    fake_mouse.c
    main_window.h
    main_window_imgui.cpp
    logo_anim.h
    logo_anim.c
    app_state.h
    app_state.c
    global_keybind.h
    global_keybind.c
    evtest_key.h
    config.h
    config.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${WAYLAND_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${WAYLAND_LIBRARIES}
    SDL3::SDL3
    imgui
)

target_compile_options(${PROJECT_NAME} PRIVATE -Wno-initializer-overrides)

# TODO: allow static linking (have you installed the static version of the wayland-client library?)
#target_link_options(${PROJECT_NAME} PRIVATE
#    -static
#)
