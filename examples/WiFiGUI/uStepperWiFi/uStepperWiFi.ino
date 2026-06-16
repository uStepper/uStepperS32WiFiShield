#include "src/config.h"
#include "src/globals.h"
#include "src/recording.h"
#include "src/webserver.h"
#include "src/filesystem.h"
#include "GCode.h"

void uart_send(char *data);
void uart_default(char *cmd, char *data);
void uart_processData(char *cmd, char *data);
void web_send(char *data);
void web_default(char *cmd, char *data);

void setup() {
    delay(5000);
    UARTPORT.begin(BAUD_RATE);
    UARTPORT.println("\n=== uStepper WiFi GUI v" VERSION " ===");

    if (!initFS()) {
        UARTPORT.println("CRITICAL: Filesystem failed to mount");
    }

    comm.setSendFunc(&uart_send);
    comm.addCommand("DATA",    &uart_processData);
    comm.addCommand("REACHED", &uart_lineReached);
    comm.addCommand(NULL,      &uart_default);

    webcomm.setSendFunc(&web_send);
    webcomm.addCommand(GCODE_STOP,         &web_stop);
    webcomm.addCommand(GCODE_RECORD_START, &web_record);
    webcomm.addCommand(GCODE_RECORD_STOP,  &web_record);
    webcomm.addCommand(GCODE_RECORD_ADD,   &web_addLine);
    webcomm.addCommand(GCODE_RECORD_PLAY,  &web_record);
    webcomm.addCommand(GCODE_RECORD_PAUSE, &web_record);
    webcomm.addCommand(NULL,               &web_default);

    initWiFi();
    initWebsocket();
    initWebserver();

    UARTPORT.println("System ready");
}

void loop() {
    dnsServer.processNextRequest();
    server.handleClient();
    websocket.loop();

    comm.run();
    webcomm.run();

    if (UARTPORT.available() > 0) {
        comm.insert(UARTPORT.read());
    }

    recordHandler();
}

void uart_send(char *data) {
    UARTPORT.print(data);
}

void uart_default(char *cmd, char *data) {
    webcomm.send(data);
}

void uart_processData(char *cmd, char *data) {
    char *values = data + strlen(cmd);
    comm.value("A", &tlm_data.angle, values);
    comm.value("S", &tlm_data.steps, values);
    comm.value("V", &tlm_data.velocity, values);
    webcomm.send(data);
}

void web_send(char *data) {
    websocket.broadcastTXT(data);
}

void web_default(char *cmd, char *data) {
    comm.send(data);
}
