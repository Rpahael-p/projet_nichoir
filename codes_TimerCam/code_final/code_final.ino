#include "M5TimerCAM.h"
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Ticker.h>

#define BUTTON_LED_PIN 13
#define CAM_EXT_WAKEUP_PIN 4

const char* ap_ssid = "TimerCAM-AP";
const char* ap_password = "12345678";

IPAddress local_IP(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);

String wifi_ssid = "Test";
String wifi_password = "Test";

bool AP = false;
bool connecting = false;
unsigned long start_connecting = 0;
const unsigned long connect_timeout = 10000; // 10s

unsigned long connected_timestamp = 0;

Ticker ticker;
volatile uint8_t led_pwm = 0;

int n = 0; // nombre de wifi

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
<title>Connecting ...</title>
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
<h2>Connecting ...</h2>
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
    Serial.print("Saving to the prefs.");
    prefs.begin("config", false);
    prefs.putString("ssid", wifi_ssid);
    prefs.putString("mdp", wifi_password);
    prefs.end();
}

bool is_stored_prefs_same() {
    prefs.begin("config", false);
    bool same = wifi_ssid == prefs.getString("ssid", "None") &&
                wifi_password == prefs.getString("mdp", "None");
    prefs.end();
    return same;
}

void handleRoot() {
    String options = "";
    if (n <= 0) {
        options = "<option>No WiFi found</option>";
    } else {
        for (int i = 0; i < n; i++) {
            options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
        }
    }
    // WiFi.scanDelete();
    String fullPage = String(htmlFormHeader) + options + String(htmlFormFooter);
    server.send(200, "text/html", fullPage);
}

void handleConnecting() {
    if (server.method() == HTTP_POST) {
        wifi_ssid = server.arg("ssid");
        wifi_password = server.arg("password");

		Serial.print("AP ssid = "); Serial.println(wifi_ssid);
		Serial.print("AP password = "); Serial.println(wifi_password);

		set_wifi_prefs();

        server.send(200, "text/html", htmlConnecting);
        WiFi.begin(wifi_ssid, wifi_password);
        start_connecting = millis();
    } else {
        handleRoot();
    }
}

void start_ap() {
    if (AP) return;
    AP = true;

    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP(ap_ssid, ap_password);

    server.on("/", handleRoot);
    server.on("/connecting", handleConnecting);

    server.begin();
    Serial.println("AP started");
}

void stop_ap() {
    if (!AP) return;

    WiFi.softAPdisconnect(true);
    AP = false;

    if (!is_stored_prefs_same()) set_wifi_prefs();
}

void blink_cb() {
    if (led_pwm == 0) {
        led_pwm = 64;
    } else {
        led_pwm = 0;
    }
    TimerCAM.Power.setLed(led_pwm);
}

void setup() {
    TimerCAM.begin(true);
    delay(200);

    pinMode(BUTTON_LED_PIN, INPUT);

    WiFi.mode(WIFI_MODE_APSTA);

    get_wifi_prefs();

    n = WiFi.scanNetworks();
	Serial.print(n); Serial.println(" Wifi found");

    Serial.print("Loaded ssid = "); Serial.println(wifi_ssid);
    Serial.print("Loaded password = "); Serial.println(wifi_password);

    if (digitalRead(BUTTON_LED_PIN) == HIGH) {
        start_ap();
		ticker.attach(0.75, blink_cb);
    } else {
        Serial.println("Trying to connect to Wi-Fi...");
        WiFi.begin(wifi_ssid, wifi_password);
        connecting = true;
		ticker.attach(1.5, blink_cb);
        start_connecting = millis();
    }
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        if (connected_timestamp == 0) {
            connected_timestamp = millis();
        }
        if (AP) {
            stop_ap();
    		TimerCAM.Power.setLed(255);
        }
        Serial.println("Connected to Wi-Fi!");
		ticker.detach();
        if (millis() - connected_timestamp > 5000) {
    		TimerCAM.Power.setLed(0);
            Serial.println("Going to sleep");
            WiFi.disconnect();
        }
        // Ici on pourrait prendre une photo ou autre action
    } else if (connecting) {
        if (millis() - start_connecting > connect_timeout) {
            Serial.println("Going to sleep");
            connecting = false;
			ticker.detach();
            // Ne pas démarrer l'AP
        }
    }

    if (AP) {
        server.handleClient();
    }

    delay(50);
}