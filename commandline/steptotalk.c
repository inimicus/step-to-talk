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

#define CONNECT_WAIT 250 /* milliseconds to wait after detecting device on usb bus - probably excessive */

// ----------------------------------------------------------------------------
// KEYBOARD MODIFIER KEYS
// ----------------------------------------------------------------------------

#define MOD_L_CTRL      0
#define MOD_L_SHIFT     1
#define MOD_L_ALT       2
#define MOD_L_GUI       3
#define MOD_R_CRTL      4
#define MOD_R_SHIFT     5
#define MOD_R_ALT       6
#define MOD_R_GUI       7
#define MOD_NONE        8

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
    printf("> Mod 1: %d\t(0x%02X)\n", nucleus->mod1, nucleus->mod1);
    printf("> Key 1: %d\t(0x%02X)\n", nucleus->key1, nucleus->key1);
    printf("> Mod 2: %d\t(0x%02X)\n", nucleus->mod2, nucleus->mod2);
    printf("> Key 2: %d\t(0x%02X)\n", nucleus->key2, nucleus->key2);
    printf("> Mod 3: %d\t(0x%02X)\n", nucleus->mod3, nucleus->mod3);
    printf("> Key 3: %d\t(0x%02X)\n", nucleus->key3, nucleus->key3);

}

void updateKeyMapping(micronucleus* nucleus, uint8_t index, uint8_t modifier, uint8_t scancode) {

    uint16_t newValue = (scancode << 8) | (modifier & 0xFF);

    int res = usb_control_msg(nucleus->device,
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

    // parse arguments
    int run = 0;
    int arg_pointer = 1;

    printf("> Please plug in the device ... \n");
    printf("> Press CTRL+C to terminate the program.\n");

    while (my_device == NULL) {
        delay(100);
        my_device = micronucleus_connect();
    }

    printf("> Device is found!\n");

    //int res = usb_control_msg(my_device->device,
    //        USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE, // bmRequestType
    //        1, // bRequest
    //        0x04, // wValue
    //        4, // wIndex
    //        NULL, // Pointer to destination buffer
    //        0, // wLength
    //        MICRONUCLEUS_USB_TIMEOUT); // Timeout

    //// Device descriptor was found, but talking to it was not succesful. This can happen when the device is being reset.
    //if (res < 0) {
    //    return EXIT_FAILURE;  
    //}

    //printf("> Device has firmware version %d.%d\n", my_device->version.major, my_device->version.minor);
    //printf("> Mod 1: %d\t(0x%02X)\n", my_device->mod1, my_device->mod1);
    //printf("> Mod 2: %d\t(0x%02X)\n", my_device->mod2, my_device->mod2);
    //printf("> Mod 3: %d\t(0x%02X)\n", my_device->mod3, my_device->mod3);
    //printf("> Key 1: %d\t(0x%02X)\n", my_device->key1, my_device->key1);
    //printf("> Key 2: %d\t(0x%02X)\n", my_device->key2, my_device->key2);
    //printf("> Key 3: %d\t(0x%02X)\n", my_device->key3, my_device->key3);
    
    getKeyMapping(my_device);
    //updateKeyMapping(my_device, 0, 0, 0x04);
    //updateKeyMapping(my_device, 1, 0, 0x05);
    //updateKeyMapping(my_device, 2, 0, 0x06);

    //getKeyMapping(my_device);
    return EXIT_SUCCESS;
}

