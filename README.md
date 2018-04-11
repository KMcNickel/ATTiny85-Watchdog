# ATTiny85-Watchdog
A simple code for an ATTiny85 based watchdog timer.

Watchdog.ino is an arduino sketch.

WD_vX.c are all codes written in Atmel Studio.
<ul>
<li>v1 is the first version of the code, utilizing the arduino "millis" function (similar to the arduino sketch).
<li>v2 utilizes Timer 0 AND Timer 1 to reduce the frequency of timer interrupts during normal operation.
<li>v3 only utilizes Timer 0 to simplify the program.
</ul>

This code is completely untested and is licensed under the MIT license.
