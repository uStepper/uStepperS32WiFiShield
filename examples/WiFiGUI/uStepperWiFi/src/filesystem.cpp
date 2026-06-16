#include "filesystem.h"
#include "config.h"
#include "GCode.h"

bool initFS() {
    if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
        UARTPORT.println("Failed to mount LittleFS");
        return false;
    }
    UARTPORT.println("LittleFS mounted");
    return true;
}

bool saveData(const char *data) {
    File f = LittleFS.open(RECORD_PATH, "a");
    if (!f) {
        UARTPORT.println("Failed to open recording file for append");
        return false;
    }
    f.println(data);
    f.close();
    return true;
}

void clearData() {
    File f = LittleFS.open(RECORD_PATH, "w");
    if (f) {
        f.print("");
        f.close();
    }
}

bool sendRecordLine(uint16_t lineNum) {
    File file = LittleFS.open(RECORD_PATH, "r");
    if (!file) {
        return false;
    }

    char buf[50];
    uint8_t len = 0;

    for (uint16_t i = 0; i <= lineNum; i++) {
        memset(buf, 0, sizeof(buf));
        len = file.readBytesUntil('\n', buf, sizeof(buf) - 1);
    }

    file.close();

    if (len > 2) {
        buf[len] = '\0';
        char command[100] = {'\0'};
        snprintf(command, sizeof(command), "%s %s", GCODE_MOVETO, buf);
        extern GCode comm;
        comm.send(command);
        return true;
    }

    return false;
}
