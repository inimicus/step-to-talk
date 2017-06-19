// ============================================================================
// main.c
// ============================================================================
//
// ============================================================================

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "micronucleus_lib.h"
#include "littleWire_util.h"

#define CONNECT_WAIT 250    // Wait time after detecting device on USB

// ----------------------------------------------------------------------------
// KEYBOARD MODIFIER KEYS
// ----------------------------------------------------------------------------

#define MOD_L_CTRL      1
#define MOD_L_SHIFT     2
#define MOD_L_ALT       3
#define MOD_L_GUI       4
#define MOD_R_CRTL      5
#define MOD_R_SHIFT     6
#define MOD_R_ALT       7
#define MOD_R_GUI       8
#define MOD_NONE        0

// Current min/max number of keys for STT devices
#define STT_MIN_KEY_INDEX   0
#define STT_MAX_KEY_INDEX   2

// ----------------------------------------------------------------------------
// USB
// ----------------------------------------------------------------------------

#define STEPTOTALK_GET_KEY   0
#define STEPTOTALK_SET_KEY   1

void getKeyMapping(micronucleus* nucleus) {

    unsigned char buffer[6];

    int res = usb_control_msg(nucleus->device,
        USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE, // bmRequestType
        STEPTOTALK_GET_KEY, // bRequest
        0, // wValue
        0, // wIndex
        (char *)buffer, // Pointer to destination buffer
        6, // wLength
        MICRONUCLEUS_USB_TIMEOUT); // Timeout

    //printf("> Device has firmware version %d.%d\n", my_device->version.major, my_device->version.minor);
    if (res < 0) {
        printf("> Could not communicate with the device.");
    } else {
        printf("> Mod 1: %d\t(0x%02X)\n", nucleus->mod1, nucleus->mod1);
        printf("> Key 1: %d\t(0x%02X)\n", nucleus->key1, nucleus->key1);
        printf("> Mod 2: %d\t(0x%02X)\n", nucleus->mod2, nucleus->mod2);
        printf("> Key 2: %d\t(0x%02X)\n", nucleus->key2, nucleus->key2);
        printf("> Mod 3: %d\t(0x%02X)\n", nucleus->mod3, nucleus->mod3);
        printf("> Key 3: %d\t(0x%02X)\n", nucleus->key3, nucleus->key3);
    }

}

void updateKeyMapping(micronucleus* nucleus, uint8_t index, uint8_t modifier, uint8_t scancode) {

    uint16_t newValue = (scancode << 8) | (modifier & 0xFF);

    usb_control_msg(nucleus->device,
        USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE, // bmRequestType
        STEPTOTALK_SET_KEY, // bRequest
        newValue, // wValue
        index, // wIndex
        NULL, // Pointer to destination buffer
        0, // wLength
        MICRONUCLEUS_USB_TIMEOUT); // Timeout

}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char **argv) {
    micronucleus *my_device = NULL;

    // Defaults
    uint8_t showKeyMapping  = 0;
    uint8_t setIndex        = 0;
    uint8_t setModifier     = 0;
    uint8_t setScancode     = 0;

    // Parse arguments
    int arg_pointer = 1;
    char* usage = "Usage: steptotalk [--help] [--show] [modifier scancode [index]]";

    while (arg_pointer < argc) {
        if (strcmp(argv[arg_pointer], "--help") == 0 || strcmp(argv[arg_pointer], "-h") == 0) {
            puts(usage);
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
            return EXIT_SUCCESS;
        } else if (strcmp(argv[arg_pointer], "--show") == 0 || strcmp(argv[arg_pointer], "-s") == 0) {
            showKeyMapping = 1;
        } else {
            //
        }

        arg_pointer++;
    }

    // Too few arguments, print usage
    if (argc < 2) {
        puts(usage);
        return EXIT_FAILURE;
    }

    printf("> Waiting for the device...\n");

    while (my_device == NULL) {
        delay(100);
        my_device = micronucleus_connect();
    }

    printf("> Device found!\n");

    // Show keys if asked, otherwise set them
    if (showKeyMapping) {
        getKeyMapping(my_device);
    } else {

        // If an index is specified, grab it
        if (argc == 4) {
            setIndex = atoi(argv[3]);

            if(setIndex > STT_MAX_KEY_INDEX || setIndex < STT_MIN_KEY_INDEX) {
                // Index out of acceptable range
                printf("Error: index incorrect or not within acceptable range.");
                return EXIT_FAILURE;
            }
        }

        setModifier = atoi(argv[1]);
        setScancode = atoi(argv[2]);

        printf("> Setting Key at index %d:\n", setIndex);
        printf("> Modifier: %d\t(0x%02X)\n", setModifier, setModifier);
        printf("> Scancode: %d\t(0x%02X)\n", setScancode, setScancode);

        updateKeyMapping(my_device, setIndex, setModifier, setScancode);
        printf("Updated!");

    }

    return EXIT_SUCCESS;
}

