
/***************************************************************/
/* See the micronucleus_lib.h for the function descriptions/comments */
/***************************************************************/
#include "micronucleus_lib.h"
#include "littleWire_util.h"

micronucleus* micronucleus_connect() {
  micronucleus *nucleus = NULL;
  struct usb_bus *busses;

  // intialise usb and find micronucleus device
  usb_init();
  usb_find_busses();
  usb_find_devices();

  busses = usb_get_busses();
  struct usb_bus *bus;
  for (bus = busses; bus; bus = bus->next) {
    struct usb_device *dev;

    for (dev = bus->devices; dev; dev = dev->next) {
      /* Check if this device is a micronucleus */
      if (dev->descriptor.idVendor == STEPTOTALK_VENDOR_ID && dev->descriptor.idProduct == STEPTOTALK_PRODUCT_ID)  {
        nucleus = malloc(sizeof(micronucleus));
        nucleus->version.major = (dev->descriptor.bcdDevice >> 8) & 0xFF;
        nucleus->version.minor = dev->descriptor.bcdDevice & 0xFF;

        nucleus->device = usb_open(dev);

        // get nucleus info
        unsigned char buffer[6];
        int res = usb_control_msg(nucleus->device,
            USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE, // bmRequestType
            0, // bRequest
            0, // wValue
            0, // wIndex
            (char *)buffer, // Pointer to destination buffer
            6, // wLength
            MICRONUCLEUS_USB_TIMEOUT); // Timeout

        if (res<0) return NULL;  
        
        assert(res >= 6);

        nucleus->mod1 = buffer[0];
        nucleus->key1 = buffer[1];
        nucleus->mod2 = buffer[2];
        nucleus->key2 = buffer[3];         
        nucleus->mod3 = buffer[4];
        nucleus->key3 = buffer[5];           

      }
    }
  }

  return nucleus;
}


