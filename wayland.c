#include <stdio.h>
#include <sys/ioctl.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/uinput.h>

#include <wayland-client.h>
#include "xdg-output-client-protocol.h"

struct output {
    struct wl_output *wl_output;
    struct zxdg_output_v1 *xdg_output;
    
    char *name;
    char *description;
    
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    
    int done;
};


struct state {
    struct wl_display *display;
    struct wl_registry *registry;
    
    struct zxdg_output_manager_v1 *xdg_manager;
    
    struct output outputs[32];
    int output_count;
};


static void output_name(void *data,
                        struct zxdg_output_v1 *xdg_output,
                        const char *name)
{
    struct output *out = data;
    
    free(out->name);
    out->name = (char *) strdup(name);
}


static void output_description(void *data,
                               struct zxdg_output_v1 *xdg_output,
                               const char *description)
{
    struct output *out = data;
    
    free(out->description);
    out->description = (char *) strdup(description);
}


static void logical_position(void *data,
                             struct zxdg_output_v1 *xdg_output,
                             int32_t x,
                             int32_t y)
{
    struct output *out = data;
    
    out->x = x;
    out->y = y;
}


static void logical_size(void *data,
                         struct zxdg_output_v1 *xdg_output,
                         int32_t width,
                         int32_t height)
{
    struct output *out = data;
    
    out->width = width;
    out->height = height;
}


static void xdg_output_done(void *data,
                            struct zxdg_output_v1 *xdg_output)
{
    struct output *out = data;
    out->done = 1;
}


static void xdg_output_v3_discard(void *data,
                                  struct zxdg_output_v1 *xdg_output)
{
}


static const struct zxdg_output_v1_listener xdg_listener = {
    .logical_position = logical_position,
    .logical_size = logical_size,
    .done = xdg_output_done,
    .name = output_name,
    .description = output_description,
    //.zxdg_output_v1_discarded = xdg_output_v3_discard,
};


static void output_geometry(void *data,
                            struct wl_output *output,
                            int32_t x,
                            int32_t y,
                            int32_t physical_width,
                            int32_t physical_height,
                            int32_t subpixel,
                            const char *make,
                            const char *model,
                            int32_t transform)
{
}


static void output_mode(void *data,
                        struct wl_output *output,
                        uint32_t flags,
                        int32_t width,
                        int32_t height,
                        int32_t refresh)
{
}


static void output_done(void *data,
                        struct wl_output *output)
{
}


static void output_scale(void *data,
                         struct wl_output *output,
                         int32_t factor)
{
}

static void l_output_name(void *data,
                          struct wl_output *output,
                          const char *name)
{
    printf("wl_output name: %s\n", name);
}


static void l_output_description(void *data,
                                 struct wl_output *output,
                                 const char *description)
{
    printf("wl_output description: %s\n", description);
}


static const struct wl_output_listener output_listener = {
    .geometry = output_geometry,
    .mode = output_mode,
    .done = output_done,
    .scale = output_scale,
    .name = l_output_name,
    .description = l_output_description,
};


static void registry_global(void *data,
                            struct wl_registry *registry,
                            uint32_t id,
                            const char *interface,
                            uint32_t version)
{
    struct state *state = data;
    
    
    if (strcmp(interface, "wl_output") == 0) {
        
        struct output *out =
            &state->outputs[state->output_count++];
        
        out->wl_output =
            wl_registry_bind(
                registry,
                id,
                &wl_output_interface,
                4);
        
        wl_output_add_listener(
            out->wl_output,
            &output_listener,
            out);
        
        
        if (state->xdg_manager) {
            out->xdg_output =
                zxdg_output_manager_v1_get_xdg_output(
                    state->xdg_manager,
                    out->wl_output);
            
            zxdg_output_v1_add_listener(
                out->xdg_output,
                &xdg_listener,
                out);
        }
    }
    
    
    if (strcmp(interface,
               "zxdg_output_manager_v1") == 0) {
        
        state->xdg_manager =
            wl_registry_bind(
                registry,
                id,
                &zxdg_output_manager_v1_interface,
                3);
    }
}


static void registry_remove(void *data,
                            struct wl_registry *registry,
                            uint32_t id)
{
}


static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_remove,
};


int main6(void)
{
    struct state state = {0};
    
    
    state.display = wl_display_connect(NULL);
    
    if (!state.display) {
        fprintf(stderr,
                "Cannot connect to Wayland\n");
        return 1;
    }
    
    
    state.registry =
        wl_display_get_registry(state.display);
    
    
    wl_registry_add_listener(
        state.registry,
        &registry_listener,
        &state);
    
    
    /*
     * First roundtrip:
     * discovers globals.
     */
    wl_display_roundtrip(state.display);
    
    
    /*
     * If xdg-output manager appeared after outputs,
     * create xdg outputs now.
     */
    for (int i = 0; i < state.output_count; i++) {
        struct output *out = &state.outputs[i];
        
        if (!out->xdg_output && state.xdg_manager) {
            out->xdg_output =
                zxdg_output_manager_v1_get_xdg_output(
                    state.xdg_manager,
                    out->wl_output);
            
            zxdg_output_v1_add_listener(
                out->xdg_output,
                &xdg_listener,
                out);
        }
    }
    
    
    /*
     * Receive output data.
     */
    wl_display_roundtrip(state.display);
    
    
    for (int i = 0; i < state.output_count; i++) {
        
        struct output *out = &state.outputs[i];
        
        printf("Output %d\n", i);
        printf("  name:        %s\n",
               out->name ?: "(unknown)");
        
        printf("  description: %s\n",
               out->description ?: "(unknown)");
        
        printf("  position:    %d,%d\n",
               out->x,
               out->y);
        
        printf("  size:        %dx%d\n",
               out->width,
               out->height);
        
        printf("\n");
    }
    
    
    wl_display_disconnect(state.display);
    
    return 0;
}