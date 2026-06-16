#pragma once

#include "config.h"
#include "GCode.h"

extern GCode comm;
extern GCode webcomm;

extern WebServer server;
extern WebSocketsServer websocket;
extern DNSServer dnsServer;

extern bool isRecording;
extern bool playRecording;
extern bool moreLines;

extern uint32_t playStepsDelay;
extern uint16_t recordLineCount;

struct TlmData {
    float angle;
    float velocity;
    int32_t steps;
};

extern TlmData tlm_data;
