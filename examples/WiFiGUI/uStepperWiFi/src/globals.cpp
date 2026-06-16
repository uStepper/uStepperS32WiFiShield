#include "globals.h"

GCode comm;
GCode webcomm;

WebServer server(80);
WebSocketsServer websocket(81);
DNSServer dnsServer;

bool isRecording = false;
bool playRecording = false;
bool moreLines = false;

uint32_t playStepsDelay = 0;
uint16_t recordLineCount = 0;

TlmData tlm_data = {0.0, 0.0, 0};
