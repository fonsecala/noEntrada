#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <TZ.h>
#include <FS.h>
#include <LittleFS.h>
#include <CertStoreBearSSL.h>
#include <ArduinoJson.h>


//Libraries added in second tutorial
//Variable declarations
int board;
int pin;
int pin_status;
String message = "";
bool messageReady = false;
bool teste = true;

/////////////////////////////////////////////////////////////////////////



// Update these with values suitable for your network.
const char* ssid = "nodemcu";
const char* password = "zolami1234";
const char* mqtt_server = "d37d3fa91de340a69dc9ce989bf5798b.s1.eu.hivemq.cloud";

// A single, global CertStore which can be used by all connections.
// Needs to stay live the entire time any of the WiFiClientBearSSLs
// are present.
BearSSL::CertStore certStore;
WiFiClientSecure espClient;
PubSubClient * client;
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (500)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void setDateTime() {
  // You can use your own timezone, but the exact time is not used at all.
  // Only the date is needed for validating the certificates.
  configTime(TZ_Europe_Berlin, "pool.ntp.org", "time.nist.gov");

  // Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(100);
    // Serial.print(".");
    now = time(nullptr);
  }
  // Serial.println();

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  // Serial.printf("%s %s", tzname[0], asctime(&timeinfo));
}


void callback(char* topic, byte* payload, unsigned int length) {
  // Serial.print("Message arrived [");
  // Serial.print(topic);
  // Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  // Serial.println();

  // Switch on the LED if the first character is present
  if ((char)payload[0] != NULL) {
    digitalWrite(LED_BUILTIN, LOW); // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
  } else {
    digitalWrite(LED_BUILTIN, HIGH); // Turn the LED off by making the voltage HIGH
  }
}


void reconnect() {
  // Loop until we’re reconnected
  while (!client->connected()) {
    // Serial.print("Attempting MQTT connection…");
    String clientId = "ESP8266Client - MyClient";
    // Attempt to connect
    // Insert your password
    if (client->connect(clientId.c_str(), "testeWebsocket", "240412Ab")) {
      // Serial.println("connected");
      // Once connected, publish an announcement…
      client->publish("testTopic", "hello world");
      // … and resubscribe
      client->subscribe("testTopic");
    } else {
      // Serial.print("failed, rc = ");
      // Serial.print(client->state());
      // Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  delay(500);
  // When opening the Serial Monitor, select 9600 Baud
  Serial.begin(9600);
  delay(500);

  LittleFS.begin();
  setup_wifi();
  setDateTime();

  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED_BUILTIN pin as an output

  // you can use the insecure mode, when you want to avoid the certificates
  //espclient->setInsecure();

  int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
  // Serial.printf("Number of CA certs read: %d\n", numCerts);
  if (numCerts == 0) {
    // Serial.printf("No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?\n");
    return; // Can't connect to anything w/o certs!
  }

  BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();
  // Integrate the cert store with this connection
  bear->setCertStore(&certStore);

  client = new PubSubClient(*bear);

  client->setServer(mqtt_server, 8883);
  client->setCallback(callback);
}

void loop() {
  if (!client->connected()) {
    reconnect();
  }
  client->loop();
  
  ///////////////////////////////////
  while (Serial.available())
  {
    message = Serial.readString();
    messageReady = true;
  }
  //Only process message if there's one
  if(messageReady){
    //The only messsages we'll parse will be formatted in JSON
    DynamicJsonDocument doc(1024);
    //Attempt to deserialize the message
    DeserializationError error = deserializeJson(doc, message);
    if(error){
      messageReady = false;
      return;
    }
    //If request is received from noPrincipalMesh
    if(doc["type"] == "request"){
      //mensagem recebida
      doc["type"] = "response";
      //Get data from virtual pin
      doc["board_status"] = board;
      doc["led"] = pin;
      doc["status"] = pin_status;
      serializeJson(doc, Serial); //Send data to noPrincipalMesh
    }
    messageReady = false;
  }

  /////////////////////////////////////
}