#include "M5TimerCAM.h"         // Library for M5TimerCAM
#include <WiFi.h>               // Library to manage Wi-Fi connection
#include <PubSubClient.h>       // MQTT client library
#include <WebServer.h>			// Embedded HTTP web server
#include <Preferences.h>		// Non-volatile storage (NVS) for saving configuration
#include <Ticker.h>

#define BUTTON_LED_PIN 13		// Button / LED pin (Yellow wire)
#define PIR_PIN 4				// PIR PIN (White wire)
bool mouvement = false;			// Indicates whether a motion-triggered wake-up occurred

Ticker ticker;					// Ticker used for LED blinking
volatile uint8_t led_pwm = 0;	// LED brightness level (PWM)

// AP configuration
const char* ap_ssid = "TimerCAM-AP";	// SSID of the configuration Access Point
const char* ap_password = "12345678";	// Password for the configuration Access Point
bool AP = false;						// Indicates whether Access Point mode is active

IPAddress local_IP(192,168,4,1);		// Static IP address of the Access Point
IPAddress gateway(192,168,4,1);			// Gateway IP address
IPAddress subnet(255,255,255,0);		// Subnet mask

// wifi Configuration
String wifi_ssid = "Test";				// Stored Wi-Fi SSID (station mode)
String wifi_password = "Test";			// Stored Wi-Fi password

bool connecting = false;				// Indicates whether the device is trying to connect to Wi-Fi
unsigned long start_connecting = 0;		// Timestamp when Wi-Fi connection attempt started
const unsigned long connect_timeout = 10000;	// Wi-Fi connection timeout in milliseconds (10 seconds)

int n = 0;								// Number of Wi-Fi networks found during scan

// MQTT Configuration
String mqtt_server = "192.168.1.60";				// MQTT broker IP address
const int mqtt_port = 1883;               			// MQTT broker port (default: 1883)

WiFiClient espClient;                               // Wi-Fi client used by the MQTT client
PubSubClient mqttClient(espClient);                 // MQTT client instance
const uint16_t max_size = 8192;                     // Maximum MQTT message buffer size (bytes)
const size_t CHUNK_SIZE = 4096;						// Maximum image chunck size

String mqtt_topic_start = "TimerCam/";				// Base MQTT topic for this device
String mqtt_topic_photo   = "/photo";				// Subtopic for photo data
String mqtt_topic_batterie   = "/batterie";			// Subtopic for battery information

bool photo_sent = false;							// Indicates whether the photo was successfully sent
bool battery_sent = false;							// Indicates whether the battery info was successfully sent

uint8_t battery_interval = 24;						// Maximum battery information interval (in hours)

// Web server
WebServer server(80);								// HTTP server running on port 80
Preferences prefs;
uint8_t Resolution;
uint8_t photo_attempt;
uint8_t quality;
int8_t brightness;
int8_t contrast;
int8_t saturation;
int8_t sharpness;
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
<br>
<label>Broker IP</label>
<input type="text" name="broker">

<input type="checkbox" id="advanced-toggle" class="advanced-toggle">
<label for="advanced-toggle" class="advanced-header">
	<span>Advanced settings</span>
	<span class="arrow">></span>
</label>
<div class="advanced-content">
	<div class="advanced-box">
		<label>Resolution</label>
		<select name="resolution">
			<option value="0">96x96</option>
			<option value="1">160x120 (QQVGA)</option>
			<option value="2">128x128</option>
			<option value="3">176x144 (QCIF)</option>
			<option value="4">240x176 (HQVGA)</option>
			<option value="5">240x240</option>
			<option value="6">320x240 (QVGA)</option>
			<option value="7"selected>320x320</option>
			<option value="8">400x296 (CIF)</option>
			<option value="9">480x320 (HVGA)</option>
			<option value="10">640x480 (VGA)</option>
			<option value="11">800x600 (SVGA)</option>
			<option value="12">1024x768 (XGA)</option>
			<option value="13">1280x720 (HD)</option>
			<option value="14">1280x1024 (SXGA)</option>
			<option value="15">1600x1200 (UXGA)</option>
			<option value="16">1920x1080 (FHD)</option>
		</select>

		<label>Photo Attempt (0-10)</label>
		<input type="text" inputmode="numeric" name="Attempt" min="1" max="10" value="3" step="1" required>
		<label>Quality (63-2)</label>
		<input type="text" inputmode="numeric" name="Quality" min="2" max="63" value="31" step="1" required>
		<label>Brightness (-2-2)</label>
		<input type="text" inputmode="numeric" name="Brightness" min="-2" max="2" value="0" step="1" required>
		<label>Contrast (-2-2)</label>
		<input type="text" inputmode="numeric" name="Contrast" min="-2" max="2" value="0" step="1" required>
		<label>Saturation (-2-2)</label>
		<input type="text" inputmode="numeric" name="Saturation" min="-2" max="2" value="0" step="1" required>
		<label>Sharpness (-2-2)</label>
		<input type="text" inputmode="numeric" name="Sharpness" min="-2" max="2" value="0" step="1" required>

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

