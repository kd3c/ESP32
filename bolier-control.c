#include <WiFi.h>
#include <PubSubClient.h>  // Library for MQTT
#include <Wire.h>
#include <ArduinoJson.h>
//___________________________________________________________________________//
// Replace the next variables with your SSID/Password combination
const char* ssid = "";
const char* password = "";
//___________________________________________________________________________//
// Add your MQTT Broker IP address and credentials
const char* mqtt_server = "";
const char* mqtt_username = "";
const int mqtt_port = 1883;
const char* mqtt_password = "";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
// relay Pin
const int relayPin = 2;
//___________________________________________________________________________//
#define debug_topic "debug"    //Topic for debugging
bool debug = true;             //Display log message if True
//___________________________________________________________________________//
void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setCallback(callback);
  pinMode(relayPin, OUTPUT);
}
//___________________________________________________________________________//
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  bool connected = false;
  bool myWiFiFirstConnect = true;
  while (connected != true){
    wl_status_t state;
    state = WiFi.status();
      if (state != WL_CONNECTED) {  // We have no connection
        if (state == WL_NO_SHIELD) {  // WiFi.begin wasn't called yet
          Serial.println("Connecting WiFi");
          WiFi.mode(WIFI_STA);
          WiFi.begin(ssid, password);
        } else if (state == WL_CONNECT_FAILED) {  // WiFi.begin has failed (AUTH_FAIL)
          Serial.println("Disconnecting WiFi");
          WiFi.disconnect(true);
        } else if (state == WL_DISCONNECTED) {  // WiFi.disconnect was done or Router.WiFi got out of range
          if (!myWiFiFirstConnect) {  // Report only once
            myWiFiFirstConnect = true; 
            Serial.println("WiFi disconnected");
          }
              vTaskDelay (250); // Check again in about 250ms
      } else { // We have connection
        if (myWiFiFirstConnect) {  // Report only once
          myWiFiFirstConnect = false;
          connected = true;
          delay(1000);
          Serial.print("Connected to ");
          Serial.println(ssid);
          Serial.print("IP address: ");
          Serial.println(WiFi.localIP());
          Serial.println("");
          client.setServer(mqtt_server, mqtt_port);
        }
        vTaskDelay (5000); // Check again in about 5s
    }
  }
 }
}
//___________________________________________________________________________//
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  // Feel free to add more if statements to control more GPIOs with MQTT
  // Changes the output state according to the message
  StaticJsonBuffer<300> JSONbuffer;
  JsonObject& JSONencoder = JSONbuffer.createObject();
  JSONencoder["device"] = "boiler_relay";
  JSONencoder["online"] = "true";
  if (String(topic) == "boiler_control") {
    Serial.print("Changing output to ");
    if(messageTemp == "on"){        
      Serial.println("ON message received");
      digitalWrite(relayPin, HIGH);
     }
    if(messageTemp == "off"){        
      Serial.println("OFF message received");
      digitalWrite(relayPin, LOW);
     }
     if(digitalRead(2) == 1){
        JSONencoder["heating_on"] = "true";
     } else {
        JSONencoder["heating_on"] = "false";
     }
     char JSONmessageBuffer[100];
     JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
     client.publish("boiler",JSONmessageBuffer);
   }
}
//___________________________________________________________________________//
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    digitalWrite(relayPin, LOW);
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //Last Will section to create JSON
    StaticJsonBuffer<300> JSONlastWillBuffer;
    JsonObject& JSONlastWill = JSONlastWillBuffer.createObject();
    JSONlastWill["device"] = "boiler_relay";
    JSONlastWill["online"] = "false";
    JSONlastWill["heating_on"] = "false";
    char JSONlastWillMessageBuffer[100];
    JSONlastWill.printTo(JSONlastWillMessageBuffer, sizeof(JSONlastWillMessageBuffer));
    if (client.connect(clientId.c_str(),mqtt_username,mqtt_password, "boiler" ,0, 1,JSONlastWillMessageBuffer)) {    
      Serial.println("connected");
      // Once connected, publish an announcement...
      StaticJsonBuffer<300> JSONbuffer;
      JsonObject& JSONencoder = JSONbuffer.createObject();
      JSONencoder["device"] = "boiler_relay";
      JSONencoder["online"] = "true";
      Serial.print("digitalRead(2): ");
      Serial.println(digitalRead(2));
      if(digitalRead(2)==1){
        JSONencoder["heating_on"] = "true";
      }
      if(digitalRead(2)==0){
        JSONencoder["heating_on"] = "false";
      }
      char JSONmessageBuffer[100];
      JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
      client.publish("boiler", JSONmessageBuffer);  //Publish after recconection to MQTT was successful
      // ... and resubscribe
      client.subscribe("boiler_control");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
//___________________________________________________________________________//
void loop() {
  if (WiFi.status() == WL_DISCONNECTED) {
    setup_wifi();  
    }
  if (!client.connected()) {
    reconnect();
  }
    client.loop();
    long now = millis();
}