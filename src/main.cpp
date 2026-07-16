/*
   Lexicon Remote 

   Auteurs :
   Pierre Le Noan
   Xavier Pechoultres

*/

#include <Arduino.h>
#include "lexicon.h"

#define SW_VERSION 26
#define SW_PLATFORM "ESP32"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

#include <ArduinoOTA.h>

#include <DNSServer.h> // utilise pour le setup

#include <ezTime.h> // gestion du temps
#define EZTIME_LANGUAGE FR
// #define EZTIME_LANGUAGE FR

#include <EEPROM.h>

#include "main.h"

const int led = 13;

int sw_data_pos = -1;
int sw_data_pos_full = 0;
#define SW_H_LENGTH 1024

WebServer server(80);
/* DNS server
   En mode setup permet d'acceder directement à la fenetre de config
*/
const byte DNS_PORT = 53;
DNSServer dnsServer;

int  updateDatas();
void handleJSON();
void handleOn();
void handleOff();

void handleIndexPage();
void handleSetupPage();
bool sendFsFile(const char *path, const char *contentType);
void handleWifiScanApi();
void handleWifiConnectApi();
void handleDebugModeApi();
void handleDebugStateApi();


#define USE_DHT // utilise un capteur dht11 - commenter pour desactiver
#define USE_DALLAS // utilise une sonde onwire dallas - commenter pour desactiver
#define POWER_PIN D1
#define RESET_PIN D8


#include <DHT.h>
#define DHTPIN D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
bool dht_ok = false;
float dht_temp = 0.0;
float dht_hum = 0.0;

/*
   Variables
*/
bool power_state = false;

float temperature = 0.0;
float humidity = 0.0;

#define DATA_HISTORY 48  // nombre d'heures pour l'historique

/*
  Timezone
*/
Timezone timezone;


// #define POTAR_TEMP
#ifdef POTAR_TEMP
#define POTAR_PIN D3
#endif

/***********************************
   DALLAS
*/
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS D4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

bool dallas_ok = false;
float dallas_temp = 0.0;

/***********************************
   DHT 11 ou 22
*/

float temperatures[DATA_HISTORY]; // DATA_HISTORY h de temperatures
// float dht_temp[DATA_HISTORY]; // DATA_HISTORY h de temperatures
float humidities[DATA_HISTORY]; // DATA_HISTORY h
float dht_hums[DATA_HISTORY]; // DATA_HISTORY h

/*
   STOCK
*/

struct sw_data {
  time_t ts;
  long sensor;
  long kind;
  float value;
};

sw_data sw_history[SW_H_LENGTH];

void addhistory(long sensor, long kind, float val)
{
  if (sw_data_pos == -1) sw_data_pos = 0;
  sw_history[sw_data_pos].ts = timezone.now();
  sw_history[sw_data_pos].kind = kind;
  sw_history[sw_data_pos].value = val;
  sw_history[sw_data_pos].sensor = sensor;
  sw_data_pos ++;
  if (sw_data_pos == SW_H_LENGTH) {
    sw_data_pos_full = 1;
    sw_data_pos = 0;
  }
}

/********/
struct SatPort {
  int on;
  int pin;
  int options;
  char driver[10];
};


/***********************************
   CONFIG UTILS
*/


int currenthour = -1; // heure actuelle : attention 0 au demarrage de la bête

struct Config {
  int setup_ok;
  char wifi_ssid[50];
  char wifi_pass[50];
  int dallas_on;
  int dht11_on;
  int dht22_on;
  long sensors; //
  char timezone[50];
  long mode; // 1 : automatique
  char hostname[50];
  SatPort ports[10];
};

Config myconf;  // global conf object

#define SID_DHT 1
#define SID_DALLAS 2


long getChipId()
{
  return ESP.getEfuseMac();
}

void conf_load()
{
  Serial.print("conf_load ");
  Serial.println(sizeof(myconf));

  return;
  EEPROM.begin(sizeof(myconf));
  EEPROM.get(0, myconf);
}

void conf_save()
{
  Serial.print("conf_save()");
  return;
  EEPROM.put(0, myconf);
  EEPROM.commit();
}


String strdebug = "";

/*
   RESET DU WIFI

*/
void IRAM_ATTR resetWifi()
{
  Serial.println("Ask reset");
  noInterrupts();
  unsigned long start = millis();

  while(true)
  {
    if (digitalRead(RESET_PIN) == LOW)
    {
      Serial.println("abort reset");
      return;
    }
    if ((millis() - start) > 2000)
      break;
  }
  Serial.println("Reseting");
  WiFi.disconnect();
  WiFi.setAutoReconnect(false);

  Serial.println("Reset Setup");
  myconf.setup_ok = 0;
  Config newconf;
  // EEPROM.get(0, newconf);
  newconf.setup_ok = 0;
  // EEPROM.put(0, newconf);
  // EEPROM.commit();
  Serial.println("will reset by button");
  ESP.restart();
  interrupts();
}


