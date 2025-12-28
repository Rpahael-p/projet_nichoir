#include "M5TimerCAM.h"           // Library for M5TimerCAM
#include <WiFi.h>                 // Library to manage Wi-Fi connection
#include <PubSubClient.h>         // Library to manage MQTT

// === WIFI CONFIG ===
const char* ssid     = "SSID";                      // Wi-Fi SSID
const char* password = "password";                  // Wi-Fi password

// === MQTT CONFIG ===
const char* mqtt_server       = "IP";               // MQTT broker IP address
const int   mqtt_port         = 1883;               // MQTT port (default 1883)
const char* mqtt_topic_text   = "TimerCam/text";    // MQTT topic for text messages
const char* mqtt_topic_photo  = "TimerCam/photo";   // MQTT topic for photos

WiFiClient espClient;                               // Create Wi-Fi client for MQTT
PubSubClient mqttClient(espClient);                 // Create MQTT client using espClient
uint16_t max_size = 8192;                           // Maximum MQTT message buffer size

void reconnect() {                                  // Function to reconnect to MQTT broker
    while (!mqttClient.connected()) {               // While MQTT client is not connected
        Serial.print("Connecting to MQTT broker..."); // Print message to Serial Monitor
        if (mqttClient.connect("TimerCamClient", "esp32", "nichoir")) { // Attempt MQTT connection
            Serial.println("Connected!");           // Print if connection succeeds
        } else {
            Serial.print("Failed, rc=");            // Print error code
            Serial.print(mqttClient.state());
            delay(2000);                            // Wait before retrying
        }
    }
}

void setup() {                                      // Initialization function
    Serial.begin(115200);                           // Start serial communication
    TimerCAM.begin();                               // Initialize TimerCAM

    if (!TimerCAM.Camera.begin()) {                 // Initialize camera
        Serial.println("Camera Init Fail");         // Print error if camera init fails
        return;
    }
    Serial.println("Camera Init Success");          // Print success message

    TimerCAM.Camera.sensor->set_pixformat(TimerCAM.Camera.sensor, PIXFORMAT_JPEG); // Set JPEG format
    TimerCAM.Camera.sensor->set_framesize(TimerCAM.Camera.sensor, FRAMESIZE_QVGA); // Set frame size
    TimerCAM.Camera.sensor->set_vflip(TimerCAM.Camera.sensor, 1);                  // Vertical flip
    TimerCAM.Camera.sensor->set_hmirror(TimerCAM.Camera.sensor, 0);                // Horizontal mirror

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);                  // Start Wi-Fi connection
    WiFi.setSleep(false);                        // Disable Wi-Fi sleep
    Serial.print("Connecting to WiFi");          // Print message
    while (WiFi.status() != WL_CONNECTED) {      // Wait for connection
        delay(500);                              // Small delay
        Serial.print(".");                       // Print dot for waiting
    }
    Serial.println();
    Serial.print("Connected. IP: ");             // Print connected IP
    Serial.println(WiFi.localIP());

    // Setup MQTT
    mqttClient.setServer(mqtt_server, mqtt_port); // Set MQTT server and port
    mqttClient.setBufferSize(max_size);           // Set MQTT buffer size
}

void loop() {                                  // Main loop
    if (!mqttClient.connected()) {             // Check MQTT connection
        reconnect();                           // Reconnect if needed
    }
    mqttClient.loop();                          // Maintain MQTT connection

    sendPhotoMQTT();                            // Capture and send photo via MQTT
    delay(5000);                                // Wait 5 seconds
}

void sendPhotoMQTT() {                              // Function to capture and send photo
    if (!TimerCAM.Camera.get()) {                   // Capture image
        Serial.println("Failed to capture image");  // Print error if capture fails
        return;
    }
    Serial.printf("Photo captured, size: %d bytes\n", TimerCAM.Camera.fb->len); // Print image size

    // Publish image via MQTT
    if (!mqttClient.connected()) {                  // Check MQTT connection
        reconnect();                                // Reconnect if needed
    }
    mqttClient.publish(mqtt_topic_text, "Message from TimerCam"); // Send text message
    bool sent = mqttClient.publish(mqtt_topic_photo, TimerCAM.Camera.fb->buf, TimerCAM.Camera.fb->len, false); // Send photo

    if (sent) {                                // Check if photo sent successfully
        Serial.printf("Photo sent via MQTT, size: %d bytes\n", TimerCAM.Camera.fb->len);
    } else {
        Serial.println("Failed to send photo via MQTT"); // Print error if failed
    }

    TimerCAM.Camera.free();                              // Free camera buffer
}
