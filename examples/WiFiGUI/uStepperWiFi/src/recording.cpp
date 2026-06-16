#include "recording.h"
#include "config.h"
#include "globals.h"
#include "filesystem.h"
#include "GCode.h"

void uart_lineReached(char *cmd, char *data) {
    if (moreLines) {
        recordLineCount++;
        playStepsDelay = millis() + PLAY_STEP_DELAY_MS;
    }
}

void web_stop(char *cmd, char *data) {
    playRecording = false;
    comm.send(data);
}

void web_addLine(char *cmd, char *data) {
    char tempAngle[12] = {'\0'};
    dtostrf(tlm_data.angle, 6, 2, tempAngle);
    char buf[64] = {'\0'};
    snprintf(buf, sizeof(buf), "A%s", tempAngle);
    saveData(buf);
}

void web_record(char *cmd, char *data) {
    if (!strcmp(cmd, GCODE_RECORD_START)) {
        clearData();
        isRecording = true;
    } else if (!strcmp(cmd, GCODE_RECORD_STOP)) {
        isRecording = false;
        playRecording = false;
        recordLineCount = 0;
    } else if (!strcmp(cmd, GCODE_RECORD_PLAY)) {
        if (!playRecording)
            recordLineCount = 0;
        playRecording = true;
    } else if (!strcmp(cmd, GCODE_RECORD_PAUSE)) {
        playRecording = false;
    }
}

void recordHandler() {
    if (!playRecording) return;
    if (millis() <= playStepsDelay) return;

    moreLines = sendRecordLine(recordLineCount);
    playStepsDelay = millis() + PLAY_LINE_DELAY_MS;

    if (!moreLines) {
        playRecording = false;
        recordLineCount = 0;
        playStepsDelay = 0;
        webcomm.send((char *)"END");
    } else {
        char buf[24] = {'\0'};
        snprintf(buf, sizeof(buf), "LINE %d", recordLineCount);
        webcomm.send(buf);
    }
}
