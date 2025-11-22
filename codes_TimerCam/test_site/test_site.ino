#include "M5TimerCAM.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

const char* ap_ssid = "TimerCAM-AP";
const char* ap_password = "12345678";

IPAddress local_IP(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);


String wifi_ssid = "Test";
String wifi_password = "Test";

bool try_connect = true;
bool connected = false;
bool connecting = true;
bool ap_started = false;
unsigned long start_connecting = millis();
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

void get_wifi_prefs() {
    prefs.begin("config", false);
    wifi_ssid = prefs.getString("ssid", "None");
    wifi_password = prefs.getString("mdp", "None");
    prefs.end();
}

void set_wifi_prefs() {
    prefs.begin("config", false);
    prefs.putString("ssid", wifi_ssid);
    prefs.putString("mdp", wifi_password);
    prefs.end();
}

bool is_stored_prefs_same() {
    prefs.begin("config", false);
    bool ssid_same = wifi_ssid == prefs.getString("ssid", "None");
    bool password_same = wifi_password == prefs.getString("mdp", "None");
    prefs.end();
    return ssid_same && password_same;
}

void handleRoot() {

    if (server.method() == HTTP_POST) {

        wifi_ssid = server.arg("ssid");
        wifi_password = server.arg("password");

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
    if (ap_started) return;
    ap_started = true;
    WiFi.softAPConfig(local_IP, gateway, subnet);
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

    //get_wifi_prefs();
    
    Serial.print("Loaded ssid = ");
    Serial.println(wifi_ssid);
    
    Serial.print("Loaded pswd = ");
    Serial.println(wifi_password);

    WiFi.mode(WIFI_MODE_APSTA);

    start_ap();
    WiFi.begin(wifi_ssid, wifi_password);
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        if (WiFi.getMode() & WIFI_MODE_AP) {
            Serial.print("Stopping the AP");
            WiFi.softAPdisconnect(true);
            WiFi.enableAP(false);
            ap_started = false;

            if (!is_stored_prefs_same()) {
                set_wifi_prefs();
            } else {
                prefs.end();
            }
        }
        if (connecting) {
            Serial.print("Connected to wifi");
            connecting = false;
        }
    } else {
        if (!(WiFi.getMode() & WIFI_MODE_AP)) {
            WiFi.enableAP(true);
            start_ap();
        }

        if (! connecting and millis() - start_connecting > 20000) {
            start_connecting = millis();
            connecting = true;

            Serial.print("Connecting to wifi ...");
            WiFi.begin(wifi_ssid, wifi_password);
        } else if (connecting) {
            if (millis() - start_connecting > 10000) {
                Serial.println("Timeout !!!");
                connecting = false;
                WiFi.disconnect(true);
            } else {
                Serial.print(".");
            }
        }

        server.handleClient();
    }

    if (WiFi.getMode() & WIFI_MODE_AP) {
        Serial.println("AP ON");
    } else {
        Serial.println("AP OFF");
    }

    delay(50);
}
