// -----------------------------------------------------------------------------
// main.c
// -----------------------------------------------------------------------------
//
// Simple configurable keyboard for ATTiny devices.
//
// Based on:
// Main functions for RFStompbox, a firmware for a USB guitar pedal based
// on vusb.
//
// Copyright (C) 2011 Daniel Thompson <daniel@redfelineninja.org.uk>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------

#define KEY_SCANCODE    53          // Key: `

#define BUTTON_PORT     PORTB       // PORTx - register for button output
#define BUTTON_PIN      PINB        // PINx - register for button input
#define BUTTON_BIT      PB0         // bit for button input/output

#define LED_BIT         PB1                                 // LED location
#define LED_OFF()       (PORTB &= ~_BV(LED_BIT))
#define LED_ON()        (PORTB |= _BV(LED_BIT))
#define LED_TOGGLE()    (PORTB ^= _BV(LED_BIT))
#define LED_INIT()      (LED_OFF(), DDRB |= _BV(LED_BIT))   // Port direction

#define UTIL_BIN4(x)        (uchar)((0##x & 01000)/64 + (0##x & 0100)/16 + (0##x & 010)/4 + (0##x & 1))
#define UTIL_BIN8(hi, lo)   (uchar)(UTIL_BIN4(hi) * 16 + UTIL_BIN4(lo))

// -----------------------------------------------------------------------------

static uchar    reportBuffer[2];    // Buffer for HID reports
static uchar    idleRate;           // In 4 ms units

static uchar    buttonState;        // Stores state of button
static uchar    buttonStateChanged; // Indicates edge detect on button

// -----------------------------------------------------------------------------

// USB report descriptor
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
    0x95, 0x01,                     //   REPORT_COUNT (1)
    0x75, 0x08,                     //   REPORT_SIZE (8)
    0x25, 0x65,                     //   LOGICAL_MAXIMUM (101)
    0x19, 0x00,                     //   USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65,                     //   USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00,                     //   INPUT (Data,Ary,Abs)
    0xc0                            // END_COLLECTION
};

// We use a simplifed keyboard report descriptor which does not support the
// boot protocol. We don't allow setting status LEDs and we only allow one
// simultaneous key press (except modifiers). We can therefore use short
// 2 byte input reports.
// The report descriptor has been created with usb.org's "HID Descriptor Tool"
// which can be downloaded from http://www.usb.org/developers/hidpage/.
// Redundant entries (such as LOGICAL_MINIMUM and USAGE_PAGE) have been omitted
// for the second INPUT item.

// -----------------------------------------------------------------------------

static void usbSendScanCode(uchar scancode) {
    reportBuffer[0] = 0;
    reportBuffer[1] = scancode;

    usbSetInterrupt(reportBuffer, sizeof(reportBuffer));
}

// -----------------------------------------------------------------------------

#define TICKS_PER_SECOND      ((F_CPU + 1024) / 2048)
#define TICKS_PER_HUNDREDTH   ((TICKS_PER_SECOND + 50) / 100)
#define TICKS_PER_MILLISECOND ((TICKS_PER_SECOND + 500) / 1000)

uchar clockHundredths;
uchar clockMilliseconds;

#define timeAfter(x, y) (((schar) (x - y)) > 0)

static void timerInit(void) {
    // First nibble:  Free running clock, no PWM
    // Second nibble: Prescale by 2048 (~8 ticks per millisecond)
    TCCR1 = UTIL_BIN8(0000, 1100);

    // Synchronous clocking mode
    //PLLCSR &= ~_BV(PCKE);   // clear by default
}

static void timerPoll(void) {
    static uchar next_hundredth = TICKS_PER_HUNDREDTH;
    static uchar next_millisecond = TICKS_PER_MILLISECOND;

    while (timeAfter(TCNT1, next_hundredth)) {
        clockHundredths++;
        next_hundredth += TICKS_PER_HUNDREDTH;
    }

    while (timeAfter(TCNT1, next_millisecond)) {
        clockMilliseconds++;
        next_millisecond += TICKS_PER_MILLISECOND;
    }
}

// -----------------------------------------------------------------------------

static void buttonPoll(void) {
    static uchar debounceTimeIsOver;
    static uchar debounceTimeout;

    uchar tempButtonValue = bit_is_clear(BUTTON_PIN, BUTTON_BIT);

    if (!debounceTimeIsOver) {
        if (timeAfter(clockHundredths, debounceTimeout)) {
            debounceTimeIsOver = 1;
        }
    }

    // Trigger a change if status has changed and the debounce-delay is over,
    // this has good debounce rejection and latency but is subject to
    // false trigger on electrical noise
    if (tempButtonValue != buttonState && debounceTimeIsOver == 1) {

        // Change button state
        buttonState = tempButtonValue;
        buttonStateChanged = 1;

        // Restart debounce timer
        debounceTimeIsOver = 0;
        debounceTimeout = clockHundredths + 5;

    }
}

// -----------------------------------------------------------------------------

static void testPoll(void) {
#if 0
    // Show (in humane units) that the tick rate is correctly calculated
    static uchar ledTimeout;

    if (timeAfter(clockHundredths, ledTimeout)) {
        // Two second duty cycle
        ledTimeout += 100;
        LED_TOGGLE();
    }
#endif
}

// -----------------------------------------------------------------------------
// ------------------------ Interface to USB Driver ----------------------------
// -----------------------------------------------------------------------------

uchar usbFunctionSetup(uchar data[8]) {
    usbRequest_t    *rq = (void *)data;

    usbMsgPtr = reportBuffer;
    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
            /* we only have one report type, so don't look at wValue */
            /* buildReport(); */
            return sizeof(reportBuffer);
        }else if(rq->bRequest == USBRQ_HID_GET_IDLE){
            usbMsgPtr = &idleRate;
            return 1;
        }else if(rq->bRequest == USBRQ_HID_SET_IDLE){
            idleRate = rq->wValue.bytes[1];
        }
    }else{
        /* no vendor specific requests implemented */
    }
    return 0;
}

void hadUsbReset(void) {
    cli();
    calibrateOscillator();
    sei();

    // Store the calibrated value in EEPROM if it has changed
    if (eeprom_read_byte(0) != OSCCAL)
        eeprom_write_byte(0, OSCCAL);
}

// -----------------------------------------------------------------------------
// --------------------------------- MAIN --------------------------------------
// -----------------------------------------------------------------------------

int main(void) {
    uchar i;
    uchar calibrationValue;

    // USB SETUP ---------------------------------------------------------------

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

    // SYSTEM SETUP ------------------------------------------------------------

    wdt_enable(WDTO_1S);
    LED_INIT();

    // Configure Pull-Ups
    BUTTON_PORT = 0;                // Clear all pull-ups
    BUTTON_PORT |= _BV(BUTTON_BIT); // Turn on internal pull-up for switch

    timerInit();
    sei();

    // MAIN LOOP ---------------------------------------------------------------

    for(;;){

        // Do all polls
        wdt_reset();
        usbPoll();
        buttonPoll();
        timerPoll();
        testPoll();

        // If a button change is detected, send appropriate scan code
        if (buttonStateChanged) {

            // Button is Active Low
            if (buttonState) {
                // Push
                usbSendScanCode(KEY_SCANCODE);
                LED_ON();
            } else {
                // Release
                usbSendScanCode(0x80 | KEY_SCANCODE);
                LED_OFF();
            }

            buttonStateChanged = 0;
        }

    }

    return 0;
}

