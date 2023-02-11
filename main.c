#include <libudev.h>
#include <stdio.h>

void hid_open(unsigned short vendor_id, unsigned short product_id)
{
    struct udev* udev;
    struct udev_enumerate* enumerate;
    struct udev_list_entry* devices;
    struct udev_list_entry* dev_list_entry;
    char device[128];

    udev = udev_new();
    if(!udev) {
        printf("Error\n");
        // TODO: exit;
    }

    enumerate = udev_enumerate_new(udev);
    if(!enumerate) {
        printf("Error\n");
        // TODO: exit;
    }
}

int main()
{
    printf("Hello, World!\n");
    return 0;
}
