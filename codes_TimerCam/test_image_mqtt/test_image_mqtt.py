import os
import paho.mqtt.client as mqtt
import config

# =========================
# CONFIGURATION
# =========================
BROKER_IP       = config.BROKER_IP                # MQTT broker IP address
BROKER_PORT     = config.BROKER_PORT              # MQTT broker port

PHOTO_SAVE_DIR  = config.PHOTO_SAVE_DIR           # Directory to save captured photos

TOPIC_TEXT      = "TimerCam/text"                 # MQTT topic for text messages
TOPIC_PHOTO     = "TimerCam/photo"                # MQTT topic for photos

os.makedirs(PHOTO_SAVE_DIR, exist_ok=True)        # Create directory if it does not exist

# =========================
# CALLBACKS
# =========================
def on_connect(client, userdata, flags, rc):      
    if rc == 0:                                    
        print("Connected to broker!")                   # Print success message
        client.subscribe("#")                           # Subscribe to all topics
        print(f"Subscribed to topics: {TOPIC_TEXT} and {TOPIC_PHOTO}")
    else:
        print("Connection error, code:", rc)            # Print error code if connection fails

def on_TC_image(client, userdata, msg):
    filename = os.path.join(PHOTO_SAVE_DIR, f"photo_{int(client._last_msg_in)}.jpg") # Generate filename
    with open(filename, "wb") as f:
        f.write(msg.payload)                            # Save binary photo to file
    print(f"Saved image: {filename}, size: {len(msg.payload)} bytes") # Print info

def on_TC_message(client, userdata, msg): 
    try:
        text = msg.payload.decode('utf-8')              # Decode payload as UTF-8
        print(f"Received text from TimerCam: {text}")   # Print received text
    except UnicodeDecodeError:
        print("Failed to decode received text.")        # Handle decode error

def on_message(client, userdata, msg): 
    try:
        text = msg.payload.decode('utf-8')              # Decode payload as UTF-8
        print(f"Message receives from topic : {msg.topic}") # Print the topic
        print(f"Received text: {text}")                 # Print received text
    except UnicodeDecodeError:
        print("Failed to decode received text.")        # Handle decode error

# =========================
# MQTT CLIENT
# =========================
client = mqtt.Client()                             # Create MQTT client
client.on_connect = on_connect                     # Assign on_connect callback
client.on_message = on_message                     # Assign on_message callback

client.message_callback_add("TimerCam/text", on_TC_message)   # Add specific callback for text topic
client.message_callback_add("TimerCam/photo", on_TC_image)    # Add specific callback for photo topic

client.username_pw_set("esp32", "nichoir")         # Set MQTT username and password
client.connect(BROKER_IP, BROKER_PORT, 60)         # Connect to broker
client.loop_forever()                              # Start infinite loop to process MQTT messages
