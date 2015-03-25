# MicroView
Applications for the MicroView Arduino clone

The [MicroView](https://www.sparkfun.com/products/12923) is a little Arduino compatible
module with a small on-board display.

This repository contains some toy projects written while exploring the MicroView.

## UsbStats

This rotates through several screen, each one display some information of interest (YMMV).

There are two built-in screens:

 * Uptime. How long the MicroView has been running, in days, hours miutes and seconds.
 * Internal temperature. Connects the internal ADC to the embedded temperature sensor 
(which is horribly inaccurate without calibration, by the way) and display the sensor
voltage and estimated corresponding temperature. 

Additional screens can be added on the fly over the serial interface (or over USB via the
MicroView programming adapter). The accompanying "UsbStats" Python program reads information
of interest from the host system and sends the formatted data to the MicroView, where it gets 
added as an additional screen in the rotation.

The version here adds two screens, for host disk usage and host uptime. I run this on a
Raspberry PI B which I use as a camera capture store for a wireless camera that watches
the feral cats in our back yard.

The host to MicroView protocol is very simple and should be easy enough to reverse engineer
from the code if you want to add anything.

## bootloader

Some of the first batch of MicroViews were shipped out [without the necessary 
bootloader code](https://learn.sparkfun.com/tutorials/installing-a-bootloader-on-the-microview)
in their on-board flash. The manufacturers made good by sending replacements, and
provided instructions on 
[how to install a bootloader](https://learn.sparkfun.com/tutorials/installing-a-bootloader-on-the-microview)
on the defective units, with a hex file for the bootloader code and pointers to the
necessary application.

Unfortunately, the process described there requires either a special programmer, or another Arduino.
Well, how about the replacement MicroView? Isn't that an Arduino? Well, yes it is, but
to use it following the official method requires use of the SPI interface, and on the MicroView
the SPI is hidden away internally and used to drive the display.

But you don't need the chip's built-in hardware SPI interface to do SPI: you can emulate it all
in software by waggling GPIO lines up and down. It's not as fast as in hardware but it doesn't
need to be.

It took me a while to get round to it as I was busy with other things, but eventually I
managed to use my live MicroView to give the kiss of life to my dead one.

The code has three parts:

 * The hex file for the bootloader code that needs to go into the MicroView's flash. That
comes from the MicroView web site.
 * An Arduino program which implements a simple bit-banger SPI interface implemented using
GPIO lines, and driven by a simple protocol over the serial lines (which in turn are driven,
via the MicroView programming adapter, from USB.)
 * A Python program which reads and parses the .hex file (it's in 
[Intel HEX format](https://en.wikipedia.org/wiki/Intel_HEX), by the way) and tells
the bit-banger what bytes to write on the bit-banged SPI.

The electrical connections to be made on the rescue MicroView are detailed on
[this page](https://learn.sparkfun.com/tutorials/installing-a-bootloader-on-the-microview).
Soldering the three wires is a bit fiddly, but not too bad with a fine pointed
soldering iron.
On the server MicroView the pins are as listed in the Arduino code (spislave.ino).
I found that a single USB programming adapter can power both MicroViews, one plugged
into the adapter, and then the adapter plugged into a breadboard along with the
second MicroView and power and ground rails wired from the former to the latter.

The code is not optimized and has development cruft left over. I only ever needed it
to work just once, and after it had I did not clean it up.

