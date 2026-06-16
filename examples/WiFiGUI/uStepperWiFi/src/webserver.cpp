#include "webserver.h"
#include "config.h"
#include "globals.h"
#include "GCode.h"

static File uploadFile;

void initWiFi() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
    delay(100);

    uint32_t chipid = 0;
    for (int i = 0; i < 17; i = i + 8) {
        chipid |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }

    char ap_ssid[30];
    snprintf(ap_ssid, sizeof(ap_ssid), "uStepper GUI-%08X", chipid);

    if (!WiFi.softAP(ap_ssid)) {
        UARTPORT.println("WiFi AP creation failed!");
    } else {
        UARTPORT.print("AP started: ");
        UARTPORT.println(ap_ssid);
        UARTPORT.print("IP: ");
        UARTPORT.println(WiFi.softAPIP());
    }

    dnsServer.start(DNS_PORT, "*", AP_IP);
    UARTPORT.println("DNS server started for captive portal");
}

void initWebsocket() {
    websocket.begin();
    websocket.onEvent(webSocketEvent);
}

void handleCaptivePortal() {
    String host = server.hostHeader();

    // Common captive portal detection URLs
    if (server.uri() == "/generate_204" ||
        server.uri() == "/fwlink" ||
        server.uri() == "/hotspot-detect.html" ||
        server.uri() == "/connectivity-check.html" ||
        server.uri() == "/check_network_status.txt" ||
        server.uri() == "/ncsi.txt")
    {
        server.sendHeader("Location", "http://192.168.4.1", true);
        server.send(302, "text/plain", "Redirecting...");
        return;
    }

    // Redirect unknown hosts to the device IP
    if (host != "192.168.4.1") {
        server.sendHeader("Location", "http://192.168.4.1", true);
        server.send(302, "text/plain", "Redirecting...");
        return;
    }

    // Serve static files from LittleFS
    String path = server.uri();
    if (path.endsWith("/")) {
        path += "index.html";
    }

    String contentType = "text/plain";
    if (path.endsWith(".html")) contentType = "text/html";
    else if (path.endsWith(".css")) contentType = "text/css";
    else if (path.endsWith(".js")) contentType = "application/javascript";
    else if (path.endsWith(".png")) contentType = "image/png";
    else if (path.endsWith(".webp")) contentType = "image/webp";
    else if (path.endsWith(".ttf")) contentType = "font/ttf";
    else if (path.endsWith(".ico")) contentType = "image/x-icon";
    else if (path.endsWith(".txt")) contentType = "text/plain";

    if (LittleFS.exists(path)) {
        File file = LittleFS.open(path, "r");
        if (file) {
            server.streamFile(file, contentType);
            file.close();
            return;
        }
    }

    server.send(404, "text/plain", "File not found");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t len) {
    if (type == WStype_TEXT) {
        // Insert payload bytes plus newline, then process immediately.
        // This ensures each message is handled before the next one starts,
        // preventing concatenation when websocket.loop() delivers multiple
        // messages in a single call (e.g. M11 + G4 from home button).
        for (size_t i = 0; i < len; i++) {
            webcomm.insert(payload[i]);
        }
        webcomm.insert('\n');
        webcomm.run();
    } else if (type == WStype_CONNECTED) {
        UARTPORT.print("WebSocket client connected: ");
        UARTPORT.println(num);
    } else if (type == WStype_DISCONNECTED) {
        UARTPORT.print("WebSocket client disconnected: ");
        UARTPORT.println(num);
    }
}

void uploadJob() {
    HTTPUpload &upload = server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        if (!filename.equals("recording.txt")) {
            server.send(500, "text/plain", "Wrong filename");
            return;
        }
        if (!filename.startsWith("/")) {
            filename = "/" + filename;
        }
        uploadFile = LittleFS.open(filename, "w");
        if (!uploadFile) {
            server.send(500, "text/plain", "Couldn't create file");
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            uploadFile.write(upload.buf, upload.currentSize);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            uploadFile.close();
            server.send(303);
        } else {
            server.send(500, "text/plain", "Couldn't create file");
        }
        uploadFile.close();
    }
}

void initWebserver() {
    // Captive portal paths
    server.on("/generate_204", handleCaptivePortal);
    server.on("/fwlink", handleCaptivePortal);
    server.on("/hotspot-detect.html", handleCaptivePortal);
    server.on("/connectivity-check.html", handleCaptivePortal);
    server.on("/check_network_status.txt", handleCaptivePortal);
    server.on("/ncsi.txt", handleCaptivePortal);

    // File upload
    server.on("/upload", HTTP_POST, []() { server.send(200); }, uploadJob);

    // Fallback: serve static files or captive portal redirect
    server.onNotFound(handleCaptivePortal);

    server.begin();
    UARTPORT.println("HTTP server started");
}
