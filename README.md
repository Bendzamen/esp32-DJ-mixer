# ESP32 DJ Mixer

An open-source, cheap DJ mixer for anybody who wants to get the thrills of DJing but doesn't want to spend a fortune.
Total cost of this build is around 30€. (for me, your price might be different depending on where you get the parts from)

## Software
The mix uses BLE MIDI protocol to connect to a PC and control a DJ software.
I have used an open-source DJ software [Mixxx](https://mixxx.org), as it is free and enables custom mapping of MIDI devices.

## Hardware
- ESP32 as the **brain**
- 2 rotary knobs for the **main jog wheels**
- 15 rotary potentiometers for **EQ, main volume and pitch controls**
- 3 linear potentiometers for **volume and cross faders**
- 10 buttons for **transport controls**
- CD74HC4067 16-channel multiplexer
- status LED to indicate **BLE connection**

Due to the limited amout of esp32 pins, it uses 16-channel analog multiplexer (CD74HC4067) for majority of the analog knobs.
Other buttons and knobs are directly connected to the esp32.

Created by Benjamin Lapos 2023
