#include <errno.h>
#include <fcntl.h>
#include <libudev.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum result_t
{
    OK,
    UDEV_NEW_ERR,
    UDEV_ENUMERATE_ERR,
    UDEV_DEVICE_LIST_ERR,
    OPEN_DEVICE_DESCRIPTOR_ERR,
    BAD_FILE_DESCRIPTOR,
    DEVICE_NOT_FOUND_ERR,

    RESULT_LENGHT
} result_t;

typedef struct result_msg_t
{
    const char* msg;
} result_msg_t;

const result_msg_t result_msg[RESULT_LENGHT] = {{"Error while creating new udev"},
                                                {"Error while creating udev monitor"},
                                                {"Error while getting device list"},
                                                {"Error while opening device descriptor"},
                                                {"Bad file descriptor"},
                                                {"Device not found"}};

#define log_fatal(res)                                                                             \
    {                                                                                              \
        printf("%s. %s\n", result_msg[res - 1].msg, errno ? strerror(errno) : "");                 \
        quick_exit(res);                                                                           \
    }

void hid_open(unsigned short vendor_id, unsigned short product_id)
{
    struct udev* udev;
    struct udev_device* dev;
    struct udev_enumerate* enumerate;
    struct udev_list_entry* devices;
    struct udev_list_entry* dev_list_entry;

    udev = udev_new();
    if(!udev) {
        log_fatal(UDEV_NEW_ERR);
    }

    // Create udev monitor
    enumerate = udev_enumerate_new(udev);
    if(!enumerate) {
        log_fatal(UDEV_ENUMERATE_ERR);
    }

    udev_enumerate_add_match_subsystem(enumerate, "hidraw");
    udev_enumerate_scan_devices(enumerate);
    // Create and fill up device list in "hidraw" subsystem
    devices = udev_enumerate_get_list_entry(enumerate);
    if(!devices) {
        log_fatal(UDEV_DEVICE_LIST_ERR);
    }

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
            int ret = sscanf(hid_start + 10, "%hx:%hx", &dev_vid, &dev_pid);

            if(ret == 2 && dev_vid == vendor_id && dev_pid == product_id) {
                dev = udev_device_new_from_syspath(udev, dev_path);
                const char* dev_node_path = udev_device_get_devnode(dev);

                printf("dev_path: %s\n", dev_path);
                printf("dev_node_path: %s\n", dev_node_path);
                printf("Vendor ID: %hx Product ID: %hx\n", dev_vid, dev_pid);

                // Open
                int fd = open(dev_node_path, O_RDWR | O_CLOEXEC);
                if(fd < 0) {
                    log_fatal(OPEN_DEVICE_DESCRIPTOR_ERR);
                }

                char buf[16];
                memset(buf, 0x0, sizeof(buf));

                // Read
                while(1) {
                    int res = read(fd, buf, sizeof(buf));
                    if(res < 0) {
                        if(errno == EAGAIN || errno == EINPROGRESS) {
                            res = 0;
                        } else {
                            log_fatal(BAD_FILE_DESCRIPTOR);
                        }
                    } else {
                        int bytes_sum = 0;
                        for(int i = 0; i < res; ++i) {
                            bytes_sum += buf[i];
                        }
                        printf("Button ID: %x\n", bytes_sum);
                    }
                }
                close(fd);
                // free dev
                udev_device_unref(dev);

            } else {
                log_fatal(DEVICE_NOT_FOUND_ERR);
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
    hid_open(0x046d, 0xb030);
    return 0;
}
