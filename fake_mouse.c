#include "fake_mouse.h"

#include "app_state.h"

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <threads.h>
#include <stdbool.h>
#include <stdatomic.h>


void nssleep(uint64_t ns) {
    struct timespec ts;
    
    ts.tv_sec = ns / 1000000000ULL;
    ts.tv_nsec = ns % 1000000000ULL;
    
    thrd_sleep(&ts, NULL);
}


static int mouse_fd;

void init_mouse() {
    mouse_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (mouse_fd < 0)
    {
        printf("Could not open /dev/uinput (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    // Tell os it's a mouse
    ioctl(mouse_fd, UI_SET_PROPBIT, INPUT_PROP_POINTER);
    
    // Allow all buttons
    ioctl(mouse_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(mouse_fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(mouse_fd, UI_SET_KEYBIT, BTN_MIDDLE);
    ioctl(mouse_fd, UI_SET_KEYBIT, BTN_RIGHT);
    //ioctl(mouse_fd, UI_SET_KEYBIT, BTN_SIDE);
    //ioctl(mouse_fd, UI_SET_KEYBIT, BTN_EXTRA);
    
    // Allow relative movement
    ioctl(mouse_fd, UI_SET_EVBIT, EV_REL);
    ioctl(mouse_fd, UI_SET_RELBIT, REL_X);
    ioctl(mouse_fd, UI_SET_RELBIT, REL_Y);
    ioctl(mouse_fd, UI_SET_RELBIT, REL_WHEEL);
    ioctl(mouse_fd, UI_SET_RELBIT, REL_HWHEEL);
    
    // Allow absolute movement
    // ioctl(mouse_fd, UI_SET_EVBIT, EV_ABS);
    // ioctl(mouse_fd, UI_SET_ABSBIT, ABS_X);
    // ioctl(mouse_fd, UI_SET_ABSBIT, ABS_Y);
    
    // Setup device params
    struct uinput_setup setup;
    
    memset(&setup, 0, sizeof(setup));
    setup.id.bustype = BUS_USB;
    setup.id.vendor = 0x4172;
    setup.id.product = 0x7469;
    setup.id.version = 1;
    strcpy(setup.name, "Mouse Raper 9000");
    ioctl(mouse_fd, UI_DEV_SETUP, &setup);
    
    // Actually create device!
    ioctl(mouse_fd, UI_DEV_CREATE);
    
    static char sys_name[128];
    ioctl(mouse_fd, UI_GET_SYSNAME(128), sys_name);
    
    printf("Created mouse input device: %s\n", sys_name);
    fflush(stdout);
}

void write_event(int fd, int type, int code, int val)
{
    struct input_event ie = {0};
    
    ie.type = type;
    ie.code = code;
    ie.value = val;
    
    if (write(fd, &ie, sizeof(ie)) < 0)
        perror("write_event");
}

void mouse_move(float x, float y)
{    
    int x_rel = (x * 65535);
    int y_rel = (x * 65535);
    
    write_event(mouse_fd, EV_ABS, ABS_X, x_rel);
    write_event(mouse_fd, EV_ABS, ABS_Y, y_rel);
    write_event(mouse_fd, EV_SYN, SYN_REPORT, 0);
    // 1 px relative movement prevents mouse from going to sleep mode
    write_event(mouse_fd, EV_REL, REL_X, 1);
    write_event(mouse_fd, EV_REL, REL_Y, 1);
    write_event(mouse_fd, EV_SYN, SYN_REPORT, 0);
}

void mouse_click(int button, int delay) {
    write_event(mouse_fd, EV_KEY, button, 1);
    write_event(mouse_fd, EV_SYN, SYN_REPORT, 0);
    
    nssleep(delay);
    
    write_event(mouse_fd, EV_KEY, button, 0);
    write_event(mouse_fd, EV_SYN, SYN_REPORT, 0);
}

static atomic_bool running = true;

int mouse_thread() {
    init_mouse();
    
    while(get_running()) {
        uint64_t ns = get_delay_ns();
        int delay = 1000000;
        if (ns < 1000000) {
            delay = ns;
            ns = 0;
        } else {
            ns -= 1000000;
        }
        if (get_clicking()) {
            mouse_click(BTN_LEFT, delay);
            if (ns > 0) {
                fflush(stdout);
                nssleep(ns);
            }
        }
        else {
            nssleep(100000000);
        }
    }
    
    printf("Mouse thread dying...\n");
    fflush(stdout);
    ioctl(mouse_fd, UI_DEV_DESTROY);
    close(mouse_fd);
    
    return 0;
}
