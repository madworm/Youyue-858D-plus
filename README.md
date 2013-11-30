
Youyue-858D-plus
================

CODE: Custom firmware for my Youyue 858D+ (ATmega168)

Some videos showing the progress from 'stock firmware' with massive temperature overshoot
towards almost no overshoot at all.

https://www.youtube.com/playlist?list=PLONcxJMOrdyeYuEgM6qhCllZelN6gPjrT


The very latest revision contains these features / improvements:

* Almost no overshoot (tunable PD control loop)
* Fan-test on startup
* Error message if wand is not connected or temperature is out of range
* Persistent temperature set-point storage
* Slow / fast temperature set-point change depending on button-press time
* Lower minimum temperature
* Option to blow cold air
* Shows live value of temperature (not just set-point)
* Set-point is shown if temperature is within +- 4°C (suppress +- 1°C noise on display)
* Indicate when temperature is safe to touch the heater


Please note:

Although this device looks very much like ones sold by 'Atten' and others, the innards
are not necessarily the same. The heater / wand are probably the same, but I know that
e.g. the 'Atten 858D' uses a different mainboard with a different brand micro controller.

Naturally, this firmware will only work 'as is' for the exact mcu / mainboard combination I have.
Please see the 'Docs' folder for schematic and PCB photos.


---

Safety information / disclaimer:
================================

Making any modifications to this device may cause you irreversible physical harm or worse.
You do this at your own risk. 

There is a significant risk of lethal electrical shock, so if you still insist of doing so, make sure to
ALWAYS UNPLUG THE MAINS CABLE before dismantling the device. Check repeatedly.

If you have an isolation transformer - do use it.

