# PubSubclient : librairie mqtt

source : [PubSubClient documentation](http://pubsubclient.knolleary.net/api)

### States

int state ()
Returns the current state of the client. If a connection attempt fails, this can be used to get more information about the failure.

All of the values have corresponding constants defined in PubSubClient.h.

Returns :  
-4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time  
-3 : MQTT_CONNECTION_LOST - the network connection was broken  
-2 : MQTT_CONNECT_FAILED - the network connection failed  
-1 : MQTT_DISCONNECTED - the client is disconnected cleanly  
0 : MQTT_CONNECTED - the client is connected  
1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT  
2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier  
3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection  
4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected  
5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect  
