#include "M5TimerCAM.h"           // Library for M5TimerCAM
#include "esp_sleep.h"
#include <WiFi.h>                 // Library to manage Wi-Fi connection
#include <PubSubClient.h>         // Library to manage MQTT
#include <WebServer.h>
#include <Preferences.h>
#include <Ticker.h>

#define BUTTON_LED_PIN 13		// Yellow
#define CAM_EXT_WAKEUP_PIN 4	// White

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
// const char* mqtt_server       = "10.42.0.1";     // MQTT broker IP address PI
const char* mqtt_server       = "192.168.2.31";     // MQTT broker IP address PC ecole
// const char* mqtt_server       = "192.168.1.58";     // MQTT broker IP address PC maiso,
const int   mqtt_port         = 1883;               // MQTT port (default 1883)

String mqtt_topic_start = "TimerCam/";
String mqtt_topic_photo   = "/photo";
String mqtt_topic_batterie   = "/batterie";


WiFiClient espClient;                               // Create Wi-Fi client for MQTT
PubSubClient mqttClient(espClient);                 // Create MQTT client using espClient
uint16_t max_size = 65536;                           // Maximum MQTT message buffer size


bool go_to_sleep = false;

WebServer server(80);
Preferences prefs;
uint8_t Resolution;
uint8_t photo_attempt;
uint8_t quality;
uint8_t brightness;
uint8_t contrast;
uint8_t saturation;
uint8_t sharpness;
bool vflip;
bool hmirror;


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
		width: 400px;
		text-align: center;
	}

	h2 {
		margin-top: 0;
		color: #333;
	}

	select, input[type="text"], input[type="number"] {
		width: 100%;
		padding: 10px;
		margin-top: 10px;
		border-radius: 6px;
		border: 1px solid #aaa;
		font-size: 14px;
		box-sizing: border-box;
	}

    select {    /*bottom margin to select*/
		margin-bottom: 10px;
    }

	input[type="submit"] {  /*Conenct button style*/
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

	input[type="submit"]:hover {    /*havering connect button*/
		background: #005fcc;
	}

	p { /*Info text*/
		color: #666;
		font-size: 12px;
		margin-top: 15px;
	}

    .advanced-toggle {  /*invisible checkbox*/
		display: none;
	}

	.advanced-header {  /*Advanced settings label*/
		display: flex;
		align-items: center;
		justify-content: space-between;
		margin-top: 20px;
		padding: 10px;
		border-radius: 6px;
		font-weight: bold;
	}

	.advanced-content {
		max-height: 0;
		overflow: hidden;
		transition: max-height 0.3s ease-out;
		margin-top: 0;
	}

	.advanced-box { /*Advanced settings box*/
		border: 1px solid black;
		border-radius: 6px;
		padding: 15px;
		margin-top: 10px;
	}

	.advanced-toggle:checked ~ .advanced-content { /*Show content when checked*/
		max-height: 600px;
		margin-top: 15px;
	}

    .advanced-toggle:checked ~ .advanced-header .arrow {    /*rotate the arrow when checked*/
		transform: rotate(90deg);
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

<input type="checkbox" id="advanced-toggle" class="advanced-toggle">
<label for="advanced-toggle" class="advanced-header">
	<span>Advanced settings</span>
	<span class="arrow">></span>
</label>
<div class="advanced-content">
    <div class="advanced-box">
		<label>Resolution</label>
		<select name="Resolution">
			<option value="0">96x96</option>
			<option value="1">160x120 (QQVGA)</option>
			<option value="2">128x128</option>
			<option value="3">176x144 (QCIF)</option>
			<option value="4">240x176 (HQVGA)</option>
			<option value="5">240x240</option>
			<option value="6" selected>320x240 (QVGA)</option>
			<option value="7">320x320</option>
			<option value="8">400x296 (CIF)</option>
			<option value="9">480x320 (HVGA)</option>
			<option value="10">640x480 (VGA)</option>
			/*<option value="11">800x600 (SVGA)</option>*/
			<option value="12">1024x768 (XGA)</option>
			<option value="13">1280x720 (HD)</option>
			<option value="14">1280x1024 (SXGA)</option>
			<option value="15">1600x1200 (UXGA)</option>
			<option value="16">1920x1080 (FHD)</option>
			<option value="17">720x1280 (P_HD)</option>
			<option value="18">864x1536 (P_3MP)</option>
			<option value="19">2048x1536 (QXGA)</option>
			<option value="20">2560x1440 (QHD)</option>
			<option value="21">2560x1600 (WQXGA)</option>
			<option value="22">1080x1920 (P_FHD)</option>
			<option value="23">2560x1920 (QSXGA)</option>
			<option value="24">2592x1944 (5MP)</option>
		</select>

		<label>Photo Attempt (0-10)</label>
		<input type="number" name="Attempt" min="0" max="10" value="3" step="1">
		<label>Quality (0-63)</label>
		<input type="number" name="Brightness" min="0" max="63" value="31" step="1">
		<label>Brightness (-2-2)</label>
		<input type="number" name="Brightness" min="-2" max="2" value="0" step="1">
		<label>Contrast (-2-2)</label>
		<input type="number" name="Contrast" min="-2" max="2" value="0" step="1">
		<label>Saturation (-2-2)</label>
		<input type="number" name="Saturation" min="-2" max="2" value="0" step="1">
		<label>Sharpness (-2-2)</label>
		<input type="number" name="Sharpness" min="-2" max="2" value="0" step="1">

		<div class="checkbox-option">
            <label for="vflip">Vflip</label>
            <input type="checkbox" id="vflip" name="Vflip" value="1" checked>
        </div>
        <div class="checkbox-option">
            <label for="hmirror">Hmirror</label>
            <input type="checkbox" id="hmirror" name="Hmirror" value="1">
        </div>
	</div>
</div>

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

void load_stored_prefs() {
    prefs.begin("config", true); // true = lecture seule
    
    wifi_ssid = prefs.getString("ssid", "None");
    wifi_password = prefs.getString("mdp", "None");
    Resolution = prefs.getUChar("Resolution", 6);
    photo_attempt = prefs.getUChar("Attempt", 3);
    quality = prefs.getUChar("Quality", 31);
    brightness = prefs.getUChar("Brightness", 0);
    contrast = prefs.getUChar("Contrast", 0);
    saturation = prefs.getUChar("Saturation", 0);
    sharpness = prefs.getUChar("Sharpness", 0);
    vflip = prefs.getBool("Vflip", true);
    hmirror = prefs.getBool("Hmirror", false);
    
    prefs.end();
}

void update_stored_prefs() {
    prefs.begin("config", false);
    
    if (wifi_ssid != prefs.getString("ssid", "None")) {
        prefs.putString("ssid", wifi_ssid);
    }
    if (wifi_password != prefs.getString("mdp", "None")) {
        prefs.putString("mdp", wifi_password);
    }
    if (Resolution != prefs.getUChar("Resolution", 6)) {
        prefs.putUChar("Resolution", Resolution);
    }
    if (photo_attempt != prefs.getUChar("Attempt", 3)) {
        prefs.putUChar("Attempt", photo_attempt);
    }
    if (quality != prefs.getUChar("Quality", 31)) {
        prefs.putUChar("Quality", quality);
    }
    if (brightness != prefs.getUChar("Brightness", 0)) {
        prefs.putUChar("Brightness", brightness);
    }
    if (contrast != prefs.getUChar("Contrast", 0)) {
        prefs.putUChar("Contrast", contrast);
    }
    if (saturation != prefs.getUChar("Saturation", 0)) {
        prefs.putUChar("Saturation", saturation);
    }
    if (sharpness != prefs.getUChar("Sharpness", 0)) {
        prefs.putUChar("Sharpness", sharpness);
    }
    if (vflip != prefs.getBool("Vflip", true)) {
        prefs.putBool("Vflip", vflip);
    }
    if (hmirror != prefs.getBool("Hmirror", false)) {
        prefs.putBool("Hmirror", hmirror);
    }
    
    prefs.end();
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
		Resolution = server.arg("Resolution").toInt();
		photo_attempt = server.arg("Attempt").toInt();
		quality = server.arg("Quality").toInt();
		brightness = server.arg("Brightness").toInt();
		contrast = server.arg("Contrast").toInt();
		saturation = server.arg("Saturation").toInt();
		sharpness = server.arg("Sharpness").toInt();
		vflip = server.arg("Vflip") == "1";
		hmirror = server.arg("Hmirror") == "1";

		Serial.print("AP ssid = "); Serial.println(wifi_ssid);
		Serial.print("AP password = "); Serial.println(wifi_password);
		Serial.print("Resolution = "); Serial.println(Resolution);
		Serial.print("Attempt = "); Serial.println(photo_attempt);
		Serial.print("Quality = "); Serial.println(quality);
		Serial.print("Brightness = "); Serial.println(brightness);
		Serial.print("Contrast = "); Serial.println(contrast);
		Serial.print("Saturation = "); Serial.println(saturation);
		Serial.print("Sharpness = "); Serial.println(sharpness);
		Serial.print("Vflip = "); Serial.println(vflip);
		Serial.print("Hmirror = "); Serial.println(hmirror);

		update_stored_prefs();

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

void update_parameters() {
	TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor, (framesize_t) Resolution); 	// Set frame size
	TimerCAM.Camera.sensor->set_quality(TimerCAM.Camera.sensor, 10); 	// Set frame size

	TimerCAM.Camera.sensor->set_brightness(TimerCAM.Camera.sensor, brightness); 	// Set frame size
	TimerCAM.Camera.sensor->set_contrast(TimerCAM.Camera.sensor, contrast); 	// Set frame size
	TimerCAM.Camera.sensor->set_saturation(TimerCAM.Camera.sensor, saturation); 	// Set frame size
	TimerCAM.Camera.sensor->set_sharpness(TimerCAM.Camera.sensor, sharpness); 	// Set frame size
	TimerCAM.Camera.sensor->set_vflip(TimerCAM.Camera.sensor, vflip);               // Vertical flip
	TimerCAM.Camera.sensor->set_hmirror(TimerCAM.Camera.sensor, hmirror);             // Horizontal mirror
}

void setup_camera() {
	// A CHANGER VERS UN BOUCLE AVEC TIMEOUT
	if (!TimerCAM.Camera.begin()) {                 // Initialize camera
		Serial.println("Camera Init Fail");         // Print error if camera init fails
		return;
	}
	Serial.println("Camera Init Success");          // Print success message
	TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor, PIXFORMAT_JPEG); // Set JPEG format
	update_parameters();
}

void setup() {
	// Setup of the TimerCam
	TimerCAM.begin(true);
	// TimerCAM.Power.setLed(128);
	delay(200);
	


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

	pinMode(BUTTON_LED_PIN, INPUT_PULLDOWN);

	WiFi.mode(WIFI_MODE_APSTA);

	load_stored_prefs();
	// wifi_ssid = "test";
	// wifi_password = "";

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
	setup_camera();
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
		esp_sleep_enable_timer_wakeup(20* 1000000);
		esp_deep_sleep_start();
		return;
	}
	if (WiFi.status() == WL_CONNECTED) {
		photo_attempt -= 1;
		// if (connected_timestamp == 0) {
		// 	connected_timestamp = millis();
		// }
		TimerCAM.Power.setLed(255);
		if (AP) {
			stop_ap();
			update_parameters();
		}
		Serial.println("Connected to Wi-Fi!");
		ticker.detach();
		// Serial.println(millis() - connected_timestamp);
		// if (millis() - connected_timestamp > 10000) {
		Serial.print("attempt left : "); Serial.println(photo_attempt);
		if (photo_attempt < 1) {
			Serial.println("Timeout connected");
			TimerCAM.Power.setLed(0);
			go_to_sleep = true;
			WiFi.disconnect();
			return;
		}
		// wait release?
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
	// Try to connect to MQTT
	mqttClient.connect("TimerCamClient", "esp32", "nichoir");
	delay(250);
	if (!mqttClient.connected()) {                  // Check MQTT connection
		Serial.print("Failed, rc=");            // Print error code
		Serial.print(mqttClient.state());
		delay(2000);                            // Wait before retrying
		return;
	}
	TimerCAM.Camera.free();                              // Free camera buffer
	delay(100);
	if (!TimerCAM.Camera.get()) {                   // Capture image
		Serial.println("Failed to capture image");  // Print error if capture fails
		return;
	}
	Serial.printf("Photo captured, size: %d bytes\n", TimerCAM.Camera.fb->len); // Print image size

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
	
	TimerCAM.Camera.free();                              // Free camera buffer

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

}