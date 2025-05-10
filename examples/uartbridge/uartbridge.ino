#include <ModbusRTU.h>
#include <SoftwareSerial.h>

// RS485 UART pin definitions
#define RS485_RX_PIN 5  // RS485 Receive pin
#define RS485_TX_PIN 6  // RS485 Transmit pin
#define RS485_DIR_PIN 4 // RS485 Driver Enable pin

// Baud rate for both interfaces
#define BAUD_RATE 9600

// Create a SoftwareSerial instance for RS485 communication
SoftwareSerial rs485Serial(RS485_RX_PIN, RS485_TX_PIN);

void setup() {
  // Initialize RS485 Modbus Master
  rs485Serial.begin(BAUD_RATE); // Configure SoftwareSerial for RS485 communication

  // Initialize Modbus Slave on UART0
  Serial.begin(BAUD_RATE); // Configure Serial (UART0) for Modbus Slave communication

  // Initialize the RS-485 driver enable pin
  pinMode(RS485_DIR_PIN, OUTPUT);       // Set RS485_DIR_PIN as output
  digitalWrite(RS485_DIR_PIN, LOW);     // Set RS485_DIR_PIN to LOW (receive mode)
}

void loop() {
  // Forward data from RS485 to Modbus Slave
  if (rs485Serial.available()) {        // Check if data is available on RS485
    while (rs485Serial.available()) {   // Read all available data
      uint8_t byte = rs485Serial.read(); // Read a byte from RS485
      Serial.write(byte);               // Send the byte to Modbus Slave
    }
  }

  // Forward data from Modbus Slave to RS485
  if (Serial.available()) {             // Check if data is available on Serial
    digitalWrite(RS485_DIR_PIN, HIGH);  // Set RS485_DIR_PIN to HIGH (transmit mode)
    while (Serial.available()) {        // Read all available data
      uint8_t byte = Serial.read();     // Read a byte from Serial
      rs485Serial.write(byte);          // Send the byte to RS485
    }
    rs485Serial.flush();                // Wait for all data to be transmitted
    digitalWrite(RS485_DIR_PIN, LOW);   // Set RS485_DIR_PIN to LOW (receive mode)
  }
}