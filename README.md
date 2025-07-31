# BlueXCSoarRemote

## Features
A compact Bluetooth remote stick for XCSoar based around an ESP32-C3 Supermini.
* Sends keystrokes via Bluetooth so only 5V power needs to be supplied to the Remote.
* 5-way navigation joystick
* three top buttons around the joystick
* PTT button at the bottom left.
* On the front of the stick is provision for another two buttons that can be either connected to the remote board or wired direct to other instruments.

In my setup I use the lower button wired direct to an S100 for Thermal/STF mode switching. The upper button is reserved for future use if I need it.

The keys respond to Click, Double-Click and Long-Press separately, enabling a large number of keystrokes to be sent to XCSoar. In the default cnfiguration the top two buttons use the different clicks, the lower left button is ESC on all three modes.

The head is designed to sit on the top of an existing grip. I don't really like the "ergonomic" control grips, preferring a simple round firm foam grip. I designed this specifically to fit a Galsflugel 19mm dia stick with an ESI Extra Chunky MTB grip on it, but it will likely work OK with many other simple grips.

## Software
A couple of libraries are used in this:
* [Button2]<https://github.com/LennartHennigs/Button2>
* BleKeyboard - this is a great library, if it was maintained!

To get a working version pay atteniton to the following!
### In Arduino IDE:
* Install esp32 by Espressif boards version 2.0.11
* Download the [ESP32-BLE-Keyboard v0.3.2-beta]<https://github.com/T-vK/ESP32-BLE-Keyboard/releases/tag/0.3.2-beta> and install via Sketch | Tools | Include Library | Add .ZIP Library
* In your Libraries folder (File | Preferences - Sketchbook Folder) open BleKeyboard.cpp in a plain text editor and find the line
  
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);

  Replace this with
  
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
  
Then save and close.

Now it should compile and it should reconnect Bluetooth on restarting the stick. No promises!

I found a post that said the following setup worked with the ESP32-C3, but I couldn't make it.
    Arduino IDE - 1.8.19
    Arduino ESP32 Boards - 2.0.18-arduino.5 - ESP32C3 Dev Module
    [NimBLE-Arduino]<https://github.com/h2zero/NimBLE-Arduino> - by Ryan powell 1.4.3
    Modified Version of this repo for [ESP32C3 Supermini]<https://github.com/oden-umaru/ESP32C3-BLE-Keyboard/releases/tag/0.3.3>

