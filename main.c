#include "overlay_window.h"

#include <SDL3/SDL.h>
#include <stdio.h>

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    
    
    
    OverlayResult result;
    
    
    if (overlay_capture(4, &result) == 0) {
        
        for (size_t i = 0;
             i < result.count;
             i++)
        {
            printf(
                "%d %.3f %.3f\n",
                result.points[i].mon_id,
                result.points[i].x,
                result.points[i].y
                );
        }
    }
    

    return 0;
}