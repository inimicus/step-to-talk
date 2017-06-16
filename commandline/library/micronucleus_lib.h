// ============================================================================
//
// ============================================================================

// ----------------------------------------------------------------------------
#ifndef MICRONUCLEUS_LIB_H
#define MICRONUCLEUS_LIB_H

/********************************************************************************
* Header files
********************************************************************************/
// this is libusb, see http://libusb.sourceforge.net/
#if defined WIN
  #include <lusb0_usb.h>    
#else
  #include <usb.h>
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/********************************************************************************
* USB details
********************************************************************************/
#define STEPTOTALK_VENDOR_ID     0x4242
#define STEPTOTALK_PRODUCT_ID    0xe131
#define MICRONUCLEUS_USB_TIMEOUT 0xFFFF

/********************************************************************************
* Declarations
********************************************************************************/
//typedef usb_dev_handle micronucleus;
// representing version number of micronucleus device
typedef struct _micronucleus_version {
  unsigned char major;
  unsigned char minor;
} micronucleus_version;

#define MICRONUCLEUS_COMMANDLINE_VERSION "Commandline tool version: 2.0a5"

// handle representing one micronucleus device
typedef struct _micronucleus {
  usb_dev_handle *device;
  // general information about device
  micronucleus_version version;
  uint8_t mod1;
  uint8_t mod2;
  uint8_t mod3;
  uint8_t key1;
  uint8_t key2;
  uint8_t key3;
} micronucleus;

typedef void (*micronucleus_callback)(float progress);

typedef struct {
    uint8_t modifier;
    uint8_t scancode;
} keymap_t;

/********************************************************************************
* Try to connect to the device
*     Returns: device handle for success, NULL for fail
********************************************************************************/
micronucleus* micronucleus_connect();

#endif
