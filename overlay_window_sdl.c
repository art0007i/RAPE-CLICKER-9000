#include "overlay_window.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>


typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    
    SDL_DisplayID display_id;
    
    char name[128];
    
    int x;
    int y;
    int width;
    int height;
} OverlayMonitor;


static int create_monitor_overlay(OverlayMonitor *mon)
{
    mon->window = SDL_CreateWindow(
        mon->name,
        mon->width,
        mon->height,
        SDL_WINDOW_BORDERLESS |
        SDL_WINDOW_ALWAYS_ON_TOP |
        SDL_WINDOW_TRANSPARENT
        );
    
    if (!mon->window) {
        fprintf(stderr,
                "CreateWindow failed: %s\n",
                SDL_GetError());
        return -1;
    }
      
    SDL_SetWindowPosition(
        mon->window,
        mon->x,
        mon->y
        );
    
    
    SDL_SetWindowFullscreen(
        mon->window,
        true
        );
    
    
    mon->renderer =
        SDL_CreateRenderer(
            mon->window,
            NULL
            );
    
    
    if (!mon->renderer) {
        fprintf(stderr,
                "CreateRenderer failed: %s\n",
                SDL_GetError());
        return -1;
    }
    
    
    return 0;
}



static void destroy_monitor_overlay(
    OverlayMonitor *mon
    )
{
    if (mon->renderer)
        SDL_DestroyRenderer(mon->renderer);
    
    if (mon->window)
        SDL_DestroyWindow(mon->window);
}



static int enumerate_monitors(
    OverlayMonitor *monitors,
    int max_monitors
    )
{
    int count = 0;
    
    SDL_DisplayID *displays =
        SDL_GetDisplays(&count);
    
    
    if (!displays)
        return 0;
    
    
    if (count > max_monitors)
        count = max_monitors;
    
    
    for (int i = 0; i < count; i++) {
        
        SDL_Rect bounds;
        
        if (!SDL_GetDisplayBounds(
                displays[i],
                &bounds))
        {
            continue;
        }
        
        
        OverlayMonitor *mon =
            &monitors[i];
        
        
        memset(mon, 0, sizeof(*mon));
        
        
        mon->display_id =
            displays[i];
        
        
        mon->x =
            bounds.x;
        
        mon->y =
            bounds.y;
        
        mon->width =
            bounds.w;
        
        mon->height =
            bounds.h;
        
        
        const char *name =
            SDL_GetDisplayName(
                displays[i]
                );
        
        
        if (name) {
            strncpy(
                mon->name,
                name,
                sizeof(mon->name)-1
                );
        }
        
        
        printf(
            "Display: %s pos=%d,%d size=%dx%d id=%d\n",
            mon->name,
            mon->x,
            mon->y,
            mon->width,
            mon->height,
            mon->display_id
            );
        fflush(stdout);
        
        
        create_monitor_overlay(mon);
    }
    
    
    SDL_free(displays);
    
    return count;
}


static void draw_overlay(
    OverlayMonitor *mon,
    OverlayResult *result
    )
{
    SDL_SetRenderDrawColor(
        mon->renderer,
        0,
        0,
        0,
        64
        );
    
    SDL_RenderClear(mon->renderer);
    
    
    SDL_SetRenderDrawColor(
        mon->renderer,
        255,
        0,
        0,
        255
        );
    
    
    for (size_t i = 0; i < result->count; i++) {
        OverlayPoint *point = &result->points[i];
        
        if (mon->display_id != point->mon_id) continue;
        
        int x = point->x * mon->width;
        int y = point->y * mon->height;
        
        
        SDL_FRect r = {
            x - 5,
            y - 5,
            10,
            10
        };
        
        SDL_RenderRect(
            mon->renderer,
            &r
            );
    }
    
    
    SDL_RenderPresent(mon->renderer);
}

int overlay_capture(
    size_t max_points,
    OverlayResult *result
    )
{
    memset(result, 0, sizeof(*result));
    
    
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr,
                "SDL_Init failed: %s\n",
                SDL_GetError());
        return -1;
    }
    
    
    OverlayMonitor monitors[16];
    
    int monitor_count =
        enumerate_monitors(
            monitors,
            16
            );
    
    
    if (monitor_count == 0)
        return -1;
    
    
    
    int running = 1;
    
    
    while (running) {
        
        SDL_Event event;
        
        for (size_t i = 0; i < monitor_count; ++i) {
            OverlayMonitor mon = monitors[i];
            draw_overlay(&mon, result);
        }
        
        while (SDL_PollEvent(&event)) {
            
            switch (event.type) {
                
            case SDL_EVENT_QUIT:
                result->cancelled = 1;
                running = 0;
                
                
            case SDL_EVENT_KEY_DOWN:
                
                if (event.key.key == SDLK_ESCAPE)
                {
                    result->cancelled = 1;
                    running = 0;
                }
                
                break;
                
                
                
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                
                if (event.button.button ==
                    SDL_BUTTON_LEFT)
                {
                    
                    if (result->count < max_points)
                    {
                        OverlayPoint *p =
                            &result->points[
                                result->count++
                        ];
                        
                        
                        SDL_Window *clicked =
                            SDL_GetWindowFromID(
                                event.window.windowID
                                );
                        
                        
                        OverlayMonitor *mon = NULL;
                        
                        
                        for (int i = 0;
                             i < monitor_count;
                             i++)
                        {
                            if (monitors[i].window ==
                                clicked)
                            {
                                mon = &monitors[i];
                                break;
                            }
                        }
                        
                        
                        if (mon) {
                            
                            p->x =
                                event.button.x /
                                (float)mon->width;
                            
                            p->y =
                                event.button.y /
                                (float)mon->height;
                            
                            p->mon_id =
                                mon->display_id;
                        }
                        
                        
                        if (result->count >= max_points)
                            running = 0;
                    }
                }
                
                break;
            }
        }
        
        
        SDL_Delay(10);
    }
    
    
    for (int i = 0; i < monitor_count; i++)
        destroy_monitor_overlay(&monitors[i]);
    
    SDL_Quit();
    
    return result->cancelled ? -1 : 0;
}