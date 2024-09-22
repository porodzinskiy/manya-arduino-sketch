# MANYA Arduino board and sketch
The project for a Mazda 3 BK car using an Arduino microcontroller

## Features
1. Sending information to the CAN bus for output to the LCD screen
2. Sending information to the RS485 interface to adjust the lighting dimmers
3. Getting the status of the buttons on the steering wheel
4. Emulation of the external remote control of the Pioneer car radio
5. Interact with an Android phone ([the app](https://github.com/porodzinskiy/manya-android-app)) via Bluetooth to control the media player on the phone, adjust lighting
6. Obtaining the status of a bistable lighting power relay

## Layout Board
<p align="center">
   <img src="https://github.com/porodzinskiy/manya-arduino-sketch/blob/main/manya_layout_board.jpg" width="500" title="Layout Board">
</p>

### Components
1. Arduino Nano
2. MCP 2515 CAN module
3. HC-05 Bluetooth module
4. UART (TTL) to RS485 module
5. DC-DC step-down module (ex. Mini360)

### Outputs
| Colour          | Description                                                     |
|---------------|-----------------------------------------------------------------|
| blue  	        | Voltage input -12V                           |
| brown  | Voltage input +12V                                      |
| yellow       | Voltage input of the bistable relay +12V                             |
| purple	   | CAN High output                                         |
| blue	   | CAN Low output                                   |
| white	   | RS485 High output                                              |
| gray	   | RS485 Low output          |
| green | Input of the analog steering wheel keyboard |
| light blue   | Pioneer remote control 3.5mm Jack High                                 |
| black   | Pioneer remote control 3.5mm Jack Low                                 |
