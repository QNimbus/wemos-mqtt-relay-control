#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <uptime_formatter.h>

// Load settings from header files
#include "credentials.h"
#include "settings.h"
#include "mqtt.h"

char ipAddress[16];
unsigned long lastTemperatureMessage = 0;
unsigned long lastActivity = 0;

// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
// Data wire is plugged into GPIO5 (D1) on the Wemos D1
OneWire oneWire(TMP_SENSOR_PIN);

DallasTemperature sensors(&oneWire);

WiFiClient espClient;
PubSubClient client(espClient);

void setupWiFi() {
  byte previousLEDState = digitalRead(LED_BUILTIN);
    
  // Connect to the WiFi network
  Serial.println();
  Serial.print("Connecting to WiFi network: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Attempt to connect and flash LED twice per second
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);    
    delay(250);
  }

  // Connected
  WiFi.localIP().toString().toCharArray(ipAddress, 16);
  digitalWrite(LED_BUILTIN, previousLEDState);
  Serial.println("");
  Serial.print("WiFi connected ");
  Serial.print("IPv4: ");
  Serial.println(WiFi.localIP().toString().c_str());
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  lastActivity = millis();
  digitalWrite(ACTIVITY_LED_PIN, HIGH);

  // Log received MQTT message
  Serial.print("MQTT message received [");
  Serial.print(topic);
  Serial.print("] with payload: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
  }
  Serial.println();

  if (strcmp(topic, RELAY_LED_TOPIC"/set")==0) {
    // Switch on the LED if an 1 was received as first character
    if ((char) payload[0] == '1') {
      digitalWrite(RELAY_LED_PIN, HIGH);
      client.publish(RELAY_LED_TOPIC, "1", true);
    } else if ((char) payload[0] == '0') {
      digitalWrite(RELAY_LED_PIN, LOW);
      client.publish(RELAY_LED_TOPIC, "0", true);
    }    
  } else if (strcmp(topic, RELAY_PWR_TOPIC"/set")==0) {
    // Switch on the LED if an 1 was received as first character
    if ((char) payload[0] == '1') {
      digitalWrite(RELAY_PWR_PIN, HIGH);
      client.publish(RELAY_PWR_TOPIC, "1", true);
    } else if ((char) payload[0] == '0') {
      digitalWrite(RELAY_PWR_PIN, LOW);
      client.publish(RELAY_PWR_TOPIC, "0", true);
    }    
  }
}

void reconnect() {
  byte previousLEDState = digitalRead(LED_BUILTIN);
  
  // Loop until we're reconnected  
  while (!client.connected()) {
    digitalWrite(LED_BUILTIN, LOW);

    // Attempt to connect
    Serial.print("Attempting MQTT connection...");    
    if (client.connect(MQTT_CLIENT_ID, mqtt_username, mqtt_password, WILL_TOPIC, 0, true, "offline")) {
      Serial.println("connected");
      client.publish(WILL_TOPIC, "online", true);
      client.publish(SSID_TOPIC, ssid, true);
      client.publish(IPV4_TOPIC, ipAddress, true);
      client.publish(RELAY_LED_TOPIC"/set", "", true);
      client.publish(RELAY_PWR_TOPIC"/set", "", true);
      client.subscribe(RELAY_LED_TOPIC"/set");
      client.subscribe(RELAY_PWR_TOPIC"/set");

      // Reset built-in LED
      digitalWrite(LED_BUILTIN, previousLEDState);

      // Read initial state of RELAY_LED_PIN
      if (digitalRead(RELAY_LED_PIN) == HIGH) {
        client.publish(RELAY_LED_TOPIC, "1");        
      } else {
        client.publish(RELAY_LED_TOPIC, "0");        
      }

      // Read initial state of RELAY_PWR_PIN
      if (digitalRead(RELAY_PWR_PIN) == HIGH) {
        client.publish(RELAY_PWR_TOPIC, "1");        
      } else {
        client.publish(RELAY_PWR_TOPIC, "0");        
      }
    } else {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  // Setup serial monitor
  Serial.begin(115200);
  
  // Setup GPIO pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ACTIVITY_LED_PIN, OUTPUT);
  pinMode(RELAY_LED_PIN, OUTPUT);
  pinMode(RELAY_PWR_PIN, OUTPUT);
  pinMode(TMP_SENSOR_PIN, INPUT);

  // Connect to WiFi
  setupWiFi(); 

  // Setup MQTT connection
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);

  // Setup sensors
  sensors.begin();
}

void loop()
{
  // Initialize temperature float
  float fTemperature = 0;
  // Get current time in millis
  unsigned long now = millis();

  // Turn off builtin LED
  digitalWrite(LED_BUILTIN, HIGH);

  // Switch activity LED off after ~300ms
  if (digitalRead(ACTIVITY_LED_PIN) == HIGH && now - lastActivity > 300) {
    digitalWrite(ACTIVITY_LED_PIN, LOW);
  }

  // Attempt to connect/reconnect to the MQTT broker
  if (!client.connected()) {
    reconnect();
  }
  
  // Run MQTT client
  client.loop();

  // Send temperature message and update 'uptime' every minute
  if (lastTemperatureMessage == 0 || now - lastTemperatureMessage > 60000) {
    String sTemperature;
    
    lastTemperatureMessage = now;
    sensors.setResolution(12);
    sensors.requestTemperatures(); // Send the command to get temperatures
    fTemperature = sensors.getTempCByIndex(0); // Use the getTempCByIndex to get the temperature from the first (only) sensor
    sTemperature = String(fTemperature, 2);
//    Serial.println(fTemperature);
//    String sTemperature = String("21.6");

    // Log published MQTT message
    client.publish(TEMPERATURE_TOPIC, sTemperature.c_str(), false);
    Serial.print("MQTT message published [");
    Serial.print(TEMPERATURE_TOPIC);
    Serial.print("] with payload: ");
    Serial.print(sTemperature.c_str());
    Serial.println();

    // Log published MQTT message
    client.publish(UPTIME_TOPIC, uptime_formatter::getUptime().c_str(), false);
    Serial.print("MQTT message published [");
    Serial.print(UPTIME_TOPIC);
    Serial.print("] with payload: ");
    Serial.print(uptime_formatter::getUptime().c_str());
    Serial.println();    
  }
}