void load_stored_prefs() {	// Function to read all saved parameters
	prefs.begin("config", true);						// true = read only
	
	wifi_ssid = prefs.getString("ssid", "None");
	wifi_password = prefs.getString("mdp", "None");
	mqtt_server = prefs.getString("broker", "None");

	Resolution = prefs.getUChar("Resolution", 6);
	photo_attempt = prefs.getUChar("Attempt", 3);
	quality = prefs.getUChar("Quality", 31);
	brightness = prefs.getChar("Brightness", 0);
	contrast = prefs.getChar("Contrast", 0);
	saturation = prefs.getChar("Saturation", 0);
	sharpness = prefs.getChar("Sharpness", 0);
	vflip = prefs.getBool("Vflip", true);
	hmirror = prefs.getBool("Hmirror", false);
	
	prefs.end();
}

void update_stored_prefs() {	// Function to update all changed parameters
	prefs.begin("config", false);
	
	if (wifi_ssid != prefs.getString("ssid", "None")) {
		prefs.putString("ssid", wifi_ssid);
	}
	if (wifi_password != prefs.getString("mdp", "None")) {
		prefs.putString("mdp", wifi_password);
	}
	if (mqtt_server != prefs.getString("broker", "None")) {
		prefs.putString("broker", mqtt_server);
	}
	if (Resolution != prefs.getUChar("Resolution", 6)) {
		prefs.putChar("Resolution", Resolution);
	}
	if (photo_attempt != prefs.getUChar("Attempt", 3)) {
		prefs.putChar("Attempt", photo_attempt);
	}
	if (quality != prefs.getUChar("Quality", 31)) {
		prefs.putChar("Quality", quality);
	}
	if (brightness != prefs.getChar("Brightness", 0)) {
		prefs.putChar("Brightness", brightness);
	}
	if (contrast != prefs.getChar("Contrast", 0)) {
		prefs.putChar("Contrast", contrast);
	}
	if (saturation != prefs.getChar("Saturation", 0)) {
		prefs.putChar("Saturation", saturation);
	}
	if (sharpness != prefs.getChar("Sharpness", 0)) {
		prefs.putChar("Sharpness", sharpness);
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
	// Build the HTML <option> list from scanned Wi-Fi networks
	String options = "";
	if (n <= 0) {
		options = "<option>No WiFi found</option>";
	} else {
		for (int i = 0; i < n; i++) {
			options += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) + "</option>";
		}
	}

	// Assemble the full configuration web page
	// (HTML header + Wi-Fi options + HTML footer)
	String fullPage = String(htmlFormHeader) + options + String(htmlFormFooter);

	// Send the configuration page to the client
	server.send(200, "text/html", fullPage);
}

void handleConnecting() {
	// Handle form submission (POST request)
	if (server.method() == HTTP_POST) {
		// Read network, MQTT, and camera parameters from the web form
		wifi_ssid = server.arg("ssid");
		wifi_password = server.arg("password");
		mqtt_server = server.arg("broker");

		Resolution = server.arg("resolution").toInt();
		photo_attempt = constrain(server.arg("Attempt").toInt(), 0, 10);
		quality = constrain(server.arg("Quality").toInt(), 2, 63);
		brightness = constrain(server.arg("Brightness").toInt(), -2, 2);
		contrast = constrain(server.arg("Contrast").toInt(), -2, 2);
		saturation = constrain(server.arg("Saturation").toInt(), -2, 2);
		sharpness = constrain(server.arg("Sharpness").toInt(), -2, 2);
		vflip = server.arg("Vflip") == "1";
		hmirror = server.arg("Hmirror") == "1";

		update_stored_prefs();			// Save the new configuration to non-volatile memory

		server.send(200, "text/html", htmlConnecting);		// Send a "connecting" page to the client

		// Start Wi-Fi connection attempt with the provided credentials
		WiFi.begin(wifi_ssid, wifi_password);
		start_connecting = millis();
	} else {
		// If the request is not POST, redirect to the configuration page
		handleRoot();
	}
}

