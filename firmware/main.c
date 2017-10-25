// ============================================================================
// main.c
// ============================================================================
//
// Simple configurable keyboard for ATTiny devices.
//
// Adapted from work by Daniel Thompson:
// Main functions for RFStompbox, a firmware for a USB guitar pedal based
// on vusb.
//
// Copyright (C) 2011 Daniel Thompson <daniel@redfelineninja.org.uk>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// ============================================================================

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>

#include "usbdrv.h"
#include "oddebug.h"
#include "osccal.h"

// ----------------------------------------------------------------------------
// IO SETUP
// ----------------------------------------------------------------------------

#define IO_PORT         PORTB           // IO Register
#define IO_PINS         PINB            // Inputs
#define IO_SW           PB1             // Switch

#define SAVE_EEPROM_OFFSET  12          // Where to saving data in EEPROM

static uchar buttonState        = 0;    // Store button states
static uchar buttonStateChanged = 0;    // Button edge detect

typedef struct {
    uint8_t modifier;
    uint8_t scancode;
} keymap_t;

keymap_t savedKey;

// ----------------------------------------------------------------------------
// USB
// ----------------------------------------------------------------------------

#define STEPTOTALK_GET_KEY   0
#define STEPTOTALK_SET_KEY   1

static uchar    reportBuffer[2];    // Buffer for HID reports
static uchar    idleRate;           // In 4 ms units

// ----------------------------------------------------------------------------
// KEYBOARD MODIFIER KEYS
// ----------------------------------------------------------------------------

#define MOD_NONE   0

// ============================================================================
// USB REPORT DESCRIPTOR
// ============================================================================

PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x05, 0x01,                     // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                     // USAGE (Keyboard)
    0xa1, 0x01,                     // COLLECTION (Application)
    0x05, 0x07,                     //   USAGE_PAGE (Keyboard)
    0x19, 0xe0,                     //   USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7,                     //   USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00,                     //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                     //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                     //   REPORT_SIZE (1)
    0x95, 0x08,                     //   REPORT_COUNT (8)
    0x81, 0x02,                     //   INPUT (Data,Var,Abs)
    0x95, 0x01,                     //   REPORT_COUNT (3)
    0x75, 0x08,                     //   REPORT_SIZE (8)
    0x25, 0x65,                     //   LOGICAL_MAXIMUM (101)
    0x19, 0x00,                     //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                     //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                     //   INPUT (Data,Ary,Abs)
    0xc0                            // END_COLLECTION
};

// ============================================================================
// KEYBOARD ACTIONS
// ============================================================================

static void usbSendScanCode(uchar modifier, uchar key) {
    reportBuffer[0] = modifier;
    reportBuffer[1] = key;

    usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
}

// ----------------------------------------------------------------------------

static void loadKeysFromEeprom() {

    // Wait for EEPROM activity to stop
    eeprom_busy_wait();

    // Read stored values from EEPROM
    eeprom_read_block((void *)&savedKey,    // Pointer to save data
        (const void *)SAVE_EEPROM_OFFSET,   // Location to read from
        2);                                 // Length to read
    
    // If EEPROM byte reads 0xFF, assume empty memory and set to zero
    savedKey.modifier = (savedKey.modifier == 0xFF) ? 0 : savedKey.modifier;
    savedKey.scancode = (savedKey.scancode == 0xFF) ? 0 : savedKey.scancode;
}

// ----------------------------------------------------------------------------

static void saveKeysToEeprom() {
    eeprom_busy_wait();
    eeprom_update_block((void *)&savedKey,  // Pointer to data
        (void *)SAVE_EEPROM_OFFSET,         // Location to update
        2);                                 // Length to update
}

// ============================================================================
// TIMER CONFIGURATION
// ============================================================================

#define TICKS_PER_SECOND      ((F_CPU + 1024) / 2048)
#define TICKS_PER_HUNDREDTH   ((TICKS_PER_SECOND + 50) / 100)
#define TICKS_PER_MILLISECOND ((TICKS_PER_SECOND + 500) / 1000)

#define UTIL_BIN4(x)        (uchar)((0##x & 01000)/64 + (0##x & 0100)/16 + (0##x & 010)/4 + (0##x & 1))
#define UTIL_BIN8(hi, lo)   (uchar)(UTIL_BIN4(hi) * 16 + UTIL_BIN4(lo))

#define timeAfter(x, y) (((schar) (x - y)) > 0)

uchar clockHundredths;
uchar clockMilliseconds;

// ----------------------------------------------------------------------------

static void timerInit(void) {
    // First nibble:  Free running clock, no PWM
    // Second nibble: Prescale by 2048 (~8 ticks per millisecond)
    TCCR1 = UTIL_BIN8(0000, 1100);

    // Synchronous clocking mode
    //PLLCSR &= ~_BV(PCKE);   // clear by default
}

