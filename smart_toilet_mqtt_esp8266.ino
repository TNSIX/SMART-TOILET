#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "ctime"
#include "time.h"
#include <NTPClient.h>
#include <WiFiUdp.h>

//const char* ssid = "Gun";            // Network Name
//const char* password = "HelloWorld";  // Password

const char* ssid = "TNSIIX";            // Network Name
const char* password = "3.1415926535";  // Password

const char* mqtt_Client = "6ce767fb-4b69-4f45-93e7-cf5b8deec4a5";  // Device ID
const char* mqtt_username = "3FJ6zHn4rsu3zZkrPwPEnM9XvEMmkY1C";    // Key
const char* mqtt_password = "FZDW1Vj1RUR5BtMRBxDD1HB2nm43nbdP";    // Secret

const char* mqtt_server = "broker.netpie.io";
const int mqtt_port = 1883;

int blueLED = D2;
int redLED = D4;
int sensor = D8;
int relay = D7;
int volume = 0;
int count = 0;
int flushingTime = 5000;
time_t timestamp;
char timeStr[20]; 

WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);
char msg[256];

void setup_wifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP: " + WiFi.localIP().toString());
}

void reconnect() {
  while (!client.connected() && WiFi.status() == WL_CONNECTED) { 
    Serial.print("Attempting MQTT connection…");
    if (client.connect(mqtt_Client, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("@msg/#");  
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" - retrying in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
}

void setup() {
  /*----------TODO CODE HERE----------*/
  pinMode(blueLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(sensor, INPUT);
  pinMode(relay, OUTPUT);
  Serial.begin(115200);
  delay(10);
  digitalWrite(relay, LOW);
  /*----------------------------------*/
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  time_t now;
  struct tm timeinfo;
  // Check WiFi status and reconnect if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Attempting to reconnect...");
    setup_wifi();
  }
  // Check MQTT connection
  if (!client.connected() && WiFi.status() == WL_CONNECTED) {
    reconnect();
  }
  client.loop();

  /*----------TODO CODE HERE----------*/
  bool value = digitalRead(sensor);
  if (value == LOW) {
    Serial.println("Detected Motion!");
    digitalWrite(blueLED, HIGH);
    digitalWrite(redLED, LOW);
    // Wait until sensor turns off
    while (digitalRead(sensor) == LOW) {
      delay(4000);
    }
    Serial.println("Activating Flush!");

    /*--- Time ---*/
    timeClient.update();                                  // Get current time from NTP
    unsigned long epochTime = timeClient.getEpochTime();  // Unix timestamp
    struct tm* timeinfo;
    time_t now = epochTime;
    timeinfo = localtime(&now);

    char timeBuffer[30];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    /*------------*/

    digitalWrite(relay, HIGH);
    delay(flushingTime);  // Volume: 25 mL per sec
    count++;
    volume += (flushingTime / 1000) * 25;

    /*--- Publish data to NETPIE shadow ---*/
    String data = "{\"data\":{\"Count\":" + String(count) + ",\"volume\":" + String(volume) + ",\"Date-n-Time\":\"" + String(timeBuffer) + "\"}}";
    Serial.println("Publishing: " + data);
    data.toCharArray(msg, data.length() + 1);
    client.publish("@shadow/data/update", msg);
    /*-------------------------------------*/
  } else {
    digitalWrite(relay, LOW);
    digitalWrite(blueLED, LOW);
    digitalWrite(redLED, HIGH);
    delay(250);
  }
  /*----------------------------------*/
}
