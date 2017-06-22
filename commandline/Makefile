# Makefile initially writen for Little-Wire by Omer Kilic <omerkilic@gmail.com>
# Later on modified by ihsan Kehribar <ihsan@kehribar.me> for Micronucleus bootloader application.
# Then modified by Garret Jennings <gjennings@me.com> for use in programming
# Step-to-Talk, a user-programmable USB keyboard based on V-USB.

CC=gcc

ifeq ($(shell uname), Linux)
	USBFLAGS=$(shell libusb-config --cflags)
	USBLIBS=$(shell libusb-config --libs)
	EXE_SUFFIX =
	OSFLAG = -D LINUX
else ifeq ($(shell uname), Darwin)
	# libusb-config points to the old and wrong libusb-compat version
	# Manually specify the correct path unless libusb-config correctly
	# points to libusb-1.0
	#USBFLAGS=$(shell libusb-config --cflags || libusb-legacy-config --cflags)
	USBFLAGS= -I /usr/local/Cellar/libusb/1.0.21/include/libusb-1.0/
	#USBLIBS=$(shell libusb-config --libs || libusb-legacy-config --libs)
	USBLIBS= -L /usr/local/Cellar/libusb/1.0.21/lib/ -lusb-1.0
	EXE_SUFFIX =
	OSFLAG = -D MAC_OS
else ifeq ($(shell uname), OpenBSD)
	USBFLAGS=$(shell libusb-config --cflags || libusb-legacy-config --cflags)
	USBLIBS=$(shell libusb-config --libs || libusb-legacy-config --libs)
	EXE_SUFFIX =
	OSFLAG = -D OPENBSD
else ifeq ($(shell uname), FreeBSD)
	USBFLAGS=
	USBLIBS= -lusb-1.0
	EXE_SUFFIX =
	OSFLAG = -D OPENBSD
else
	USBFLAGS = -I C:\MinGW\include
	USBLIBS = -L C:\MinGW\lib -lusb-1.0
	EXE_SUFFIX = .exe
	OSFLAG = -D WIN
endif

LIBS    = $(USBLIBS)
INCLUDE = library
CFLAGS  = $(USBFLAGS) -I$(INCLUDE) -O -g $(OSFLAG)

LWLIBS = steptotalk_lib littleWire_util
EXAMPLES = steptotalk

.PHONY:	clean library

all: library $(EXAMPLES)

library: $(LWLIBS)

$(LWLIBS):
	@echo Building library: $@...
	$(CC) $(CFLAGS) -c library/$@.c

$(EXAMPLES): $(addsuffix .o, $(LWLIBS))
	@echo Building command line tool: $@...
	$(CC) $(CFLAGS) $(LIBS) -o $@$(EXE_SUFFIX) $@.c $^ $(LIBS)

clean:
	rm -f $(EXAMPLES)$(EXE_SUFFIX) *.o *.exe
	rm -rf *.dSYM
