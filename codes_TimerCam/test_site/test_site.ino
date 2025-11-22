#include "M5TimerCAM.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

const char* ap_ssid = "TimerCAM-AP";
const char* ap_password = "12345678";

String wifi_ssid;
String wifi_password;

bool try_connect = true;
bool connected = false;

WebServer server(80);

Preferences prefs;

const char* htmlForm = R"rawliteral(
<!DOCTYPE html>
<html>
    <head>
        <title>Config Wi-Fi</title>
    </head>
    <body>
        <h2>Connectez votre Timercam au Wi-Fi</h2>
        <form action="/" method="POST">
            SSID: <input type="text" name="ssid"><br><br>
            Mot de passe: <input type="text" name="password"><br><br>
            <input type="submit" value="Envoyer">
        </form>
    </body>
</html>
)rawliteral";

void handleRoot() {

    if (server.method() == HTTP_POST) {

        wifi_ssid = server.arg("ssid");
        wifi_password = server.arg("password");

        prefs.begin("config", false);
        prefs.putString("ssid", wifi_ssid);
        prefs.putString("mdp", wifi_password);
        prefs.end();

        Serial.println("=== DONNÉES REÇUES ===");
        Serial.print("SSID : ");
        Serial.println(wifi_ssid);
        Serial.print("PASSWORD : ");
        Serial.println(wifi_password);
        try_connect = true;

        // IMPORTANT : éviter la re-soumission du POST
        server.sendHeader("Location", "/");
        server.send(303);    // Redirect vers GET /

        return;
    }

    // GET → on affiche la page
    server.send(200, "text/html", htmlForm);
}

void start_ap() {
    WiFi.softAP(ap_ssid, ap_password);

    Serial.println("AP SSID: TimerCAM-AP");
    Serial.println("AP PASSWORD: 12345678");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", handleRoot);
    server.begin();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    prefs.begin("config", false);

    wifi_ssid = prefs.getString("ssid", "None");
    wifi_password = prefs.getString("mdp", "None");

    prefs.end();

    Serial.print("Loaded ssid = ");
    Serial.println(wifi_ssid);
    
    Serial.print("Loaded pswd = ");
    Serial.println(wifi_password);

    start_ap();
}

void loop() {
    if (try_connect) {
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifi_ssid, wifi_password);
        WiFi.setSleep(false);
        Serial.print("Connecting to WiFi");
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
            if (millis() - start > 10000) {
                break;
            }
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println();
            Serial.print("Connected. IP address: ");
            Serial.println(WiFi.localIP());
            try_connect = false;
        } else {
            try_connect = false;
            start_ap();
        }
    }
    if (WiFi.status() != WL_CONNECTED) {
        connected = false;
        server.handleClient();
    } else {
        Serial.println("Je suis connecté");
        delay(500);
    }
}
