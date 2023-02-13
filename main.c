#include <libudev.h>
#include <stdio.h>
#include <string.h>

void hid_open(unsigned short vendor_id, unsigned short product_id)
{
    struct udev* udev;
    struct udev_device* dev;
    struct udev_enumerate* enumerate;
    struct udev_list_entry* devices;
    struct udev_list_entry* dev_list_entry;
    //char device[128];

    udev = udev_new();
    if(!udev) {
        printf("Error while createing new udev\n");
        // TODO: exit;
    }

    // Create udev monitor
    enumerate = udev_enumerate_new(udev);
    if(!enumerate) {
        printf("Error while creating udev monitor\n");
        // TODO: exit;
    }

    udev_enumerate_add_match_subsystem(enumerate, "hidraw");
    udev_enumerate_scan_devices(enumerate);
    // Create and fill up device list in "hidraw" subsystem
    devices = udev_enumerate_get_list_entry(enumerate);
    if(!devices) {
        printf("Error while filling up device list\n");
        // TODO: exit;
    }

    // Iterate over each item and find needed PID & VID
    udev_list_entry_foreach(dev_list_entry, devices)
    {
        unsigned short dev_vid = 0;
        unsigned short dev_pid = 0;
        const char* path;

        path = udev_list_entry_get_name(dev_list_entry);

        // 'path' format is something like this:
        // /sys/devices/virtual/misc/uhid/0005:046D:B030.000D/hidraw/hidraw1
        // so let's find the 'uhind' dir
        const char* hid_start = strstr(path, "uhid");
        if(hid_start) {
            // and skip 10 bytes (uhid/0005:) with dir name and device bus
            // read the string staring from the device vendor ID.
            int ret = sscanf(hid_start + 10, "%hx:%hx", &dev_vid, &dev_pid);

            if(ret == 2 && dev_vid == vendor_id && dev_pid == product_id) {
                dev = udev_device_new_from_syspath(udev, path);
                printf("dev_path: %s\n", udev_device_get_syspath(dev));
                printf("Product ID: %hx Vendor ID: %hx\n", dev_vid, dev_pid);
                // free dev
                udev_device_unref(dev);
            } else {
                printf("Device not found\n");
                // TODO: exit;
            }
        }
    }

    // clean up
    udev_enumerate_unref(enumerate);
    udev_unref(udev);
}

int main()
{
    printf("Hello, World!\n");
    hid_open(0x46d, 0xb030);
    return 0;
}
