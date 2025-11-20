#include <WiFi.h>                 // Library to manage Wi-Fi connection
#include <PubSubClient.h>         // Library to manage MQTT

WiFiClient espClient;             // Create a Wi-Fi client for MQTT
PubSubClient client(espClient);   // Create an MQTT client using espClient

const char* ssid     = "SSID";                    // Current Wi-Fi SSID
const char* password = "password";                // Current Wi-Fi password

const char* mqtt_server = "IP";                   // MQTT broker IP address
const int mqtt_port = 1883;                       // MQTT port (default 1883)

void reconnect() {                                // Function to reconnect to the MQTT broker
  while (!client.connected()) {                   // While the client is not connected
    Serial.println("Connecting to MQTT broker...");               // Print message to Serial Monitor
    if (client.connect("TimerCamClient", "esp32", "nichoir")) {   // Attempt MQTT connection
      Serial.println("Connected!");               // Print message if connection succeeds
    } else {
      Serial.print("Failed, rc=");                // Print error code
      Serial.print(client.state());
      delay(2000);                                // Wait before retrying
    }
  }
}

void setup() {                                 // Initialization function
  Serial.begin(115200);                        // Start serial communication
  WiFi.begin(ssid, password);                  // Connect to Wi-Fi network

  while (WiFi.status() != WL_CONNECTED) {      // Wait until Wi-Fi is connected
    delay(500);                                // Small delay
    Serial.print(".");                         // Print a dot to indicate waiting
  }
  
  client.setServer(mqtt_server, mqtt_port);    // Set MQTT server and port
}

void loop() {                                  // Main loop
  if (!client.connected()) {                   // Check if MQTT client is connected
    reconnect();                               // If not, reconnect
  }
  client.loop();                               // Keep MQTT connection alive

  client.publish("test", "Message from TimerCam"); // Publish MQTT message
  Serial.println("test");                      // Print "test" to Serial Monitor
  delay(5000);                                 // Wait 5 seconds before next message
}
