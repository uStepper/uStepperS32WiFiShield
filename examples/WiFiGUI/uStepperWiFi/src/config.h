#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <DNSServer.h>
#include "FS.h"
#include <LittleFS.h>

#define FORMAT_LITTLEFS_IF_FAILED true

#define UARTPORT Serial

#define DNS_PORT 53
#define AP_IP IPAddress(192, 168, 4, 1)
#define AP_GATEWAY IPAddress(192, 168, 4, 1)
#define AP_SUBNET IPAddress(255, 255, 255, 0)

#define VERSION "0.3.0"

// G-Code move commands
#define GCODE_MOVE          "G0"
#define GCODE_MOVETO        "G1"
#define GCODE_CONTINUOUS    "G2"
#define GCODE_BRAKE         "G3"
#define GCODE_HOME          "G4"

// G-Code miscellaneous commands
#define GCODE_STOP              "M0"
#define GCODE_SET_SPEED         "M1"
#define GCODE_SET_ACCEL         "M2"
#define GCODE_SET_BRAKE_FREE    "M3"
#define GCODE_SET_BRAKE_COOL    "M4"
#define GCODE_SET_BRAKE_HARD    "M5"
#define GCODE_SET_CL_ENABLE     "M6"
#define GCODE_SET_CL_DISABLE    "M7"

#define GCODE_RECORD_START      "M10"
#define GCODE_RECORD_STOP       "M11"
#define GCODE_RECORD_ADD        "M12"
#define GCODE_RECORD_PLAY       "M13"
#define GCODE_RECORD_PAUSE      "M14"
#define GCODE_REQUEST_DATA      "M15"
#define GCODE_REQUEST_CONFIG    "M16"

#define BAUD_RATE 115200

#define RECORD_PATH "/recording.txt"
#define PLAY_LINE_DELAY_MS 250
#define PLAY_STEP_DELAY_MS 500
