#include "fake_mouse.h"

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <linux/uinput.h>

void init_mouse() {
    int mouse_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
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
    ioctl(mouse_fd, UI_SET_KEYBIT, BTN_SIDE);
    ioctl(mouse_fd, UI_SET_KEYBIT, BTN_EXTRA);
    
    // Allow relative movement
    ioctl(mouse_fd, UI_SET_EVBIT, EV_REL);
    ioctl(mouse_fd, UI_SET_RELBIT, REL_X);
    ioctl(mouse_fd, UI_SET_RELBIT, REL_Y);
    ioctl(mouse_fd, UI_SET_RELBIT, REL_WHEEL);
    ioctl(mouse_fd, UI_SET_RELBIT, REL_HWHEEL);
    
    // Allow absolute movement
    ioctl(mouse_fd, UI_SET_EVBIT, EV_ABS);
    ioctl(mouse_fd, UI_SET_ABSBIT, ABS_X);
    ioctl(mouse_fd, UI_SET_ABSBIT, ABS_Y);
    
    // Setup device params
    struct uinput_setup usetup;
    
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x4172;
    usetup.id.product = 0x7469;
    usetup.id.version = 1;
    strcpy(usetup.name, "Mouse Raper 9000");
    ioctl(mouse_fd, UI_DEV_SETUP, &usetup);
    
    // Actually create device!
    ioctl(mouse_fd, UI_DEV_CREATE);
    
    static char sys_name[256];
    ioctl(mouse_fd, UI_GET_SYSNAME(50), sys_name);
    printf("Created mouse input device: %s\n", sys_name);        
}