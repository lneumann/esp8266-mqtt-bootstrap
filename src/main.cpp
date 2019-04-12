#include "Arduino.h"
#include "ESP8266WiFi.h"
#include "PubSubClient.h"

WiFiClient   wifi_client;
PubSubClient mqtt_client(wifi_client);
const char* sub_topic = "bootstrap/led";
const char* pub_topic = "bootstrap/millis";
const long sleep_send = 1000;
long last_send = -sleep_send; // send first time

void error() {
  Serial.println(" failed");
  Serial.println("**RESTARTING**");

  for (int i = 0; i < 10; i++)
  {
    digitalWrite(LED, i % 2);
    delay(500);
  }
  
  ESP.restart(); // reboot, force reconnect
}

void setup_wifi() {
  Serial.print("(Wifi) Connecting to ");
  Serial.print(WIFI_SSID);
  Serial.print("...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    tries++;
    if (tries >= 20) {
      error();
    }
    Serial.print(".");
    delay(500);
  }

  Serial.println(" connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("(Mqtt) Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  auto state = ((char)payload[0] == '0') ? HIGH : LOW;
  digitalWrite(LED, state);
}


void reconnect_mqtt() {
  Serial.print("(Mqtt) Connecting to ");
  Serial.print(MQTT_SERVER);
  Serial.print("...");
  
  int tries = 0;
  while (!mqtt_client.connect(MQTT_CLIENT_ID)) {
    tries++;
    if (tries >= 10) {
      error();
    }
    
    Serial.print(".");
    delay(500);
  }
  Serial.println(" connected");
  mqtt_client.subscribe(sub_topic); //resubscribe
}

char buf[50];
void send(const char* channel, long value) { 
  Serial.print("(Mqtt) Sending to ");
  Serial.print(channel); 
  Serial.print("...");
 
  ltoa(value, buf, 10);
  if (mqtt_client.publish(channel, buf)) {
    Serial.println(" ok");
  } else {
    Serial.println(" failed");
  }
}

void setup_mqtt() {
  mqtt_client.setServer(MQTT_SERVER, 1883);
  mqtt_client.setCallback(callback);
}

void setup()
{
  pinMode(LED, OUTPUT);
  Serial.begin(9600);

  setup_wifi();
  setup_mqtt();
}

void loop()
{
  if (!mqtt_client.connected()) {
    reconnect_mqtt();
  }
  mqtt_client.loop();

  long now = millis();
  if (now - last_send > sleep_send) {
    send(pub_topic, now);
    last_send = now;
  }
}