String httpSecure = "";
bool g_mainFsReady = false;
bool g_debugMode = false;

bool parseBoolArg(const String &name, bool defaultValue)
{
  if (!server.hasArg(name))
  {
    return defaultValue;
  }

  String raw = server.arg(name);
  raw.toLowerCase();
  if (raw == "1" || raw == "true" || raw == "on" || raw == "yes")
  {
    return true;
  }
  if (raw == "0" || raw == "false" || raw == "off" || raw == "no")
  {
    return false;
  }
  return defaultValue;
}

void setDebugMode(bool enabled)
{
  g_debugMode = enabled;
  //Serial.setDebugOutput(enabled);
}

bool sendFsFile(const char *path, const char *contentType)
{
  if (!g_mainFsReady)
  {
    g_mainFsReady = LittleFS.begin();
  }

  if (!g_mainFsReady)
  {
    server.send(500, "text/plain", "LittleFS mount failed");
    return false;
  }

  File f = LittleFS.open(path, FILE_READ);
  if (!f)
  {
    return false;
  }

  server.sendHeader("Cache-Control", "no-cache");
  server.streamFile(f, contentType);
  f.close();
  return true;
}

void handleIndexPage()
{
  if (!sendFsFile("/index.html", "text/html"))
  {
    server.send(404, "text/plain", "index.html not found in LittleFS");
  }
}

void handleSetupPage()
{
  if (!sendFsFile("/setup.html", "text/html"))
  {
    server.send(404, "text/plain", "setup.html not found in LittleFS");
  }
}

