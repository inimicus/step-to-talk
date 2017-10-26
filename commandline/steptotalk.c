// ============================================================================
// main.c
// ============================================================================
//
// Command line interface for reprogramming of Step-to-Talk USB devices.
// Simply run this utility, with the correct parameters, to reprogram the
// keymapping of the device.
//
// Run 'steptotalk -h' for more information on usage.
//
// Supported Models:
//  - ST01
//  - ST02
//  - ST03
//
// This utility heavily modified and customized from the original Micronucleus
// command line utility, originally written by Ihsan Kehribar
// <ihsan@kehribar.me> and Bluebie <a@creativepony.com> as seen here:
//
// https://github.com/micronucleus/micronucleus/tree/master/commandline
//
// ============================================================================

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "steptotalk_lib.h"
#include "littleWire_util.h"

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char **argv) {

    stepDevice *Step = NULL;
    setbuf(stdout, NULL);

    // Defaults
    uint8_t showKeyMapping  = 0;
    uint8_t setIndex        = 0;
    uint8_t setModifier     = 0;
    uint8_t setScancode     = 0;
    int result              = 0;

    // Parse arguments --------------------------------------------------------
    int arg_pointer = 1;

    while (arg_pointer < argc) {
        if (strcmp(argv[arg_pointer], "--help") == 0 || strcmp(argv[arg_pointer], "-h") == 0) {
            printHelp();
            return EXIT_SUCCESS;
        } else if (strcmp(argv[arg_pointer], "--show") == 0 || strcmp(argv[arg_pointer], "-s") == 0) {
            showKeyMapping = 1;
        } else {
            // Reaching here: Passed CLI arguments should
            // only be modifiers, scancodes, indices
        }

        arg_pointer++;
    }

    // Too few arguments, fail and print usage
    if (argc < 2) {
        puts(STEPTOTALK_USAGE);
        return EXIT_FAILURE;
    }

    // Get and connect to device ----------------------------------------------
    puts("==============================================");
    puts("               Step-to-Talk CLI");
    puts("==============================================");
    printf("\r            Waiting for the device...        ");

    while (Step == NULL) {
        delay(100);
        Step = device_connect();
    }
    printf("\r                 Device found!               ");

    // Perform specified operation --------------------------------------------
    if (showKeyMapping) {

        result = getDeviceInfo(Step);    // Get keys (May have changed since initial connect)
        if (result < 0) {
            printf("Error getting key map (#%d): %s", result, libusb_error_name(result));

            device_close(Step);
            return EXIT_FAILURE;
        } else {
            printKeyMapping(Step);  // Print out
        }

    } else {

        // If an index is specified, grab it
        if (argc == 4) {
            setIndex = atoi(argv[3]);

            // Make sure index within known-allowed range
            if(setIndex > STT_MAX_KEY_INDEX || setIndex < STT_MIN_KEY_INDEX) {
                printf("\r                    ERROR!                  \n");
                printf("              Index incorrect or\n");
                printf("          not within acceptable range.\n");

                device_close(Step);
                return EXIT_FAILURE;
            }
        }

        setModifier = atoi(argv[1]);
        setScancode = atoi(argv[2]);

        printf("\r               Updating Key #%d              \n", setIndex);
        printf("               Modifier:  0x%02X\n", setModifier);
        printf("               Scancode:  0x%02X\n", setScancode);

        int result = updateKeyMapping(Step, setIndex, setModifier, setScancode);
        if (result < 0) {
            printf("Error updating key map (#%d): %s", result, libusb_error_name(result));
            device_close(Step);
            return EXIT_FAILURE;
        } else {
            puts("");
            puts("                    DONE");
            puts("==============================================");
        }

    }

    device_close(Step);
    return EXIT_SUCCESS;
}

