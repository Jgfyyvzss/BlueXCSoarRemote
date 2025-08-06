## Key features:
* OTA updates of both Firmware and Keymap file can be done direct from a device to the Remote. The Remote creates an Access Point.
* Keymap can be updated without any change to Firmware. Easily change your button mapping at any time.
* All buttons can have separate actions for Click, Double-Click and Long-Press. Possibility of 24 different keystrokes being sent (only 12 unique keystrokes can be sent at this time).

Currently the following keystrokes can be sent:
ESC - KEY_ESC
RETURN - KEY_RETURN
F1 - KEY_F1
F2 - KEY_F2
...
F10 - KEY_F10

When the remote first starts (power on) it checks to see if the TopRight button is being held down. If it is it goes into OTA update mode and starts the Access Point and webserver.<br/>
You will find a WiFi network called BLUEXCREM, password 12345678<br/>
Once connected to that network you can browse to http://192.168.4.1 (make sure your browser isn't tying to force an https connection) and a page will appear allowing you to select a Firmware ( .bin) or keymap.txt file to update.

The keymap.txt file is a simple text file that looks like:<br/>
 '# Click,DoubleClick,LongPress<br/>
Top,KEY_F1,KEY_F3,KEY_F8<br/>
TopRight,KEY_F4,KEY_F5,KEY_F6<br/>
BotRight,KEY_ESC,KEY_ESC,KEY_ESC<br/>
Left,KEY_LEFT_ARROW,,<br/>
Right,KEY_RIGHT_ARROW,,<br/>
Up,KEY_UP_ARROW,,<br/>
Down,KEY_DOWN_ARROW,,<br/>
Centre_Click,KEY_RETURN,,KEY_ESC<br/>

Displaying it as a table makes it easier to understand:<br/>
|# |Click|DoubleClick|LongPress|
|--|-----|-----------|---------|
|Top|KEY_F1|KEY_F3|KEY_F8|
|TopRight|KEY_F4|KEY_F5|KEY_F6|
|BotRight|KEY_ESC|KEY_ESC|KEY_ESC|
|Left|KEY_LEFT_ARROW|||
|Right|KEY_RIGHT_ARROW|||
|Up|KEY_UP_ARROW|||
|Down|KEY_DOWN_ARROW|||
|Centre_Click|KEY_RETURN||KEY_ESC|

The first column is the button name or position.<br/>
Blank cells are ignored.<br/>
Only those keystrokes listed above can be included.<br/>

### Requirements for succesful compilation of the firmware
* Arduino IDE 2+ (created in 2.3.6)
* Arduino IDE esp32 board version 2.0.11. More recent versions will fail to run the BleKeyboard.
* [BleKeyboard library](https://github.com/T-vK/ESP32-BLE-Keyboard) See Note 1.
* [Button2 library](https://github.com/LennartHennigs/Button2)

Note 1: BleKeyboard.cpp has to be modified.<br/>
Find pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);<br/>
Replace this with<br/>
pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);<br/>
