#include "app_state.h"
#include "logo_anim.h"
#include "global_keybind.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

#include <SDL3/SDL.h>

#include <string.h>
#include <stdio.h>

static bool options_window;
static bool demo_window;

static bool need_refresh = true;

static bool done = false;

static SDL_Texture *anim_textures[LOGO_ANIM_FRAME_COUNT];
static size_t anim_frame = 0;
static float time_passed;

static char selected_device_path[MAX_NAME_SIZE];
static char selected_device_name[MAX_NAME_SIZE];

static device_array *old_devices;
static event_array *old_events;

static bool clicker_debounce;
static float detect_debounce;

static double click_delay = 10;

bool hovering_item_with_pad(int pad) {
    
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    
    min = ImVec2(min.x - pad, min.y - pad);
    max = ImVec2(max.x + pad, max.y + pad);
    
    return ImGui::IsMouseHoveringRect(min, max);
}

void imgui_main() {
    ImGuiIO io = ImGui::GetIO();
     
    float delay = logo_anim.delays[anim_frame] / 1000.0;
    while (time_passed > delay) {
        time_passed -= delay;
        anim_frame = (anim_frame + 1) % LOGO_ANIM_FRAME_COUNT;
        delay = logo_anim.delays[anim_frame] / 1000.0;
    }
    time_passed += io.DeltaTime;
    
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowPos(ImVec2());
    ImGui::Begin("MainWindow", NULL, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Exit")) {
                done = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Windows"))
        {
            //ImGui::MenuItem("Options", NULL, &options_window);
            //ImGui::MenuItem("Logs", NULL, logs_visible);
            ImGui::MenuItem("Demo Window", NULL, &demo_window);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float aspect =
        (float)LOGO_ANIM_HEIGHT / (float)LOGO_ANIM_WIDTH;
    ImVec2 size;
    size.x = avail.x;
    size.y = size.x * aspect;
    if (size.y > avail.y)
    {
        size.y = avail.y;
        size.x = size.y / aspect;
        
        ImVec2 cursor = ImGui::GetCursorPos();
        cursor.x += (avail.x - size.x) * 0.5f;
        ImGui::SetCursorPos(cursor);
    }
    ImGui::Image((ImTextureID)anim_textures[anim_frame], size);
    
    bool enable_click = get_clicking();
    ImGui::BeginDisabled(clicker_debounce);
    if (ImGui::Checkbox("1 million cps", &enable_click)) {
        printf("toggle clicking\n");
        fflush(stdout);
        set_clicking(enable_click);
        clicker_debounce = true;
    }
    if (!hovering_item_with_pad(8))
    {
        clicker_debounce = false;
    }
    ImGui::EndDisabled();
    
    ImGui::BeginDisabled(enable_click);
    if (ImGui::InputDouble("delay ms", &click_delay, 0.01f, 1.0f, "%.6f")) {
        printf("Delay = %f ms\n", click_delay);
        set_delay_ns(click_delay * 1000000);
    }
    ImGui::EndDisabled();
    
    
    char text[MAX_NAME_SIZE*2 + 64] = "NONE";
    bool is_some = strlen(selected_device_path) > 0;
    if (is_some) {
        sprintf(text, "%s (%s)", selected_device_name, selected_device_path);
    }
    if(ImGui::BeginCombo("keybind device", text)) {
        if (need_refresh) {
            need_refresh = false;
            refresh_devices();
            fflush(stdout);
        }
        
        if (ImGui::MenuItem("NONE", NULL, !is_some)) {
            memset(selected_device_name, 0, MAX_NAME_SIZE);
            memset(selected_device_path, 0, MAX_NAME_SIZE);
            set_wanted_device(NULL);
        }
        
        device_array *devices = get_devices();
        if (old_devices != devices && old_devices != nullptr) {
            printf("Freeing old devices! %p > %p\n", old_devices, devices);
            free(old_devices->devices);
            free(old_devices);
        }
        old_devices = devices;
        
        for (int i = 0; i < devices->count; ++i) {
            device *dev = &devices->devices[i];
            char text[MAX_NAME_SIZE*2 + 64] = {};
            sprintf(text, "%s (%s)##%d", dev->name, dev->path, i);
            bool selected = strcmp(dev->path, selected_device_path) == 0;
            if (ImGui::MenuItem(text, NULL, selected)) {
                memcpy(selected_device_name, dev->name, MAX_NAME_SIZE);
                memcpy(selected_device_path, dev->path, MAX_NAME_SIZE);
                set_wanted_device(strdup(selected_device_path));
            } else if (selected && strcmp(dev->name, selected_device_name) != 0) {
                memcpy(selected_device_name, dev->name, MAX_NAME_SIZE);                
            }
        }
        
        ImGui::EndCombo();
    } else if (!need_refresh) {
        printf("need refres\n");
        fflush(stdout);
        need_refresh = true;
    }
    
    event_array *events = get_events();
    if (old_events != events && old_events != nullptr) {
        printf("Freeing old events! %p > %p\n", old_events, events);
        free(old_events->events);
        free(old_events);
    }
    old_events = events;
    if (events != NULL && strcmp(events->path, selected_device_path) == 0) {
        int key = get_listen_key();
        char text[64] = "NONE";
        for (int i = 0; i < events->count; ++i) {
            event *evt = &events->events[i];
            if (evt->code == key) {
                sprintf(text, "%s (%d)", evt->name, evt->code);
                break;
            }
        }
        if (ImGui::BeginCombo("event to listen", text)) {
            if (ImGui::MenuItem("NONE", NULL, key == -1)) {
                set_listen_key(-1);
            }
            
            for (int i = 0; i < events->count; ++i) {
                event *evt = &events->events[i];
                char text[64] = {};
                sprintf(text, "%s (%d)", evt->name, evt->code);
                if (ImGui::MenuItem(text, NULL, key == evt->code)) {
                    set_listen_key(evt->code);
                }
            }
            ImGui::EndCombo();
        }
        
        ImGui::BeginDisabled(detect_debounce < 0.5f);
        bool want = get_want_read_key();
        const char* txt = want ? "Press Any Key" : "Detect Key";
        if (ImGui::Button(txt)) {
            set_clicking(false);
            set_want_read_key(!want);
            detect_debounce = 0;
        }
        if (!hovering_item_with_pad(8))
        {
            detect_debounce = 100;
        }
        detect_debounce += io.DeltaTime;
        ImGui::EndDisabled();
    }
    
    bool hold_mode = get_hold_mode();
    if(ImGui::Checkbox("hold mode", &hold_mode)) {
        set_hold_mode(hold_mode);
    }
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted("by default the bind toggles, with this you have to hold bind to activate");
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
    
    ImGui::End();
    if (demo_window) ImGui::ShowDemoWindow(&demo_window);
}

void load_gif(SDL_Renderer *renderer) {
    for (int i = 0; i < LOGO_ANIM_FRAME_COUNT; i++) {
        int w,h,channels;
        
        unsigned char* pixels = stbi_load_from_memory(logo_anim.frames[i], logo_anim.sizes[i], &w, &h, &channels, 4);
        SDL_Surface* surface = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_RGBA32, pixels, w * 4);
        anim_textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
        
        SDL_DestroySurface(surface);
        stbi_image_free(pixels);
    }
}

extern "C" int create_window() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr,
                "SDL_Init failed: %s\n",
                SDL_GetError());
        return -1;
    }
    
    click_delay = get_delay_ns() / 1000000.0;
    char *dev = get_wanted_device();
    strncpy(selected_device_name, "unknown (update dropdown to update)", MAX_NAME_SIZE);
    strncpy(selected_device_path, dev, MAX_NAME_SIZE);
    free(dev);
    
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_TRANSPARENT;
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    if (!SDL_CreateWindowAndRenderer("RAPE CLICKER 9000", (int)(640 * main_scale), (int)(480 * main_scale), window_flags, &window, &renderer))
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    
    load_gif(renderer);
    
    //SDL_SetRenderVSync(renderer, 1);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);
    
    ImGui::CreateContext();
    auto io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
    
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        
        ImGui_ImplSDL3_NewFrame();
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui::NewFrame();
        
        imgui_main();
        
        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        auto col = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
        SDL_SetRenderDrawColorFloat(renderer, col.x, col.y, col.z, col.w);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }
    
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext(NULL);
    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
