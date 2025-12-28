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

const char* htmlFormHeader = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>TimerCam Configuration</title>
<style>
    body {
        background: #f2f2f2;
        font-family: Arial, sans-serif;
        display: flex;
        justify-content: center;
        align-items: center;
        height: 100vh;
        margin: 0;
    }

    .card {
        background: white;
        padding: 30px;
        border-radius: 12px;
        box-shadow: 0 4px 20px rgba(0,0,0,0.15);
        width: 320px;
        text-align: center;
    }

    h2 {
        margin-top: 0;
        color: #333;
    }

    select, input[type="text"] {
        width: 100%;
        padding: 10px;
        margin-top: 10px;
        border-radius: 6px;
        border: 1px solid #aaa;
        font-size: 14px;
    }

    input[type="submit"] {
        margin-top: 20px;
        width: 100%;
        padding: 12px;
        background: #0078ff;
        color: white;
        font-size: 16px;
        border: none;
        border-radius: 6px;
        cursor: pointer;
    }

    input[type="submit"]:hover {
        background: #005fcc;
    }

    p {
        color: #666;
        font-size: 12px;
        margin-top: 15px;
    }
</style>
</head>
<body>
<div class="card">
<h2>Wi-Fi Setup</h2>
<form action="/connecting" method="POST">
SSID:
<select name="ssid">
)rawliteral";

const char* htmlFormFooter = R"rawliteral(
</select>
<br>
Password:
<input type="text" name="password">
<input type="submit" value="Connect">
</form>
<p>After submitting, the device will try to connect to your Wi-Fi.</p>
</div>
</body>
</html>
)rawliteral";

const char* htmlConnecting = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>Connecting…</title>
<meta http-equiv="refresh" content="10; URL=/" />
<style>
    body {
        background: #f2f2f2;
        font-family: Arial, sans-serif;
        display: flex;
        justify-content: center;
        align-items: center;
        height: 100vh;
        margin: 0;
    }
    .card {
        background: white;
        padding: 30px;
        border-radius: 12px;
        box-shadow: 0 4px 20px rgba(0,0,0,0.15);
        width: 320px;
        text-align: center;
    }
    h2 {
        margin-top: 0;
        color: #333;
    }
    p {
        color: #666;
        font-size: 14px;
    }
</style>
</head>
<body>
<div class="card">
<h2>Connecting…</h2>
<p>The device is trying to connect to your Wi-Fi.</p>
<p>If it fails, you will return to the configuration page.</p>
</div>
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
    int n = WiFi.scanNetworks();

    String options = "";

    if (n == 0) {
        options = "<option>No WiFi found</option>";
    } else {
        for (int i = 0; i < n; i++) {
            options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
        }
    }

    WiFi.scanDelete();

    // 🔵 Construire la page HTML complète
    String fullPage = String(htmlFormHeader) + options + String(htmlFormFooter);

    server.send(200, "text/html", fullPage);
}

void handleConnecting() {

    if (server.method() == HTTP_POST) {

        wifi_ssid = server.arg("ssid");
        wifi_password = server.arg("password");

        Serial.println("=== DONNÉES REÇUES ===");
        Serial.print("SSID : ");
        Serial.println(wifi_ssid);
        Serial.print("PASSWORD : ");
        Serial.println(wifi_password);
        try_connect = true;

        server.send(200, "text/html", htmlConnecting);

        return;
    };

    // GET → on affiche la page
    handleRoot();
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
    server.on("/connecting", handleConnecting);
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