void handleWifiScanApi()
{
  const int numSsid = WiFi.scanNetworks();
  String json = "{\"ok\":1,\"ssids\":[";
  for (int i = 0; i < numSsid; i++)
  {
    if (i > 0)
    {
      json += ",";
    }
    String ssid = WiFi.SSID(i);
    ssid.replace("\\", "\\\\");
    ssid.replace("\"", "\\\"");
    json += "\"";
    json += ssid;
    json += "\"";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleWifiConnectApi()
{
  if (!server.hasArg("ssid"))
  {
    server.send(400, "application/json", "{\"ok\":0,\"error\":\"missing ssid\"}");
    return;
  }

  const String ssid = server.arg("ssid");
  const String pass = server.hasArg("pass") ? server.arg("pass") : "";
  if (ssid.length() == 0)
  {
    server.send(400, "application/json", "{\"ok\":0,\"error\":\"empty ssid\"}");
    return;
  }

  WiFi.softAPdisconnect(true);
  WiFi.persistent(true);
  WiFi.setAutoConnect(true);
  WiFi.begin(ssid.c_str(), pass.c_str());

  int k = 0;
  while ((WiFi.status() != WL_CONNECTED) && (k < 80))
  {
    delay(100);
    k++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    String response = "{\"ok\":1,\"ip\":\"" + WiFi.localIP().toString() + "\"}";
    server.send(200, "application/json", response);
  }
  else
  {
    server.send(502, "application/json", "{\"ok\":0,\"error\":\"wifi connect failed\"}");
  }
}

void handleDebugModeApi()
{
  const bool enabled = parseBoolArg("enabled", g_debugMode);
  setDebugMode(enabled);
  const String response = String("{\"ok\":1,\"debug\":") + (g_debugMode ? "1" : "0") + "}";
  server.send(200, "application/json", response);
}

void handleDebugStateApi()
{
  const String response = String("{\"ok\":1,\"debug\":") + (g_debugMode ? "1" : "0") + "}";
  server.send(200, "application/json", response);
}

/*
   utilitaire : renvoie l'etant du bit index d'un entier i
   (1 << index) prend 1 et decale vers la gauche de index  bit
   ex = (1 << 3) ==> ..000000100
   ex = (1 << 4) ==> ..000001000
   ensuite on fait un ET (&) logique dessus.
*/
bool bittest(long i, int index)
{
  return (i & (1 << index)) != 0;
}

void set_mode(int mode, bool state)
{
  if (state) myconf.mode =  myconf.mode | (1 << mode);
  else myconf.mode = myconf.mode & ~(1 << mode);
}

bool is_mode(int mode)
{
  return bittest(myconf.mode, mode);
}

bool is_sensor(int sensor_id)
{
  return bittest(myconf.sensors, sensor_id);
}
void set_sensor(int sensor_id, bool state)
{
  if (state) myconf.sensors =  myconf.sensors | (1 << sensor_id);
  else myconf.sensors = myconf.sensors & ~(1 << sensor_id);
}



/*
   SENSOR ID
*/
#define SID_DHT 1
#define SID_DALLAS 2
/*
   DATA KIND
*/
#define DK_TEMP 1
#define DK_HUM 2
/*
   recupere les données des capteurs et met a jours les 2 variables globales
   temperature & humidite
*/
int sw_data_h = -1;
int updateDatas()
{
  bool store = false;
  if (sw_data_h == timezone.hour()) {
    store = false;
  } else {
    sw_data_h = timezone.hour();
  }
  if (dht.read())  {
    Serial.println("DHT read ");
    dht_hum = dht.readHumidity();
    humidity = dht_hum;
    temperature = dht.readTemperature();
    dht_temp = temperature;
    dht_ok = true;
    if (store)
    {
      addhistory(SID_DHT, DK_TEMP, temperature);
      addhistory(SID_DHT, DK_HUM, humidity);
    }

  }
  else {
    dht_ok = false;
    Serial.println("DHT error");
  }

  // si on a les 2 la DALLAS est plus precise, on prend celle la.
  sensors.requestTemperatures(); // Send the command to get temperatures
  float tempC = sensors.getTempCByIndex(0);
  if (tempC != DEVICE_DISCONNECTED_C) {
    dallas_ok = true;
    dallas_temp = tempC;
    temperature = tempC;
    Serial.print("Dallas : ");
    Serial.println(temperature);
    if (store)
    {
      addhistory(SID_DALLAS, DK_TEMP, temperature);
    }
  }
  else {
    dallas_ok = false;
    Serial.println("Error: Could not read temperature data from DALLAS");
  }


  // si les données sont erronnées on met des valeurs stupides
  if (isnan(temperature)) temperature = -100.0;
  if (isnan(humidity)) humidity = -100.0;

  // debug
  Serial.print("Temp: ");
  Serial.println(temperature);
  Serial.print("Hum: ");
  Serial.println(humidity);
  return 0;
}

const char* wifisatus(int s)
{
  switch (s) {
    case WL_CONNECTED : return "assigned when connected to a WiFi network";
    case WL_NO_SHIELD: return "assigned when no WiFi shield is present";
    case WL_IDLE_STATUS: return "it is a temporary status assigned when WiFi.begin() is called and remains active until the number of attempts expires (resulting in WL_CONNECT_FAILED) or a connection is established (resulting in WL_CONNECTED)";
    case WL_NO_SSID_AVAIL: return "assigned when no SSID are available";
    case WL_SCAN_COMPLETED: return "assigned when the scan networks is completed";
    case WL_CONNECT_FAILED: return "assigned when the connection fails for all the attempts";
    case WL_CONNECTION_LOST: return "assigned when the connection is lost";
    case WL_DISCONNECTED: return "disconnected";
  }
  return "unknown wifi status !";
}


// unsigned long CULONG_MAX = 0UL - 1UL;

unsigned long elapsed(unsigned long from)
{
  unsigned long n = millis();
  if (n >= from) return n - from;
  return n + (ULONG_MAX - from);
}

unsigned long limited_duration = 10000;
unsigned long limited_start = 0;
unsigned long limited_session_id = 0;

void setPower(bool activ)
{
  power_state = activ;
  // Relay is wired active-low on this board.
  digitalWrite(POWER_PIN, activ ? LOW : HIGH);
}

void startPower()
{
  setPower(true);
}

void stopPower()
{
  setPower(false);
}

void handleOn()
{
  startPower();
  server.send(200, "text/plain", "power on");
}

void handleOff()
{
  stopPower();
  server.send(200, "text/plain", "power off");
}

void handleJSON()
{
  updateDatas();
  String json = "{\"temperature\":" + String(temperature, 2)
              + ",\"humidity\":" + String(humidity, 2)
              + ",\"power\":" + String(power_state ? 1 : 0)
              + "}";
  server.send(200, "application/json", json);
}


String hostname;


void connectWifi()
{
  
  /* gestion WIFI */
   if (myconf.setup_ok)
   {
      WiFi.mode(WIFI_STA);
      WiFi.begin();

      Serial.println(myconf.wifi_ssid);
      while (WiFi.status() != WL_CONNECTED)
      {
        delay(700);
        Serial.printf("Connection status: %s\n", wifisatus(WiFi.status()));
        Serial.print(".");
      }
       Serial.println(" connected");
       Serial.println(wifisatus(WiFi.status()));
      if (WiFi.status() == WL_CONNECTED)
       {
          Serial.print("Connected, IP address: ");
          Serial.println(WiFi.localIP());
         // NTP
        waitForSync(); // NTP
        timezone.setLocation("Europe/Paris");
       } else {
       Serial.print("WTF with WIFI");
     
       }
  } else {
    Serial.print("Not connected, try to reconnected later ...");
    Serial.println("Starting Accesspoint mode ...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(hostname);
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  }
}


/*
   setup - wtf
*/
void setup()
{
  // demare la sortie standard
  Serial.begin(9600);
  // sleep(3000); // pour laisser le temps au serial de demarrer
  Serial.println("SETUP START");
  
  pinMode(POWER_PIN, OUTPUT);   // set pin to output

  // Serial.setDebugOutput(true);
  hostname = "lexicon_remote_";
  hostname.concat(getChipId());

  Serial.println(hostname);
  

  
  pinMode( RESET_PIN, INPUT);
  // attachInterrupt(digitalPinToInterrupt(RESET_PIN), resetWifi, CHANGE);
  
  /* Chargement Config */
  // Config myconf;
  conf_load();
  stopPower();
  Serial.print("SSID : ");
  Serial.println(myconf.wifi_ssid);
  /* Manage Wifi */
  //WiFi.setAutoConnect(true);
  WiFi.begin();

  int connect_attempts = 0;
  while (WiFi.status() != WL_CONNECTED)
      {
        delay(700);
        Serial.printf("Connection status: %s\n", wifisatus(WiFi.status()));
        Serial.print(".");
        connect_attempts++;
        if (connect_attempts > 20) break;
      }
       Serial.println(" connected");
       Serial.println(wifisatus(WiFi.status()));
      if (WiFi.status() == WL_CONNECTED)
       {
          Serial.print("Connected, IP address: ");
          Serial.println(WiFi.localIP());
         // NTP
        waitForSync(); // NTP
        timezone.setLocation("Europe/Paris");
       } else {
       Serial.print("WTF with WIFI");
     
       }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Starting Accesspoint mode ...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(hostname);
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    Serial.println(WiFi.softAPIP());
  }
  
  // Demarre le mDNS
  Serial.print("hostname:");
  Serial.println(hostname + ".local");
  // WiFi.hostname(nsname);
  Serial.print("Start mDNS: ");
  delay(700);

  if (MDNS.begin(hostname)) {
    Serial.println("MDNS responder started");
  }
  delay(700);
  MDNS.addService("lexicon", "tcp", 80); // declare un service
  MDNS.addService("http", "tcp", 80);
  
  Serial.print("start DHT on pin ");
  Serial.println(DHTPIN);
  pinMode(DHTPIN, INPUT);           // set pin to input
  dht.begin();

  //demarre le Dallas
  Serial.print("start DALLAS on pin ");
  Serial.println(ONE_WIRE_BUS);
  sensors.begin();

  Serial.print("Start Http Server: ");
  // definition des callback pour le server web
  server.on("/json", handleJSON);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/", handleIndexPage);
  server.on("/index.html", handleIndexPage);
  server.on("/home", handleIndexPage);
  server.on("/setup", handleSetupPage);
  server.on("/setup.html", handleSetupPage);
  server.on("/wifi_scan", HTTP_GET, handleWifiScanApi);
  server.on("/wifi_connect", HTTP_GET, handleWifiConnectApi);
  server.on("/wifi_connect", HTTP_POST, handleWifiConnectApi);
  server.on("/debug_mode", HTTP_GET, handleDebugModeApi);
  server.on("/debug_mode", HTTP_POST, handleDebugModeApi);
  server.on("/debug_state", HTTP_GET, handleDebugStateApi);

  lexiconSetup();
  // mise a jour OTA
  server.on("/update", HTTP_GET, []() {
    server.sendHeader("Location", "/update");
    server.send(302, "text/plain", "Redirecting to update page");
  });
  server.on("/update", HTTP_POST, []() {
    server.send(200, "text/plain", "Update complete. Rebooting...");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      String filename = upload.filename;
      if (!filename.startsWith("/")) {
        filename = "/" + filename; // Ensure the filename starts with a slash
      }
      Serial.printf("Update: %s\n", filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { // true to keep the sketch after update
        Serial.printf("Update Success: %u bytes\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
      Serial.println("Update Aborted");
    }
  });

  // demarrage du server web
  server.begin();
  Serial.println("started");


  // setEvent(..., uint8_t hr, uint8_t min, uint8_t sec,
  //        uint8_t day, uint8_t mnth, uint16_t yr)

  events();
}



void loop() {

  // strdebug = "";
  server.handleClient(); // on verifie si on a une connexion http et on la gère
  dnsServer.processNextRequest();
  // unsigned long current_h = timezone.hour();
  events();
  delay(100); // pause de 100ms : gain d'energie
}

String log_messages = "";
bool activelog = false;
void htmllog(String x)
{
  if (!activelog) return;
  log_messages += "<div class=\"alert\">";
  log_messages += x;
  log_messages += "</div>";
}
void stopLogging()
{
  activelog = false;
  activelog = "";
}