#include "app_state.h"
#include "assets/assets.h"
#include "global_keybind.h"
#include "evtest_key.h"
#include "licenses.h"
#include "version.h"
#include "config.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "imgui_internal.h"

#include <SDL3/SDL.h>

#include <string.h>
#include <stdio.h>
#include <linux/uinput.h>

#define DEG2RAD IM_PI/180

static ImFont *minecraft_font;
static float splash_anim;

static bool options_window;
static bool demo_window;
static bool licenses_window;
static bool appinfo_window;

static bool need_refresh = true;

static bool done = false;

static SDL_Texture *icon_texture;
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
static double click_length = 1;

ImVec2 rotate_vec2(ImVec2 v, float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);
    
    return ImVec2(
        v.x * c - v.y * s,
        v.x * s + v.y * c
        );
}

void add_rotated_text(
    ImDrawList* draw,
    ImFont* font,
    float size,
    ImVec2 pos,
    ImU32 color,
    const char* text,
    float angle,
    float post_scale = 1)
{
    int vtx_start = draw->VtxBuffer.Size;
    
    size = size / 24;
    draw->AddText(
        font,
        24,
        ImVec2(0,100),
        color,
        text
    );
    
    int vtx_end = draw->VtxBuffer.Size;
    
    if (vtx_start == vtx_end)
        return;
    
    // Calculate center
    ImVec2 min(FLT_MAX, FLT_MAX);
    ImVec2 max(-FLT_MAX, -FLT_MAX);
    
    for (int i = vtx_start; i < vtx_end; i++)
    {
        ImVec2 p = draw->VtxBuffer[i].pos;
        
        min.x = ImMin(min.x, p.x);
        min.y = ImMin(min.y, p.y);
        
        max.x = ImMax(max.x, p.x);
        max.y = ImMax(max.y, p.y);
    }
    
    ImVec2 center = {
        (min.x + max.x) * 0.5f,
        (min.y + max.y) * 0.5f
    };
    
    
    float s = sinf(angle);
    float c = cosf(angle);
    
    // Rotate vertices
    for (int i = vtx_start; i < vtx_end; i++)
    {
        ImVec2 p = draw->VtxBuffer[i].pos;
        
        p -= center;
        p *= size;
        p *= post_scale;
        
        draw->VtxBuffer[i].pos =
            {
                pos.x + p.x * c - p.y * s,
                pos.y + p.x * s + p.y * c
            };
    }
}

void color_picker_u32(const char *name, uint32_t *col) {
    ImVec4 vec = ImGui::ColorConvertU32ToFloat4(*col);
    if (ImGui::ColorEdit4(name, (float*)&vec, ImGuiColorEditFlags_Uint8))
        *col = ImGui::ColorConvertFloat4ToU32(vec);
}

bool hovering_item_with_pad(int pad) {
    
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    
    ImVec2 pad2 = ImVec2(pad, pad);
    min -= pad2;
    max += pad2;
    
    return ImGui::IsMouseHoveringRect(min, max);
}

