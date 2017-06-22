// ============================================================================
// steptotalk_lib.h
// ============================================================================

#ifndef STEPTOTALK_LIB_H
#define STEPTOTALK_LIB_H

// ============================================================================
// Header files
// ============================================================================

#include <libusb.h> // See http://libusb.sourceforge.net/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// PROGRAM INFO
// ============================================================================

#define STEPTOTALK_CLI_VERSION "Step-to-Talk CLI Tool Version: 1.0"
#define STEPTOTALK_USAGE "Usage: steptotalk [--help] [--show] [modifier scancode [index]]"

#define CONNECT_WAIT 250        // Wait time after detecting device on USB

// ============================================================================
// DEVICE DETAILS
// ============================================================================

#define STEPTOTALK_VENDOR_ID    0x4242
#define STEPTOTALK_PRODUCT_ID   0xe131
#define STEPTOTALK_USB_TIMEOUT  0xFFFF

// ----------------------------------------------------------------------------
// USB CONTROL CODES
// ----------------------------------------------------------------------------

#define STEPTOTALK_GET_KEY   0
#define STEPTOTALK_SET_KEY   1

// ----------------------------------------------------------------------------
// DEVICE PARAMETERS
// ----------------------------------------------------------------------------

// Current min/max number of keys for STT devices
#define STT_MIN_KEY_INDEX   0
#define STT_MAX_KEY_INDEX   2

// ============================================================================
// Declarations
// ============================================================================

typedef struct {
    unsigned char major;
    unsigned char minor;
} stt_version;

// ----------------------------------------------------------------------------

typedef struct {
    libusb_device_handle *device;
    stt_version version;
    uint8_t mod1;
    uint8_t mod2;
    uint8_t mod3;
    uint8_t key1;
    uint8_t key2;
    uint8_t key3;
} stepDevice;

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================

// ----------------------------------------------------------------------------
// Function:    device_connect
// Description: Find and connect to a Step-to-Talk device.
// Arguments:   None
// Returns:     Device pointer
// ----------------------------------------------------------------------------
stepDevice* device_connect();

// ----------------------------------------------------------------------------
// Function:    printHelp
// Description: Prints help message.
// Arguments:   None
// Returns:     Nothing
// ----------------------------------------------------------------------------
void printHelp();

// ----------------------------------------------------------------------------
// Function:    getDeviceInfo
// Description: Retrieves information from device, including USB descriptors,
//              device version, and key mapping.
// Arguments:   None
// Returns:     Nothing
// ----------------------------------------------------------------------------
int getDeviceInfo(stepDevice* step);

// ----------------------------------------------------------------------------
// Function:    printKeyMapping
// Description: Outputs lightly formatted key mapping.
// Arguments:   None
// Returns:     Nothing
// ----------------------------------------------------------------------------
void printKeyMapping(stepDevice* step);

// ----------------------------------------------------------------------------
// Function:    updateKeyMapping
// Description: Sends data for one key to update device mapping and save
//              to EEPROM.  Operates on one key at a time due to USB control
//              endpoints being limited to two-byte messages and the desire
//              to keep things simple.
// Arguments:   stepDevice* Step: Pointer to STT device
//                 uint8_t index: Index of key to assign
//              uint8_t modifier: Bitwise modifier key assignment
//              uint8_t scancode: Scancode of the key to assign
// Returns:     Nothing
// ----------------------------------------------------------------------------
int updateKeyMapping(stepDevice* Step, uint8_t index, uint8_t modifier, uint8_t scancode);

#endif
