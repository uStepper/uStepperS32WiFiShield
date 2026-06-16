#pragma once

#include <Arduino.h>
#include "FS.h"
#include <LittleFS.h>

bool initFS();
bool saveData(const char *data);
void clearData();
bool sendRecordLine(uint16_t lineNum);