void add_tooltip(const char* tip) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(tip);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
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
            ImGui::MenuItem("Options", NULL, &options_window);
            //ImGui::MenuItem("Logs", NULL, logs_visible);
            ImGui::MenuItem("Demo Window", NULL, &demo_window);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("About"))
        {
            ImGui::MenuItem("App Info", NULL, &appinfo_window);
            ImGui::MenuItem("Licenses", NULL, &licenses_window);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float aspect =
        (float)LOGO_ANIM_HEIGHT / (float)LOGO_ANIM_WIDTH;
    ImVec2 size;
    ImVec2 cursor = ImGui::GetCursorPos();
    size.x = avail.x;
    size.y = size.x * aspect;
    if (size.y > avail.y)
    {
        size.y = avail.y;
        size.x = size.y / aspect;
        
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
        set_click_delay(click_delay * 1000000);
    }
    add_tooltip("using a time less than 1 ms might have detrimental effects on your OS's performance. you have been warned.");
    if (ImGui::InputDouble("length ms", &click_length, 0.01f, 1.0f, "%.6f")) {
        printf("Delay = %f ms\n", click_length);
        set_click_length(click_length * 1000000);
    }
    add_tooltip("how long the mouse button is pressed down for each click");
    int key = get_click_button();
    if (ImGui::BeginCombo("mouse button", get_key_name(key))) {
        for (int i = BTN_LEFT; i <= BTN_EXTRA; ++i) {
            if (ImGui::MenuItem(get_key_name(i), NULL, key == i)) {
                set_click_button(i);
            }
        }
        ImGui::EndCombo();
    }
    add_tooltip("the mouse button which will be clicked automatically");
    int limit = get_click_limit();
    if (ImGui::InputInt("click limit", &limit)) {
        set_click_limit(limit);
    }
    add_tooltip("the clicking will stop automatically after this amount of clicks. set to negative number to disable");
    bool hold_mode = get_keybind_hold();
    if(ImGui::Checkbox("hold mode", &hold_mode)) {
        set_keybind_hold(hold_mode);
    }
    add_tooltip("by default the bind toggles, with this you have to hold bind to activate");
    ImGui::EndDisabled();
    
    ImGui::Separator();
    
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
            set_keybind_device(NULL);
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
                set_keybind_device(selected_device_path);
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
        int key = get_keybind_event();
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
                set_keybind_event(-1);
            }
            
            for (int i = 0; i < events->count; ++i) {
                event *evt = &events->events[i];
                char text[64] = {};
                sprintf(text, "%s (%d)", evt->name, evt->code);
                if (ImGui::MenuItem(text, NULL, key == evt->code)) {
                    set_keybind_event(evt->code);
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
    
    splash *splash_params = get_splash_params();
    if (splash_params->splash_enable) {
        float angle = splash_params->splash_angle * DEG2RAD;
        splash_anim += io.DeltaTime * splash_params->splash_bounce_speed;
        splash_anim = fmod(splash_anim, 2*IM_PI);
        float font_size = size.y * splash_params->splash_size;
        float post_size = 1 - splash_params->splash_bounce_size * abs(sin(splash_anim));
        float pad = font_size/8.0;
        ImVec2 pos = ImVec2(cursor.x + size.x * splash_params->splash_xpos, cursor.y + size.y * splash_params->splash_ypos) - ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY());
        ImVec2 bg_pos = rotate_vec2(ImVec2(pad, pad), angle) + pos;
        
        // Color is ABGR
        add_rotated_text(ImGui::GetWindowDrawList(), minecraft_font, font_size, bg_pos, ImU32(splash_params->splash_color_bg), splash_params->splash_text, angle, post_size);
        add_rotated_text(ImGui::GetWindowDrawList(), minecraft_font, font_size, pos, ImU32(splash_params->splash_color), splash_params->splash_text, angle, post_size);
    }
    
    ImGui::End();
    if (options_window) {
        ImGui::Begin("Options", &options_window);
        
        ImGui::Checkbox("enable splash", &splash_params->splash_enable);
        ImGui::DragFloat("splash xpos", &splash_params->splash_xpos, 0.005);
        ImGui::DragFloat("splash ypos", &splash_params->splash_ypos, 0.005);
        ImGui::DragFloat("splash angle", &splash_params->splash_angle, 0.1);
        ImGui::DragFloat("splash size", &splash_params->splash_size, 0.005, 0.01, 9999);
        ImGui::DragFloat("splash bounce speed", &splash_params->splash_bounce_speed, 0.01);
        ImGui::DragFloat("splash bounce size", &splash_params->splash_bounce_size, 0.001);
        ImGui::InputText("splash text", splash_params->splash_text, 256);
        color_picker_u32("splash color", &splash_params->splash_color);
        color_picker_u32("splash bg color", &splash_params->splash_color_bg);
        
        if (ImGui::Button("Reset Splash Params")) {
            reset_splash_params();
        }
        
        if (ImGui::Button("Save Config")) {
            set_want_save_config(true); 
        }
        
        ImGui::End();
    }
    if (appinfo_window) {
        ImGui::Begin("App Info", &appinfo_window);
        ImGui::Columns(2, NULL, false);
        ImVec2 spacing = ImGui::GetStyle().ItemSpacing;
        float size = ImGui::GetTextLineHeightWithSpacing() * 4 - spacing.y;
        ImGui::Image(icon_texture, ImVec2(size, size));
        ImGui::SetColumnWidth(0, size + spacing.x * 2); // remove hardcoding
        ImGui::NextColumn();
        ImGui::Text("%s v%s", APP_NAME, APP_VERSION);
        ImGui::Text(" ");
        ImGui::Text("Compiled on %s", APP_BUILD_DATE);
        ImGui::Text("git commit '%s'", APP_GIT_COMMIT);
        ImGui::EndColumns();
        ImGui::NewLine();
        ImGui::Text("Thank you for using %s! <3", APP_NAME);
        const license *my_license = get_own_license();
        ImGui::TextLinkOpenURL("Open source code repo", my_license->url);
        if (ImGui::TreeNode("Software License")) {
            ImGui::Text("%s", my_license->license);
            ImGui::TreePop();
        }
        ImGui::End();
    }
    if (licenses_window) {
        ImGui::Begin("Licenses", &licenses_window);
        ImGui::Text("%s is made possible by the following open source libraries:", APP_NAME);
        const license *licenses = get_licenses();
        size_t license_count = get_license_count();
        for (int i = 0; i < license_count; ++i) {
            ImGui::NewLine();
            bool node = (ImGui::CollapsingHeader(licenses[i].lib));
            ImGui::PushID(i);
            ImGui::TextLinkOpenURL("Open source code repo", licenses[i].url);
            if (node) {
                ImGui::Text("%s", licenses[i].license);
            }
            ImGui::PopID();
        }
        ImGui::End();
    }    
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
    
    // TODO: remove these few lines, instead the ui should not store state at all...
    // these will have the issue that changing them from other sources will not update imgui
    click_delay = get_click_delay() / 1000000.0;
    click_length = get_click_length() / 1000000.0;
    char *dev = get_keybind_device();
    strncpy(selected_device_name, "unknown (open dropdown to update)", MAX_NAME_SIZE);
    strncpy(selected_device_path, dev, MAX_NAME_SIZE);
    free(dev);
    
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_TRANSPARENT;
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    if (!SDL_CreateWindowAndRenderer(APP_NAME, (int)(640 * main_scale), (int)(480 * main_scale), window_flags, &window, &renderer))
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    int x,y,comp;
    unsigned char *pixels = stbi_load_from_memory(app_icon, APP_ICON_BYTES, &x, &y, &comp, 4);
    SDL_Surface* surface = SDL_CreateSurfaceFrom(x, y, SDL_PIXELFORMAT_RGBA32, pixels, x * 4);
    icon_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetWindowIcon(window, surface);
    SDL_DestroySurface(surface);
    stbi_image_free(pixels);
    
    load_gif(renderer);
    
    //SDL_SetRenderVSync(renderer, 1);
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);
    
    ImGui::CreateContext();
    ImGuiIO *io = &ImGui::GetIO();
    
    static char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/imgui.ini", get_config_dir());
    io->IniFilename = path;
    
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    io->Fonts->AddFontDefault();
    ImFontConfig cfg;
    cfg.FontDataOwnedByAtlas = false;
    strncpy(cfg.Name, "Minecraft", sizeof(cfg.Name));
    minecraft_font = io->Fonts->AddFontFromMemoryTTF((void*)mc_font, MC_FONT_BYTES, 16.0f, &cfg);
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
        ImGui::Text("ImGui ini: %s", ImGui::GetIO().IniFilename);
        
        imgui_main();
        
        ImGui::Render();
        SDL_SetRenderScale(renderer, io->DisplayFramebufferScale.x, io->DisplayFramebufferScale.y);
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