// ----------------------------------------------------------------------------

static void timerPoll(void) {
    static uchar next_hundredth     = TICKS_PER_HUNDREDTH;
    static uchar next_millisecond   = TICKS_PER_MILLISECOND;

    while (timeAfter(TCNT1, next_hundredth)) {
        clockHundredths++;
        next_hundredth += TICKS_PER_HUNDREDTH;
    }

    while (timeAfter(TCNT1, next_millisecond)) {
        clockMilliseconds++;
        next_millisecond += TICKS_PER_MILLISECOND;
    }
}

// ============================================================================
// INPUT POLLING
// ============================================================================

static void buttonPoll() {

    static uchar debounceTimeout;
    uchar tempButtonValue;

    if (timeAfter(clockHundredths, debounceTimeout)) {

        tempButtonValue = bit_is_clear(IO_PINS, IO_SW);

        // Trigger a change if status has changed and the debounce-delay is over,
        // this has good debounce rejection and latency but is subject to
        // false trigger on electrical noise

        if (tempButtonValue != buttonState) {
            buttonState = tempButtonValue;
            buttonStateChanged = 1;
        }

        // Restart debounce timer
        debounceTimeout = clockHundredths + 10;
    }

}

// ============================================================================
// USB DRIVER INTERFACE
// ============================================================================

uchar usbFunctionSetup(uchar data[8]) {

    usbRequest_t *rq    = (void *)data;
    usbMsgPtr           = reportBuffer;

    // CLASS-SPECIFIC REQUEST -------------------------------------------------
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {

        if(rq->bRequest == USBRQ_HID_GET_REPORT) {
            // Only one report type
            // Don't need to look at wValue
            return sizeof(reportBuffer);
        } else if(rq->bRequest == USBRQ_HID_GET_IDLE) {
            usbMsgPtr = &idleRate;
            return 1;
        } else if(rq->bRequest == USBRQ_HID_SET_IDLE) {
            idleRate = rq->wValue.bytes[1];
        }

    // VENDOR-SPECIFIC REQUEST ------------------------------------------------
    } else if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_VENDOR) {

        if(rq->bRequest == STEPTOTALK_GET_KEY) {

            // Get key mapping
            loadKeysFromEeprom(); 

            // Send to host
            usbMsgPtr = (usbMsgPtr_t)&savedKey;
            return sizeof(savedKey);

        } else if(rq->bRequest == STEPTOTALK_SET_KEY) {

            // Get key from request
            savedKey.modifier = rq->wValue.bytes[0];
            savedKey.scancode = rq->wValue.bytes[1];

            // Save
            saveKeysToEeprom();
            return sizeof(savedKey);

        } else {
            // Not understood
        }

    // UNKNOWN REQUEST --------------------------------------------------------
    } else {
        // Nothing
    }

    return 0;
}

// ----------------------------------------------------------------------------

void hadUsbReset(void) {
    cli();
    calibrateOscillator();
    sei();

    // Store the calibrated value in EEPROM if it has changed
    if (eeprom_read_byte(0) != OSCCAL)
        eeprom_write_byte(0, OSCCAL);
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    uchar i;
    uchar calibrationValue;

    // USB SETUP --------------------------------------------------------------

    // Get calibration value from last time
    calibrationValue = eeprom_read_byte(0);
    if(calibrationValue != 0xff){
        OSCCAL = calibrationValue;
    }

    usbInit();

    // Enforce re-enumeration, do this while interrupts are disabled!
    usbDeviceDisconnect();

    // Fake USB disconnect for > 250 ms
    i = 0;
    while(--i) {
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();

    // SYSTEM SETUP -----------------------------------------------------------

    wdt_enable(WDTO_1S);

    // Configure Pull-Ups
    IO_PORT = 0;            // Clear all pull-ups
    IO_PORT = _BV(IO_SW);   // Set switch pull-ups

    timerInit();
    sei();

    // KEY SETUP --------------------------------------------------------------
    
    loadKeysFromEeprom();

    // MAIN LOOP --------------------------------------------------------------

    for(;;) {

        // Do polls
        wdt_reset();
        usbPoll();
        buttonPoll();
        timerPoll();

        // Temporary
        static uchar keyOut = 0;
        uchar modOut = 0;

        // If a button change is detected, send appropriate scan code
        if (buttonStateChanged) {

            if (buttonState) {

                // Press
                keyOut = savedKey.scancode;

                if (savedKey.modifier != MOD_NONE) {
                    modOut |= savedKey.modifier;
                }

            } else {

                // Release, but only if a key was specified
                if (savedKey.scancode == 0) {
                    keyOut = 0;
                } else {
                    keyOut = 0x80 | savedKey.scancode;
                }

            }

            // Reset debounce
            buttonStateChanged = 0;
            usbSendScanCode(modOut, keyOut);
        }

    }

    return 0;
}

