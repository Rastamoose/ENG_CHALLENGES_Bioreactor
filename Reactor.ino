/*
 * Arduino Nano ESP32 - Connectivity Gateway
 * 
 * Incoming (from other board via Serial):
 *   Temperature : 18.00 – 30.00 °C
 *   pH          : 3.0 – 7.5
 *   Motor RPM   : 500 – 1500
 *
 * Outgoing (to MQTT broker):
 *   Temperature, pH, Motor RPM, Time elapsed (ms)
 */

#include <WiFi.h>
#include <PubSubClient.h>

// ====================== CONFIGURATION ======================
const char* WIFI_SSID     = "YousufHotspot";
const char* WIFI_PASSWORD = "yousufmiah1";

// Local Mosquitto broker
const char* MQTT_HOST     = "10.97.249.59";   // your laptop’s IP
const int   MQTT_PORT     = 1883;
const char* MQTT_USER     = "bioreactor";
const char* MQTT_PASSWORD = "bioreactor123";

const char* TOPIC_DATA     = "bioreactor/sensors/data";
const char* TOPIC_STATUS   = "bioreactor/status";
const char* TOPIC_COMMANDS = "bioreactor/commands/#";

// Expected ranges (for reference / light validation)
const float TEMP_MIN = 18.0;
const float TEMP_MAX = 30.0;
const float PH_MIN   = 3.0;
const float PH_MAX   = 7.5;
const int   RPM_MIN  = 500;
const int   RPM_MAX  = 1500;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

String serialBuffer = "";
unsigned long startTime = 0;

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("MQTT Command [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  // Forward the command to the other board
  Serial2.println(message);
}

void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT...");
    String clientId = "NanoESP32-" + String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str(), TOPIC_STATUS, 1, true, "offline")) {
      Serial.println("connected");
      mqtt.publish(TOPIC_STATUS, "online", true);
      mqtt.subscribe(TOPIC_COMMANDS);
      Serial.println("Subscribed to commands");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" retrying...");
      delay(5000);
    }
  }
}

void handleIncomingData() {
  while (Serial2.available()) {
    char c = Serial2.read();

    if (c == '\n') {
      serialBuffer.trim();

      if (serialBuffer.length() > 0) {
        float temp, ph;
        int rpm;

        // Expected format: 25.4,6.8,1200
        if (sscanf(serialBuffer.c_str(), "%f,%f,%d", &temp, &ph, &rpm) == 3) {

          // Light range check (optional but useful)
          bool valid = true;
          if (temp < TEMP_MIN || temp > TEMP_MAX) valid = false;
          if (ph   < PH_MIN   || ph   > PH_MAX)   valid = false;
          if (rpm  < RPM_MIN  || rpm  > RPM_MAX)  valid = false;

          unsigned long elapsed = millis() - startTime;

          // Outgoing format: temp,ph,rpm,elapsed
          String outgoing = String(temp, 2) + "," +
                            String(ph, 2) + "," +
                            String(rpm) + "," +
                            String(elapsed);

          if (valid) {
            Serial.print("Outgoing (valid): ");
            Serial.println(outgoing);
            mqtt.publish(TOPIC_DATA, outgoing.c_str());
          } else {
            Serial.print("Outgoing (out of range): ");
            Serial.println(outgoing);
            // Still publish, but you can choose to ignore later
            mqtt.publish(TOPIC_DATA, outgoing.c_str());
          }

        } else {
          Serial.print("Bad data format: ");
          Serial.println(serialBuffer);
        }
      }
      serialBuffer = "";
    } else {
      serialBuffer += c;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);        // Link to the other board

  delay(1000);
  Serial.println("\n=== Nano ESP32 Gateway Starting ===");
  Serial.println("Waiting for: Temp, pH, RPM");

  startTime = millis();
  connectWiFi();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  mqtt.setKeepAlive(60);

  connectMQTT();
}

void loop() {
  if (!mqtt.connected()) {
    connectMQTT();
  }
  mqtt.loop();
  handleIncomingData();
}