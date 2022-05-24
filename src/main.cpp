#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <TZ.h>
#include <FS.h>
#include <LittleFS.h>
#include <CertStoreBearSSL.h>
#include <ArduinoJson.h>
#include <string.h>

// Update these with values suitable for your network.
const char* ssid = "hsNCE";
const char* password = "";
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
String message;
bool messageReady = false;

// const char mychar[11] = "outro"; //Teste topico para debug

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
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
  char string[length];
  for (unsigned int i = 0; i < length; i++) {
    string[i] = (char)payload[i]; 
  }
  string[length] = NULL; //Determina o fim da variável pois o payload não acompanha sinalizador
  Serial.println(string);

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
    // Attempting MQTT connection…
    String clientId = "noEntrada";
    // Attempt to connect
    // Insert your password
    if (client->connect(clientId.c_str(), "testeWebsocket", "240412Ab")) {
      // connected
      // Once connected, blink twice to indicate it…
      digitalWrite(LED_BUILTIN, LOW); 
      delay(500);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW); 
      delay(500);
      digitalWrite(LED_BUILTIN, HIGH);
      // … and resubscribe
      client->subscribe("testTopic");
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  delay(500);
  // Initiating Serial, select 115200 Baud
  Serial.begin(115200);
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
    // No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?
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

  while (Serial.available())
  {
    message = Serial.readString();
    messageReady = true;
  }
  
  if(messageReady)
  {
    // O codigo presente aqui foi utilizado somente para debug. Futuro codigo deverá prever comunicação no sentido noPrincipalMesh --> broker mqtt
    // client->publish(mychar, &message[0]);
    messageReady = false;
    
  }
}

// Estrutura a ser adotada no envio de mensagem por Mqtt
// {"sensor":"garota","time":1351824120,"data":[48.756080,2.302038]}
// {'type':'garota', 'time': 1030, 'data':[48.35,50.96]}  ---> Ultimo utilizado no HiveMq