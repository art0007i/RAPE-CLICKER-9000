#include "overlay_window.h"
#include "main_window.h"
#include "fake_mouse.h"
#include "global_keybind.h"
#include "app_state.h"
#include "config.h"

#include <stdio.h>
#include <threads.h>

int main() {
    load_config();
    
    thrd_t t_mouse;
    thrd_create(&t_mouse, mouse_thread, NULL);
    thrd_t t_bind;
    thrd_create(&t_bind, keybind_thread, NULL);
    
    int result = create_window();
    
    set_running(false);
    thrd_join(t_mouse, NULL);
    thrd_join(t_bind, NULL);
    
    save_config();
    /*
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
    */

    return result;
}