void start_ap() {
	// Prevent restarting the Access Point if it is already active
	if (AP) return;
	AP = true;

	// Configure and start the Wi-Fi Access Point
	WiFi.softAPConfig(local_IP, gateway, subnet);
	WiFi.softAP(ap_ssid, ap_password);

	// Register HTTP routes for the configuration web interface
	server.on("/", handleRoot);
	server.on("/connecting", handleConnecting);

	// Start the web server
	server.begin();
}

void stop_ap() {
	// Do nothing if the Access Point is not running
	if (!AP) return;

	// Stop the Access Point and disconnect all clients
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
	uint64_t mac = ESP.getEfuseMac();	// Retrieve the unique ESP32 MAC address from eFuse memory
	uint32_t low = mac & 0xFFFFFF;		// Extract the lower 24 bits to generate a shorter unique identifier
	
	// Convert the identifier to a fixed-length hexadecimal string
	char id[7];
	sprintf(id, "%06X", low);
	return String(id);					// Return the short ID as a String
}

void update_parameters() {
	// Apply image resolution and JPEG quality settings
	TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor, (framesize_t) Resolution);
	TimerCAM.Camera.sensor->set_quality(TimerCAM.Camera.sensor, quality);

	// Apply image tuning parameters (brightness, contrast, saturation, sharpness)
	TimerCAM.Camera.sensor->set_brightness(TimerCAM.Camera.sensor, brightness);
	TimerCAM.Camera.sensor->set_contrast(TimerCAM.Camera.sensor, contrast);
	TimerCAM.Camera.sensor->set_saturation(TimerCAM.Camera.sensor, saturation);
	TimerCAM.Camera.sensor->set_sharpness(TimerCAM.Camera.sensor, sharpness);

	// Apply image orientation settings
	TimerCAM.Camera.sensor->set_vflip(TimerCAM.Camera.sensor, vflip);
	TimerCAM.Camera.sensor->set_hmirror(TimerCAM.Camera.sensor, hmirror);
}

bool setup_camera() {
	// Try to initialize the camera with a retry loop and timeout
	unsigned long camera_init_start = millis();
	while (!TimerCAM.Camera.begin()) {
		if (millis() - camera_init_start > 3000) {	// Abort initialization if timeout is reached
			return false;
		}
		
		delay(100);									// Short delay between initialization attempts
	}

	TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor, PIXFORMAT_JPEG); // Configure the camera to output JPEG images
	update_parameters();							// Apply stored camera configuration parameters
	return true;
}

void goToSleep() {
	// Stop network services and disable Wi-Fi
	server.stop();
	WiFi.softAPdisconnect(true);
	WiFi.disconnect(true);
	WiFi.mode(WIFI_OFF);

	// Turn off the led
	TimerCAM.Power.setLed(0);
	ticker.detach();

	// Schedule wake-up and power off the device
	if (battery_sent) {
		TimerCAM.Rtc.setAlarmIRQ(battery_interval * 3600);	// if battery level has been send wake up in 24 hours
	} else {
		TimerCAM.Rtc.setAlarmIRQ(1800);						// else wake up in 30 minutes
	}
	TimerCAM.Power.powerOff();
}

void setup() {
	digitalWrite(POWER_HOLD_PIN, HIGH);			// Keep power enabled after boot
	TimerCAM.begin(true);						// Initialize TimerCAM hardware (power, RTC, peripherals)
	delay(50);

	mouvement = digitalRead(PIR_PIN);			// Read motion state from PIR sensor

	// Configure MQTT client
	mqttClient.setServer(mqtt_server.c_str(), mqtt_port);
	mqttClient.setBufferSize(max_size);

	pinMode(BUTTON_LED_PIN, INPUT_PULLDOWN);	// Configure button pin (used to force AP mode at boot)

	WiFi.mode(WIFI_MODE_APSTA);					// Enable both Access Point and Station Wi-Fi modes

	load_stored_prefs();						// Load stored configuration from non-volatile memory
	// wifi_ssid = "Proximus-Home-06B0";
	// wifi_password = "wm24mn4snd6rh";

	// Decide between Access Point mode or normal Wi-Fi connection
	if (digitalRead(BUTTON_LED_PIN) == HIGH) {
		// Start configuration Access Point
		n = WiFi.scanNetworks();
		start_ap();
		ticker.attach(0.75, blink_cb);
	} else {
		// Attempt to connect to stored Wi-Fi network
		WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);
		WiFi.begin(wifi_ssid, wifi_password);
		connecting = true;
		start_connecting = millis();
	}

	// Reconfigure button pin as output
	pinMode(BUTTON_LED_PIN, OUTPUT);
	digitalWrite(BUTTON_LED_PIN, LOW);

	// Initialize camera and enter sleep mode if initialization fails
	if (!setup_camera()) {
		goToSleep();
	}
}

