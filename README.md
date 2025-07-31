# BlueXCSoarRemote

A compact Bluetooth remote stick for XCSoar based around an ESP32-C3 Supermini.
Sends keystrokes via Bluetooth so only 5V power needs to be supplied to the Remote.

* 5-way navigation joystick
* three top buttons around the joystick
* PTT button at the bottom left.
* On the front of the stick is provision for another two buttons that can be either connected to the remote board or wired direct to other instruments.

In my setup I use the lower button wired direct to an S100 for Thermal/STF mode switching. The upper button is reserved for future use if I need it.

The keys respond to Click, Double-Click and Long-Press separately, enabling a large number of keystrokes to be sent to XCSoar. In the default cnfiguration the top two buttons use the different clicks, the lower left button is ESC on all three modes.

The head is designed to sit on the top of an existing grip. I don't really like the "ergonomic" control grips, preferring a simple round firm foam grip. I designed this specifically to fit a Galsflugel 19mm dia stick with an ESI Extra Chunky MTB grip on it, but it will likely work OK with many other simple grips.
