# uStepper S32 WiFi Shield 

This repository provides you with the three basic code snippets relevant for the uStepper S32 WiFi Shield:

1. uStepper WiFi GUI - providing you with an easy and intuitive way to control a single uStepper S32 manually while also providing the ability to record and replay sequences of operation
2. UART bridge - a simple piece of software that bridges the uStepper S32 Modbus interface to the RS485 physical interface on the uStepper S32 WiFi Shield
3. ESPHome - a yaml file for ESPHome enabling you to expose the uStepper S32 Modbus interface in ESPHome compatile smart home systems which at the time of writing is Home Assistant and to some extent Homey

## uStepper WiFi GUI
https://docs.espressif.com/projects/esp-test-tools/en/latest/esp32/production_stage/tools/flash_download_tool.html
## Modbus RTU Connection
Max 200 m cable length. 
Characteristic impedance 100 - 130 Ohm. 
Shunt capacitance < 100 pF/m. 

### Common ground: 
All network nodes must share a common ground connection as the bus signals shall refer to the same voltage potential. 

### Network termination: 
120 Ohm resistors shall be connected between the two RS485 signal wires at each end-point of the bus wire. The resistor value shall match the characteristic impedance of the cable.

## Modbus TCP Connection

## ESPHome Modbus Interface

## Modbus Protocol description
Details on the protocol can be found here: LINK TIL USTEPPR MODBUS DESCR.