void loop() {
	// Handle Wi-Fi connection success
	if (WiFi.status() == WL_CONNECTED) {
		// Turn off the led
		ticker.detach();
		TimerCAM.Power.setLed(0);

		// Stop Access Point if it was enabled
		if (AP) {
			stop_ap();
			update_parameters();
		}

		// Check remaining photo attempts
		if (photo_attempt < 1) {
			goToSleep();
		}

		// Try to capture and send data
		photo_attempt -= 1;
		send_data();
	} else if (connecting) {	// Handle Wi-Fi connection timeout
		if (millis() - start_connecting > connect_timeout) {
			goToSleep();
		}
	}

	// Handle HTTP requests when Access Point is active
	if (AP) {
		server.handleClient();
	}

	delay(50);
}

void send_data() {
	// Attempt to connect to the MQTT broker
	if (!MQTT_connection()) {
		return;
	}
	
	// Skip photo capture if no motion was detected
	if (!mouvement) {
		photo_sent = true;
	}

	// Capture and send photo if not yet sent
	if (!photo_sent) {
		sendPhotoMQTT();
	}

	// Send battery information if not yet sent
	if (!battery_sent) {
		sendBatteryInfo();
	}

	// Check if both photo and battery data have been successfully sent
	if (photo_sent && battery_sent) {
		goToSleep();
	}
}

bool MQTT_connection() {
	// Create a unique client ID using the device short ID
	String clientId = "TimerCam-" + getShortID();

	// Attempt to connect to MQTT broker with username/password
	mqttClient.connect(clientId.c_str(), "esp32", "nichoir");
	delay(100);

	// Check if connection succeeded
	if (!mqttClient.connected()) {
		delay(2000);                            // Wait before retrying
		return false;
	}
	return true;
}

void sendPhotoMQTT() {
	digitalWrite(BUTTON_LED_PIN, HIGH);				// Turn the led ON
	delay(50);

	// Capture image from camera
	if (!TimerCAM.Camera.get()) {
		digitalWrite(BUTTON_LED_PIN, LOW);
		return;
	}
	digitalWrite(BUTTON_LED_PIN, LOW);				// Turn the led OFF

	// Prepare MQTT topic for sending the photo
	String PhotoBaseTopic = mqtt_topic_start + getShortID() + mqtt_topic_photo;

	size_t img_len = TimerCAM.Camera.fb->len;
	uint8_t* img_buf = TimerCAM.Camera.fb->buf;

	int chunkIndex = 0;
	bool sent;

	// Split image into chunks and send via MQTT
	for (size_t offset = 0; offset < img_len; offset += CHUNK_SIZE) {

		size_t chunkLen = min(CHUNK_SIZE, img_len - offset);
		String chunkTopic = PhotoBaseTopic + "/" + String(chunkIndex);

		sent = mqttClient.publish(chunkTopic.c_str(), img_buf + offset, chunkLen, false);

		if (!sent) {
			break;
		}

		chunkIndex++;
		mqttClient.loop();
		delay(10);
	}

	photo_sent = sent;
	mqttClient.loop();
	delay(200);
	mqttClient.publish((PhotoBaseTopic + "/end").c_str(), "end", 1);
	mqttClient.loop();
	
	TimerCAM.Camera.free();                              // Free camera buffer
}

void sendBatteryInfo() {
	// Read battery voltage
	uint16_t voltage = TimerCAM.Power.getBatteryVoltage();
	// level will be determined by the raspberry pi
  	// uint16_t level = TimerCAM.Power.getBatteryLevel();

	// Prepare MQTT topic for battery info
	String batterie = String(voltage);
	String BatterieTopic = mqtt_topic_start + getShortID() + mqtt_topic_batterie;

	// Publish battery voltage to MQTT
	mqttClient.loop();
	delay(100);
	battery_sent = mqttClient.publish(BatterieTopic.c_str(), batterie.c_str());
	delay(20);
	mqttClient.loop();

}