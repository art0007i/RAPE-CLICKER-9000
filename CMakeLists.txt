cmake_minimum_required(VERSION 3.10)

project(RAPE-CLICKER-9000 C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(WAYLAND_PROTOCOLS_DIR ${CMAKE_SOURCE_DIR}/protocols)

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
    main.c
    wayland.c
    wayland.h
    ${xdg-output_SOURCES}
    overlay_window_sdl.c
    overlay_window.h
    fake_mouse.c
    fake_mouse.h
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${WAYLAND_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${WAYLAND_LIBRARIES}
    SDL3::SDL3
)

# TODO: allow static linking (have you installed the static version of the wayland-client library?)
#target_link_options(${PROJECT_NAME} PRIVATE
#    -static
#)
