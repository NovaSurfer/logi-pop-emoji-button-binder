#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#include <errno.h>
#include <fcntl.h>
#include <libudev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define log_error(...)                                                                             \
    {                                                                                              \
        printf("%s. %s\n", __VA_ARGS__, errno ? strerror(errno) : "");                             \
    }

static void x11_press_key(Display* disp, Window* root, KeySym keysym, KeySym modsym)
{
    int state;
    XGetInputFocus(disp, root, &state);

    long event_mask = KeyPressMask | KeyReleaseMask;
    XSelectInput(disp, *root, event_mask);

    KeyCode keycode;
    keycode = XKeysymToKeycode(disp, keysym);
    if(keycode == 0) {
        return;
    }

    XTestGrabControl(disp, True);

    // Generate modkey press
    KeyCode modcode;
    if(modsym != 0) {
        modcode = XKeysymToKeycode(disp, modsym);
        XTestFakeKeyEvent(disp, modcode, True, 0);
    }
    // Generate regular key press and release
    XTestFakeKeyEvent(disp, keycode, True, 0);
    XTestFakeKeyEvent(disp, keycode, False, 0);

    // Generate modkey release
    if(modsym != 0) {
        XTestFakeKeyEvent(disp, modcode, False, 0);
    }

    XSync(disp, False);
    XTestGrabControl(disp, False);
}

void hid_open(struct udev* udev, struct udev_list_entry* devices, unsigned short vendor_id,
              unsigned short product_id)
{
    struct udev_list_entry* dev_list_entry;

    // Iterate over each item and find needed PID & VID
    udev_list_entry_foreach(dev_list_entry, devices)
    {
        unsigned short dev_vid = 0;
        unsigned short dev_pid = 0;

        const char* dev_path = udev_list_entry_get_name(dev_list_entry);

        // 'path' format is something like this:
        // /sys/devices/virtual/misc/uhid/0005:046D:B030.000D/hidraw/hidraw1
        // so let's find the 'uhid' dir
        const char* hid_start = strstr(dev_path, "uhid");
        if(hid_start) {
            // and skip 10 bytes (uhid/0005:) with dir name and device bus
            // read the string staring from the device vendor ID.
            const int ret = sscanf(hid_start + 10, "%hx:%hx", &dev_vid, &dev_pid);

            if(ret == 2 && dev_vid == vendor_id && dev_pid == product_id) {
                struct udev_device* dev = udev_device_new_from_syspath(udev, dev_path);
                const char* dev_node_path = udev_device_get_devnode(dev);

                printf("dev_path: %s\n", dev_path);
                printf("dev_node_path: %s\n", dev_node_path);
                printf("Vendor ID: %hx Product ID: %hx\n", dev_vid, dev_pid);

                // Open
                int fd = open(dev_node_path, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
                if(fd < 0) {
                    log_error("Error while getting device desctiptor");
                }

                char buf[2];
                memset(buf, 0x0, sizeof(buf));
                Display* display = XOpenDisplay(NULL);
                Window root = XDefaultRootWindow(display);

                // Read
                while(1) {
                    int res = read(fd, buf, sizeof(buf));
                    if(res < 0) {
                        if(errno == EAGAIN || errno == EINPROGRESS) {
                            res = 0;
                        } else {
                            log_error("Bad file desctiptor");
                        }
                    } else {
                        // hard coding reading of first 2 bytes
                        if(buf[0] + buf[1] == 34) {
                            printf("Emoji button pressed\n");
                            x11_press_key(display, &root, XK_Left, XK_Alt_L);
                        }
                    }
                }
                // TODO: Get to that part on program termination
                close(fd);
                // free dev
                udev_device_unref(dev);
            } else {
                log_error("Device not found");
            }
        }
    }
}

int main()
{
    printf("Hello, World!\n");

    struct udev* udev = udev_new();
    if(!udev) {
        log_error("Error while creating new udev");
    }

    // Create udev monitor
    struct udev_enumerate* enumerate = udev_enumerate_new(udev);
    if(!enumerate) {
        log_error("Error while creating udev monitor");
    }

    udev_enumerate_add_match_subsystem(enumerate, "hidraw");
    udev_enumerate_scan_devices(enumerate);

    // Create and fill up device list in "hidraw" subsystem
    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    if(!devices) {
        log_error("Error while getting device list");
    } else {
        hid_open(udev, devices, 0x046d, 0xb030);
    }

    // clean up
    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    return 0;
}
