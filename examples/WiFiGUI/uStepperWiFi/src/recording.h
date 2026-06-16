#pragma once

#include <Arduino.h>

void recordHandler();

// GCode callbacks for recording commands
void web_record(char *cmd, char *data);
void web_addLine(char *cmd, char *data);
void web_stop(char *cmd, char *data);

// UART callback for line reached
void uart_lineReached(char *cmd, char *data);
