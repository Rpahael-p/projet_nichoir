#include "M5TimerCAM.h"           // Library for M5TimerCAM
#include "esp_sleep.h"
#include <WiFi.h>                 // Library to manage Wi-Fi connection
#include <PubSubClient.h>         // Library to manage MQTT
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

/////////////// MQTT /////////////////
// const char* mqtt_server       = "192.168.2.51";     // MQTT broker IP address PI
const char* mqtt_server       = "10.42.0.1";     // MQTT broker IP address PI
// const char* mqtt_server       = "192.168.2.49";     // MQTT broker IP address PC
const int   mqtt_port         = 1883;               // MQTT port (default 1883)

String mqtt_topic_start = "TimerCam/";
String mqtt_topic_photo   = "/photo";
String mqtt_topic_batterie   = "/batterie";


WiFiClient espClient;                               // Create Wi-Fi client for MQTT
PubSubClient mqttClient(espClient);                 // Create MQTT client using espClient
uint16_t max_size = 8192;                           // Maximum MQTT message buffer size


bool go_to_sleep = false;

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

String getShortID() {
  uint64_t mac = ESP.getEfuseMac(); // 48 bits
  uint32_t low = mac & 0xFFFFFF; // first 24 bits
  char id[7];
  sprintf(id, "%06X", low);
  return String(id);
}

void setup_camera() {
	// A CHANGER VERS UN BOUCLE AVEC TIMEOUT
	if (!TimerCAM.Camera.begin()) {                 // Initialize camera
		Serial.println("Camera Init Fail");         // Print error if camera init fails
		return;
	}
	Serial.println("Camera Init Success");          // Print success message

	TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor, PIXFORMAT_JPEG); // Set JPEG format
	TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor, FRAMESIZE_QVGA); // Set frame size
	TimerCAM.Camera.sensor->set_vflip(TimerCAM.Camera.sensor, 1);                  // Vertical flip
	TimerCAM.Camera.sensor->set_hmirror(TimerCAM.Camera.sensor, 0);                // Horizontal mirror
}

void setup() {
	// Setup of the TimerCam
	TimerCAM.begin(true);
	TimerCAM.Power.setLed(128);
	delay(200);

	setup_camera();

	// Setup of the deep sleep
	gpio_hold_en((gpio_num_t)POWER_HOLD_PIN);
	gpio_deep_sleep_hold_en();
	esp_sleep_enable_ext0_wakeup((gpio_num_t) CAM_EXT_WAKEUP_PIN, 1);  // 1 = High, 0 = Low

	esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
	if (cause == ESP_SLEEP_WAKEUP_GPIO) {
		Serial.println("Wake up by GPIO");
	} else if (cause == ESP_SLEEP_WAKEUP_EXT0) {
		Serial.println("Wake up by EXT0");
	} else if (cause == ESP_SLEEP_WAKEUP_EXT1) {
		Serial.println("Wake up by EXT1");
	} else if (cause == ESP_SLEEP_WAKEUP_TIMER) {
		Serial.println("Wake up by RTC internal timer");
	} else if (cause == ESP_SLEEP_WAKEUP_TOUCHPAD) {
		Serial.println("Wake up by touchpad");
	} else if (cause == ESP_SLEEP_WAKEUP_ULP) {
		Serial.println("Wake up by ULP coprocessor");
	} else if (cause == ESP_SLEEP_WAKEUP_UART) {
		Serial.println("Wake up by UART");
	} else if (cause == ESP_SLEEP_WAKEUP_UNDEFINED) {
		Serial.println("Wake up: power-on or reset");
	} else {
		Serial.print("Wake up from unknown cause: ");
		Serial.println((int)cause);
	}

	// Setup MQTT
	mqttClient.setServer(mqtt_server, mqtt_port); // Set MQTT server and port
	mqttClient.setBufferSize(max_size);           // Set MQTT buffer size

	pinMode(BUTTON_LED_PIN, INPUT);

	WiFi.mode(WIFI_MODE_APSTA);

	// get_wifi_prefs();
	wifi_ssid = "test";
	wifi_password = "";

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

	pinMode(BUTTON_LED_PIN, INPUT_PULLDOWN);
}

void loop() {
	if (go_to_sleep) {
		Serial.println("Going to sleep");
		server.stop();
		WiFi.softAPdisconnect(true);
		WiFi.disconnect(true);
		WiFi.mode(WIFI_OFF);
		ticker.detach();
		TimerCAM.Power.setLed(0);

		// TimerCAM.Rtc.setAlarmIRQ(15);
		esp_sleep_enable_timer_wakeup(20000000);
		esp_deep_sleep_start();
		return;
	}
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
		Serial.println(millis() - connected_timestamp);
		if (millis() - connected_timestamp > 10000) {
			Serial.println("Timeout connected");
			TimerCAM.Power.setLed(0);
			go_to_sleep = true;
			WiFi.disconnect();
			return;
		}
		sendPhotoMQTT();
	} else if (connecting) {
		if (millis() - start_connecting > connect_timeout) {
			Serial.println("timeout not connected");
			connecting = false;
			ticker.detach();
			go_to_sleep = true;
			// Ne pas démarrer l'AP
		}
	}

	if (AP) {
		server.handleClient();
	}

	delay(50);
}

void sendPhotoMQTT() {                              // Function to capture and send photo
	if (!TimerCAM.Camera.get()) {                   // Capture image
		Serial.println("Failed to capture image");  // Print error if capture fails
		return;
	}
	Serial.printf("Photo captured, size: %d bytes\n", TimerCAM.Camera.fb->len); // Print image size

	// Publish image via MQTT
	mqttClient.connect("TimerCamClient", "esp32", "nichoir");
	delay(250);
	if (!mqttClient.connected()) {                  // Check MQTT connection
		Serial.print("Failed, rc=");            // Print error code
		Serial.print(mqttClient.state());
		delay(2000);                            // Wait before retrying
		return;
	}
	// mqttClient.publish(mqtt_topic_text, "Message from TimerCam"); // Send text message

	String batterie = String(esp_random() % 100);
	Serial.println(batterie.c_str());
	String BatterieTopic = mqtt_topic_start + getShortID() + mqtt_topic_batterie;
	Serial.println(BatterieTopic.c_str());
	String PhotoTopic = mqtt_topic_start + getShortID() + mqtt_topic_photo;
	Serial.println(PhotoTopic.c_str());

	mqttClient.publish("test", "test");

	bool sent_batterie = mqttClient.publish(BatterieTopic.c_str(), batterie.c_str()); // Send text message
	bool sent_photo = mqttClient.publish(PhotoTopic.c_str(), TimerCAM.Camera.fb->buf, TimerCAM.Camera.fb->len, false); // Send photo

	unsigned long start_loop = millis();
	while (mqttClient.connected() && millis() - start_loop < 2000) {
		mqttClient.loop();
	}

	if (sent_photo) {                                // Check if photo sent successfully
		Serial.printf("Photo sent via MQTT, size: %d bytes\n", TimerCAM.Camera.fb->len);
		go_to_sleep = true;
		WiFi.disconnect();
	} else {
		Serial.println("Failed to send photo via MQTT"); // Print error if failed
	}

	TimerCAM.Camera.free();                              // Free camera buffer
}