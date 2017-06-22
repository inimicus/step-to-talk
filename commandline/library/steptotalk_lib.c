// ============================================================================
// steptotalk_lib.c
// ============================================================================

#include "steptotalk_lib.h"
#include "littleWire_util.h"

// ============================================================================
// FUNCTIONS
// ============================================================================

stepDevice* device_connect() {

    stepDevice *Step = NULL;
    libusb_device **devs, **dev;

    // Initialize USB
    if (libusb_init(NULL) < 0 || libusb_get_device_list(NULL, &devs) < 0) return NULL;

    for (dev = devs; *dev; dev++) {
        struct libusb_device_descriptor desc;

        // Try to get descriptor, otherwise skip
        if (libusb_get_device_descriptor(*dev, &desc) < 0) continue;

            // Match device
            if (desc.idVendor     == STEPTOTALK_VENDOR_ID
                && desc.idProduct == STEPTOTALK_PRODUCT_ID) {

                    Step = malloc(sizeof(stepDevice));
                    Step->version.major = (desc.bcdDevice >> 8) & 0xFF;
                    Step->version.minor = desc.bcdDevice & 0xFF;

                    if (libusb_open(*dev, &Step->device) < 0) return NULL;

            }
        }

    return Step;
}

// ----------------------------------------------------------------------------

void getDeviceInfo(stepDevice* Step) {

    unsigned char buffer[6];

    int res = libusb_control_transfer(Step->device,           // Device
                                                              // bmRequestType
        LIBUSB_ENDPOINT_IN|LIBUSB_REQUEST_TYPE_VENDOR|LIBUSB_RECIPIENT_DEVICE,
        STEPTOTALK_GET_KEY,                                   // bRequest
        0,                                                    // wValue
        0,                                                    // wIndex
        buffer,                                               // Destination
        6,                                                    // wLength
        STEPTOTALK_USB_TIMEOUT);                              // Timeout

    if (res < 0) {
        printf("\nCould not communicate with the device.");
    } else {
        assert(res >= 6);

        Step->mod1 = buffer[0];
        Step->key1 = buffer[1];
        Step->mod2 = buffer[2];
        Step->key2 = buffer[3];
        Step->mod3 = buffer[4];
        Step->key3 = buffer[5];
    }

}

// ----------------------------------------------------------------------------

void printKeyMapping(stepDevice* Step) {
    printf("\r                              ");
    printf("\rMod 1\tKey 1\tMod 2\tKey 2\tMod 3\tKey 3\n");
    printf("%d\t%d\t%d\t%d\t%d\t%d\n",
            Step->mod1,
            Step->key1,
            Step->mod2,
            Step->key2,
            Step->mod3,
            Step->key3);
}

// ----------------------------------------------------------------------------

void updateKeyMapping(stepDevice* Step, uint8_t index, uint8_t modifier, uint8_t scancode) {

    uint16_t newValue = (scancode << 8) | (modifier & 0xFF);

    libusb_control_transfer(Step->device,                     // Device
                                                              // bmRequestType
        LIBUSB_ENDPOINT_IN|LIBUSB_REQUEST_TYPE_VENDOR|LIBUSB_RECIPIENT_DEVICE,
        STEPTOTALK_SET_KEY,                                   // bRequest
        newValue,                                             // wValue
        index,                                                // wIndex
        NULL,                                                 // Destination
        0,                                                    // wLength
        STEPTOTALK_USB_TIMEOUT);                              // Timeout

}

// ----------------------------------------------------------------------------

void printHelp() {
    puts("====================================================================");
    puts("                        Step-to-Talk CLI Help");
    puts("====================================================================");
    puts(STEPTOTALK_USAGE);
    puts("");
    puts("   --help: This help text");
    puts("       -h: Alias for --help");
    puts("   --show: Get and show current keymapping from device");
    puts("       -s: Alias for --show");
    puts(" modifier: Bitwise flags for modifier key(s) to use. In decimal.");
    puts("");
    puts("           0 0 0 0 0 0 0 0");
    puts("           | | | | | | | +--  Left Control");
    puts("           | | | | | | +----  Left Shift");
    puts("           | | | | | +------  Left Alt");
    puts("           | | | | +--------  Left GUI");
    puts("           | | | +----------  Right Control");
    puts("           | | +------------  Right Shift");
    puts("           | +--------------  Right Alt");
    puts("           +----------------  Right GUI");
    puts("");
    puts("           Examples:");
    puts("           00000011 = 0x03 =  3 = Left Shift + Left Control");
    puts("           01000000 = 0x40 = 64 = Right Alt");
    puts("           00000100 = 0x04 =  4 = Left Alt");
    puts("");
    puts("           Note:");
    puts("           Setting all bits high will have the same result");
    puts("           as setting all bits low. For reasons.");
    puts("");
    puts(" scancode: USB HID keyboard scancode for the key to map. In decimal.");
    puts("    index: Default 0. Which key to program. 0-2 for ST03 models.");
    puts("");
    puts("====================================================================");
    puts(STEPTOTALK_CLI_VERSION);
    puts("====================================================================");
}

