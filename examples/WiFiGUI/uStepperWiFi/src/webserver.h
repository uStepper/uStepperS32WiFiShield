#pragma once

#include "config.h"
#include <WebSocketsServer.h>

void initWiFi();
void initWebserver();
void initWebsocket();
void handleCaptivePortal();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t len);
void uploadJob();
