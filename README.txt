OpenSegment is a ATmega328 based backpack for larger 7-segment displays. The ATmega receives data over digital interfaces (such as TWI, UART, SPI and buttons) and does all the PWM and upkeep of the display. 

Larger 7-segment displays pull more power than a microcontroller is able to drive directly. OpenSegment has 8 PNP and 4 NPN transistors in order to drive the segments at their maximum brightness.

OpenSegment uses a standard FTDI connection and a bootloader compatible with the Arduino Pro Mini @ 8MHz setting under the Arduino IDE. 

The board runs at 5V but doesn't use an external resonator so the maximum speed is 8MHz.

You will find only the hardware layout in this repo. For the current firmware see the Serial7Segment repo.

http://github.com/sparkfun/Serial7SegmentDisplay