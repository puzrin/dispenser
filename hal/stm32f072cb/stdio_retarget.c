#include "usb_device.h"
#include "usbd_cdc_if.h"


void stdio_retarget_init()
{
    MX_USB_DEVICE_Init();

    //setvbuf(stdout, NULL, _IONBF, 0);
}


int _write(int fd, char* ptr, int len)
{
    CDC_Transmit_FS((uint8_t*)ptr, len);
    return len;
}

int _isatty(int fd) {
    (void)fd;
    return 1;
}

int _close(int fd) {
    (void)fd;
    return -1;
}

int _lseek(int fd, int ptr, int dir) {
    (void) fd; (void) ptr; (void) dir;
    return -1;
}

int _read(int fd, char* ptr, int len) {
    (void)fd; (void)ptr; (void)len;
    return -1;
}

#include <sys/stat.h>

int _fstat(int fd, struct stat* st) {
    st->st_mode = S_IFCHR;
    return 0;
}
