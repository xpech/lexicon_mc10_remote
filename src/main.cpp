/*
   COOLHOME Project

   Gestion des satellites
    capteur
    actionneur

   Auteurs :
   Pierre Le Noan
   Xavier Pechoultres

*/

#include <Arduino.h>
#include "lexicon.h"

#define SW_VERSION 26
#if defined(ESP8266)
#define SW_PLATFORM "ESP8266"
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>
#elif defined(ESP32)
#define SW_PLATFORM "ESP32"
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <HTTPClient.h>

// #include <ESPAsyncWebServer.h>
// #include <ESPAsyncHTTPUpdateServer.h>
#endif

#include <WiFiClient.h>
#include <ArduinoOTA.h>
// #include <ESPHttpUpdate.h>
#include <ArduinoJson.h>

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

#if defined(ESP8266)
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
#else
WebServer server(80);
// HTTPUpdateServer httpUpdater;
#endif
/* DNS server
   En mode setup permet d'acceder directement à la fenetre de config
*/
const byte DNS_PORT = 53;
DNSServer dnsServer;

int  updateDatas();
void watering();
void watering(unsigned long duration);
unsigned long watering_duration = 0;
unsigned long start_watering_at = 0;
boolean on_watering = false;



#define USE_DHT // utilise un capteur dht11 - commenter pour desactiver
#define USE_DALLAS // utilise une sonde onwire dallas - commenter pour desactiver
#define TOASTER_PIN D1
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
bool toaster = false;

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

/***********************************
   PROGRAM
*/

struct program
{
  int hour;
  int minute;
  float temp;
};


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
  program programs[10];
  char hostname[50];
  char cloudlogin[100];
  SatPort ports[10];
};

Config myconf;  // global conf object

#define SID_DHT 1
#define SID_DALLAS 2


#define MODE_AUTO 1
#define MODE_PILOT 2
#define MODE_WATERING 3

long getChipId()
{
#if defined(ESP8266)
  return ESP.getChipId();
#else
  return ESP.getEfuseMac();
#endif
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
  EEPROM.put(0, myconf);
  EEPROM.commit();
}


String strdebug = "";


float getTarget()
{
  bool found = false;
  int h = timezone.hour();
  int m = timezone.minute();
  Serial.printf("get target for %d:%d\n", h, m);
  // strdebug = "<div>getTarget h=" + String(h) +":"+ String(m) + "</div>";

  float last = 0.0;
  float target = 0.0;
  for (int i = 0; i < 10; i++)
  {
    // strdebug += "<div>" + String(i) + "</div>";

    if (myconf.programs[i].hour >= 0)
    {
      //Serial.printf("programs[%d].hour = %d\n", i, myconf.programs[i].hour);
      last = myconf.programs[i].temp;
      // strdebug += "<div>last:" + String(last) + "</div>";

      if (
        myconf.programs[i].hour < h ||
        (myconf.programs[i].hour == h &&
         myconf.programs[i].minute <= m))
      {
        // strdebug += "<div>match ! " + String(i) + " hour : " + String(myconf.programs[i].hour) + "</div>";
        last = myconf.programs[i].temp;
        target = last;
        found = true;
        //Serial.printf("last = %f  // target = %f \n", last, target);
      } // else strdebug += "<div>NOT MATCH ! " + String(i) + " hour : " + String(myconf.programs[i].hour) + ":" +String(myconf.programs[i].minute) + "</div>";
    }
  }
  //strdebug += "<div> out target = "+ String(target)+ "</div>";
  //strdebug += "<div> out last = "+ String(last)+ "</div>";
  if (!found) target = last;
  return target;
}

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
  EEPROM.get(0, newconf);
  newconf.setup_ok = 0;
  EEPROM.put(0, newconf);
  EEPROM.commit();
  Serial.println("will reset by button");
  #if defined(ESP8266)
  ESP.reset();
  #else
  ESP.restart();
  #endif
  interrupts();
}

/*
  handle http reponse /
  standard : renvoie les donnees au format JSON (facile a utiliser)
  { "temperature" : 12.0, "humidity" : 50.0, "toaster" : 0, temperatures : [23,24,23]  }
*/
void handleJSON() {

  digitalWrite(led, 1); // j'allume la diode pour info

  updateDatas(); // lit les capteurs (voir plus bas)

  // creation de la reponse
  String response;
  //server.send(200, F("application/json"), response.c_str());
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);

  server.send(200, F("application/json"), "");
  /*
    Serial.println("fin");
    server.sendContent("");
    server.client().stop();
  */
  server.sendContent(F("{ \"temperature\": "));
  server.sendContent(String(temperature));
  server.sendContent(F(", \"humidity\" :  "));
  server.sendContent(String(humidity));
  server.sendContent(", \"toaster\" :  ");
  server.sendContent(String(toaster));
  server.sendContent(", \"datas\" : [");


  int hp = sw_data_pos;
  if (sw_data_pos_full)
  {
    while (hp != SW_H_LENGTH)
    {
      if (sw_history[hp].ts > 0)
        server.sendContent( "{ time:" + String(sw_history[hp].ts) +
                            ", kind:" + String(sw_history[hp].kind) +
                            ", value:" + String(sw_history[hp].value) +
                            ", sensor:" + String(sw_history[hp].sensor) + "},");
      hp++;
    }
  }


  hp = 0;
  Serial.print("sw_data_pos : ");
  Serial.println(sw_data_pos);
  while (hp <= sw_data_pos);
  {
    if (sw_history[hp].ts > 0)
      server.sendContent( "{ time:" + String(sw_history[hp].ts) +
                          ", kind:" + String(sw_history[hp].kind) +
                          ", value:" + String(sw_history[hp].value) +
                          ", sensor:" + String(sw_history[hp].sensor) + "},");
    hp++;
  }
  server.sendContent( "],\"temperatures\" : [");
  bool addsep = false;
  for (int i = currenthour + 1; i < DATA_HISTORY; i++)
  {
    String response;
    if (addsep) response.concat(","); else addsep = true;
    response.concat(temperatures[i]);
    server.sendContent(response);
  }
  for (int i = 0; i <= currenthour; i++)
  {
    String response;
    if (addsep) response.concat(","); else addsep = true;
    response.concat(temperatures[i]);
    server.sendContent(response);

  }
  server.sendContent("]");
  server.sendContent(", \"humidities\" : [");
  addsep = false;
  for (int i = currenthour + 1; i < DATA_HISTORY; i++)
  {
    String response;
    if (addsep) response.concat(","); else addsep = true;
    response.concat(humidities[i]);
    server.sendContent(response);
  }
  for (int i = 0; i <= currenthour; i++)
  {
    String response;
    
    if (addsep) response.concat(","); else addsep = true;
    response.concat(humidities[i]);
    server.sendContent(response);
  }
  server.sendContent(F("]}"));

  server.sendContent("");
  server.client().stop();

  // envoie de la reponse au client
  // server.send(200, F("application/json"), response.c_str());

  // on eteint la led :)
  digitalWrite(led, 0);
}

String lastHttpResponse = "";
String lastHttpRequest = "";

String httpSecure = "";


/*
   Affiche la fenetre de configuration
*/
void handleSetup()
{
  if (httpSecure == "") httpSecure = String(micros());
  Serial.println(httpSecure);
  Serial.println(server.arg("securekey"));
  String akey = server.arg("securekey");
  String html = htmlHead("Setup", 0);

  if (server.hasArg("saveconf") && akey == httpSecure)
  {
    html.concat("<div>Save</div>");
    String newcloud = server.arg("cloudlogin");
    newcloud.toCharArray(myconf.cloudlogin, 99);

    Serial.println("Save eprom");
    Serial.println(myconf.cloudlogin);
    set_mode(MODE_PILOT, (server.arg("pilot") == "1"));
    set_mode(MODE_WATERING, (server.arg("watering") == "1"));
    set_sensor(SID_DHT, (server.arg("dht11") == "1"));
    set_sensor(SID_DALLAS, (server.arg("dallas") == "1"));
    conf_save();
    if (server.hasArg("ssid") && (server.arg("ssid") != ""))
    {
      Serial.print("Change SSID : ");

      
      WiFi.softAPdisconnect(true);
      String newssid = server.arg("ssid");
      String newpass = server.arg("pass");
      Serial.println(newssid);
  
      WiFi.persistent(true);
      WiFi.setAutoConnect(true);
      WiFi.begin(newssid, newpass);
      WiFi.waitForConnectResult();
      int k = 0;
      while ((WiFi.status() != WL_CONNECTED) && (k < 80)) {
        delay(100);
        Serial.print('.');
        k++;
        if (k > 80)
        {
          resetWifi();
        }
      }
    }
  }
  html.concat(F("<form method=\"GET\" action=\"/setup\">"));
  html.concat(F("<input type=\"hidden\" name=\"securekey\" value=\""));
  html.concat(httpSecure);
  html.concat(F("\"/>"));

  html.concat(cardBegin("Wifi"));
  html.concat(F("<div class=\"form-group\"><label>Wifi :</label>"
                "<select class=\"form-control\" name=\"ssid\">"
                "<option value=\"\">Choose...</option>"));
  int numSsid = WiFi.scanNetworks();
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    html.concat("<option value=\"" + WiFi.SSID(thisNet) + "\">" + WiFi.SSID(thisNet) + "</option>");
  }
  html.concat(F("</select></div><br>"
                "<div class=\"form-group\">"
                "<label>Mot de passe:</label><input class=\"form-control\" type=\"text\" name=\"pass\"/></div>"));
  html.concat("<input type=\"submit\" name=\"saveconf\" class=\"btn btn-success\" value=\"Connecter\"></div></div>");

  html.concat(cardBegin("CoolHome Server"));
  html.concat("<div class=\"form-group\"><label>CoolHome ID (email)</label><input class=\"form-control\" type=\"text\" name=\"cloudlogin\" value=\"" +  String(myconf.cloudlogin) + "\"></div><br/>"
              "<input type=\"submit\" name=\"saveconf\" class=\"btn btn-success\" value=\"Connecter\"></div></div>");

  html.concat(cardBegin("Capteurs"));
  html.concat(htmlCheck("DHT11 ?", "dht11", is_sensor(SID_DHT)));
  html.concat(htmlCheck("DALLAS ?", "dallas", is_sensor(SID_DALLAS)));
  html.concat(htmlCheck("Use pilot for toaster ?", "pilot", is_mode(MODE_PILOT)));
  html.concat(htmlCheck("Watering ?", "watering", is_mode(MODE_WATERING)));

  html.concat(F("<button class=\"btn btn-primary\" type=\"submit\" name=\"saveconf\" value=\"1\">Save</button></div></div></form>"));
  html.concat(F("<hr><pre>"));
  html.concat(lastHttpResponse);
  html.concat(F("</pre>"));
  html.concat(F("<hr><pre>"));
  html.concat(lastHttpRequest);
  html.concat(F("</pre>"));
  html.concat(htmlFoot());
  server.send(200, "text/html", html.c_str());
}

String htmlCheck(String title, String name, bool state)
{
  String x = F("<div class=\"form-check\"><input class=\"form-check-input\" type=\"checkbox\" value=\"1\" ");
  if (state) x.concat("checked");
  x.concat(" name=\"" + name + "\">");
  x.concat(F("<label class=\"form-check-label\">"));
  x.concat(title);
  x.concat(F("</label></div>"));
  return x;
}

String htmlNav()
{
  return "<nav class=\"navbar navbar-dark bg-dark navbar-expand-lg\">"
         "<a class=\"navbar-brand\" href=\"/\">CoolHome</a>"
         "<button class=\"navbar-toggler\" type=\"button\""
         " data-toggle=\"collapse\" data-target=\"#navbarSW\" aria-controls=\"navbarSW\""
         " aria-expanded=\"false\" aria-label=\"Toggle navigation\">"
         " <span class=\"navbar-toggler-icon\"></span>"
         "</button>"
         "<div class=\"collapse navbar-collapse\" id=\"navbarSW\">"
         "<ul class=\"navbar-nav mr-auto mt-2 mt-lg-0\">"
         "<li class=\"nav-item\"><a class=\"nav-link\" href=\"/program\">Programme</a></li>"
         "<li class=\"nav-item\"><a class=\"nav-link\" href=\"/setup\">Setup</a></li>"
         "<li class=\"nav-item\"><a class=\"nav-link\" href=\"/update\">Mise à jour</a></li>"
         "<li class=\"nav-item\"><a class=\"nav-link\" href=\"/json\">JSON</a></li>"
         "</ul></div><ul class=\"navbar-nav flex-row ml-md-auto d-none d-md-flex\">"
         "<li class=\"nav-item nav-link\"><a href=\"https://coolhome.ovh\">CoolHome</a></li>"
         "<li class=\"nav-item nav-link\">ESP #" + String(getChipId()) + "</li>"
         "<li class=\"nav-item nav-link\">version " + String(SW_VERSION) + " (" + __DATE__ + ")</li>"
         "<li class=\"nav-item nav-link\">" + timezone.dateTime() + "</li></ul></nav>";
}

String cardBegin(String title)
{
  String x = F("<div class=\"card\"><div class=\"card-header\">");
  x.concat(title);
  x.concat(F("</div><div class=\"card-body\">"));
  return x;
}
/*
   Renvoie le debut de la page html
*/
String htmlHead(String title, int refresh)
{
  String html = F("<!DOCTYPE html>"
                  "<html lang=\"fr\"><head>"
                  "<meta charset=\"utf-8\">");
  if (refresh > 0)
  {
    html.concat(F("<meta http-equiv=\"refresh\" content=\""));
    html.concat(refresh);
    html.concat(F("\" >"));
  }
  html.concat(F("<meta name=\"viewport\" content=\"awidth=device-width, initial-scale=1, shrink-to-fit=no\"/>"
                "<title>"));
  html.concat(title);
  html.concat(F("</title>"
                "<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/css/bootstrap.min.css\" integrity=\"sha384-Vkoo8x4CGsO3+Hhxv8T/Q5PaXtkKtu6ug5TOeNV6gBiFeWPGFN9MuhOf23Q9Ifjh\" crossorigin=\"anonymous\">"
                "</head><body>"));

  html.concat(F("<div class=\"container-fluid\">"));
  html.concat(htmlNav());
  return html;
}

String htmlFoot()
{
  return F("</div><script src=\"https://code.jquery.com/jquery-3.4.1.slim.min.js\" integrity=\"sha384-J6qa4849blE2+poT4WnyKhv5vZF5SrPo0iEjwBvKU7imGFAV0wwj1yYfoRSJoZ+n\" crossorigin=\"anonymous\"></script>"
           "<script src=\"https://cdn.jsdelivr.net/npm/popper.js@1.16.0/dist/umd/popper.min.js\" integrity=\"sha384-Q6E9RHvbIyZFJoft+2mJbHaEWldlvI9IOYy5n3zV9zzTtmI3UksdQRVvoxMfooAo\" crossorigin=\"anonymous\"></script>"
           "<script src=\"https://stackpath.bootstrapcdn.com/bootstrap/4.4.1/js/bootstrap.min.js\" integrity=\"sha384-wfSDF2E50Y2D1uUdj0O3uMBJnjuUD4Ih7YwaYd1iqfktj0Uod8GCExl3Og8ifwB6\" crossorigin=\"anonymous\"></script>"
           "</body></html>");
}

/*
   Gestion des options,
   on utilise un long comme un tableau de booleen, on a donc 32 boolean disponible

*/

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
int httpConsigne = -1;


/*
   Affichage de la home page
*/
void handleHome()
{
  updateDatas();
  Serial.println("handleHome");
  String html = htmlHead("CoolHome", 0);
  html.concat(F("<div class=\"card\">"
                "  <div class=\"card-header\">Temperature</div><div class=\"card-body\">"));
  html.concat(temperature);
  html.concat(F("°</div><div class=\"card-footer\">"));
  if (!isnan(dallas_temp))
  {
    html.concat("<div class=\"mr-2\">Dallas : ");
    html.concat(dallas_temp);
    html.concat("</div>");
  }
  if (!isnan(dht_temp))
  {
    html.concat("<div class=\"mr-2\">DHT : ");
    html.concat(dht_temp);
    html.concat("</div>");
  }
  html.concat(F("</div></div><div class=\"card\">"
                "  <div class=\"card-header\">Humidité</div><div class=\"card-body\">"));
  html.concat(humidity);
  html.concat(F("%</div></div><div class=\"card\">"
                "  <div class=\"card-header\">Radiateur</div><div class=\"card-body\">"));

  if (toaster)
    html.concat(F("<a href=\"/off\" class=\"btn btn-warning\">Toaster is On</a>"));
  else
    html.concat(F("<a href=\"/on\" class=\"btn btn-secondary\">Toaster is Off</a>"));

  if (httpConsigne < 0)
    html.concat(F("<p class=\"alert alert-light\">Pas de consigne serveur</p>"));
  else if (httpConsigne == 0)
    html.concat(F("<p class=\"alert alert-dark\">Consigne Off</p>"));
  else html.concat(F("<p class=\"alert alert-warning\">Consigne On</p>"));

  if (is_mode(MODE_WATERING))
  {
    html.concat(F("<p class=\"alert alert-info\">Arrosage durée : "));
    html.concat(watering_duration);
    html.concat(F(" départ : "));
    html.concat(start_watering_at);
    html.concat(F(" </p>"));
  }

  html.concat(F("</div><div class=\"card-footer\">"));
  if (is_mode(MODE_AUTO))
  {
    html.concat(F("Mode automatique activé ("));
    html.concat(getTarget());
    html.concat(F("°)"));
  }
  html.concat(F("<a href=\"/sync\" class=\"btn btn-primary mr-2\">Synchro Serveur</a>"));
  /*
    html.concat(F("<a href=\"/program\" class=\"btn btn-primary mr-2\">Programme</a>"));
    html.concat(F("<a href=\"/setup\" class=\"btn btn-light mr-2\"> Setup </a> "));
    html.concat(F("<a href=\"/update\" class=\"btn btn-warning mr-2\"> Update </a>")); */
  html.concat(F("</div></div>"));
  html.concat(strdebug);
  html.concat(htmlFoot());
  // strdebug = "";
  server.send(200, "text/html", html.c_str());
}



/*
  Program html Form
*/
void handleProgram() {
  String html = htmlHead("CoolHome Programation", 0);
  if (server.method() == HTTP_POST)
  {
    if (server.hasArg("reset"))
    {
      for (int i = 0; i < 10; i++ ) {
        myconf.programs[i].hour = -1;
        myconf.programs[i].minute = 0;
        myconf.programs[i].temp = 18.0;
      }

    } else {

      if (server.hasArg("enable"))
      {
        Serial.print("enable");
        myconf.mode =  myconf.mode | (1 << MODE_AUTO);
        Serial.println(myconf.mode);

      }
      if (server.hasArg("disable"))
      {
        Serial.print("enable");
        myconf.mode = myconf.mode & ~(1 << MODE_AUTO);
      }
      Serial.print("Mode : "); Serial.println(myconf.mode);

      for ( uint8_t i = 0; i < 10; i++ ) {
        String key = "hour["; key.concat(i);  key.concat("]");
        // long h = atol(server.arg(key));


        int h = atoi(server.arg(key).c_str());
        if (server.arg(key) == "") h = -1;
        if (h >= 0 && h < 24) {
          myconf.programs[i].hour = h;

          key = "minute["; key.concat(i);  key.concat("]");
          int m = atoi(server.arg(key).c_str());
          if (m < 0) m = 0;
          else if (m > 59) m = 59;
          myconf.programs[i].minute = m;
          key = "temp["; key.concat(i);  key.concat("]");
          float t = atof(server.arg(key).c_str());
          if (t < 0.0 && t > 50.0) t = 18.0;
          //float m = atof(server.arg(key));
          myconf.programs[i].temp = t;
        } else {
          myconf.programs[i].hour = -1;
          myconf.programs[i].minute = 0;
          myconf.programs[i].temp = 18.0;
        }
      }
      conf_save();
    }
  }

  html.concat(F("<div class = \"card\">"
                "<div class=\"card-header\">Programmation</div>"
                "<div class=\"card-body\"><form method=\"POST\">"
                "<table class = \"table table-sm\">"
                "<thead><tr><th>Heure</th><th>Minute</th><th>consigne</th></tr></thead><tbody>"));

  for (int i = 0; i < 10; i++)
  {
    program p = myconf.programs[i];
    html.concat("<tr><td><input type=\"number\" class=\"form-control text-right\" name=\"hour[");
    html.concat(i); html.concat("]\" size=\"2\" value=\"");
    if (p.hour >= 0)
      html.concat(p.hour);
    html.concat(F("\"></td>"));
    html.concat(F("<td><input type=\"number\" class=\"form-control text-right\" name=\"minute["));
    html.concat(i); html.concat("]\" size=\"2\" value=\"");
    if (p.hour >= 0) html.concat(p.minute);
    html.concat("\"></td>");
    html.concat(F("<td><input type=\"float\" class=\"form-control text-right\" name=\"temp["));
    html.concat(i); html.concat("]\" size=\"4\" value=\"");
    if (p.hour >= 0) html.concat(p.temp); else html.concat(18);
    html.concat(F("\"></td></tr>"));
  }
  html.concat(F("</tbody></table></div><div class=\"card-footer\">"));
  html.concat("<button class=\"btn btn-primary mr-2\" type=\"submit\">Enregistrer</button>");
  Serial.print("mode auto:");
  Serial.println(myconf.mode);
  Serial.println(is_mode(MODE_AUTO));
  Serial.println(1 << MODE_AUTO);

  if (is_mode(MODE_AUTO))
    html.concat("<button class=\"btn btn-warning mr-2\" type=\"submit\" name=\"disable\" value=\"1\">Désactiver le mode automatique</button>");
  else
    html.concat("<button class=\"btn btn-success mr-2\" type=\"submit\" name=\"enable\" value=\"1\">Activer le mode automatique</button>");
  html.concat("<button class=\"btn btn-success mr-2\" type=\"submit\" name=\"reset\" value=\"1\">Reset</button>");

  html.concat(F("</div></div>"));
  html.concat(htmlFoot());
  server.send(200, F("text/html"), html.c_str());

}

/*
  handle http reponse /on
  allume le radiateur et memorise (toaster = 1 )
  renvoie le statut standard
*/
void handleOn() {
  /* switdh off */
  startToaster();
  handleHome();
}

/*
  handle http reponse /on
  allume le radiateur et memorise (toaster = 1 )
  renvoie le statut standard
*/
void handleSync() {
  /* switdh off */
  connectService();
  handleHome();
}



/*
  handle http reponse /off
  eteint le radiateur et memorise (toaster = 0 )
  renvoie le statut standard
*/
void handleOff() {
  stopToaster();
  handleHome();
}



void handleTest() {
  // server.sendHeader("Content-Length", (String)fileSize);

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  Serial.println("handleTest");
  server.send(200, "text/html", "");
  server.sendContent("dd");
  for (int i  = 0 ; i < INT_MAX; i++ )
  {
    server.sendContent(String(i) + "<br>");
  }
  Serial.println("fin");
  server.sendContent("");
  server.client().stop();
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


void startToaster() {
  setToaster(true);
}
void stopToaster() {
  setToaster(false);
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

void setToaster(bool activ) {
  toaster = activ;
  if (is_mode(MODE_PILOT)) activ = !activ;
  digitalWrite(TOASTER_PIN, (activ ? LOW : HIGH));
}
String hostname;


void connectWifi()
{
  
  /* gestion WIFI */
   if (myconf.setup_ok)
   {
      WiFi.mode(WIFI_STA);
      //WiFi.begin("lamouetterieuse2", "MARINE/XAV");

  
      // WiFi.begin(myconf.wifi_ssid, myconf.wifi_pass);
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

int HttpUpdatePeriod = 60 * 5;

time_t lastHttpUpdate = 0;
/*
  Send data to coolhome server
  If wifi is not connected, try to connect
*/
void http_update()
{
  if ((timezone.now() - lastHttpUpdate) >= HttpUpdatePeriod)
  {
    if (!connectService())
      Serial.println("error update server");
    lastHttpUpdate =  timezone.now();
    if (WiFi.status() != WL_CONNECTED)
    {
      if (myconf.setup_ok) connectWifi();
    }
  } else {
    // Serial.println("not update server");
  }
}

void wifimanager()
{

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
  
  pinMode(TOASTER_PIN, OUTPUT);   // set pin to output

  // Serial.setDebugOutput(true);
  hostname = "coolhome_";
  hostname.concat(getChipId());

  Serial.println(hostname);
  

  
  pinMode( RESET_PIN, INPUT);
  // attachInterrupt(digitalPinToInterrupt(RESET_PIN), resetWifi, CHANGE);
  
  /* Chargement Config */
  // Config myconf;
  conf_load();
  stopToaster();
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
  MDNS.addService("coolhome", "tcp", 80); // declare un service
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
  server.on("/", handleHome);
  server.on("/setup", handleSetup);
  server.on("/test", handleTest);
  server.on("/program", handleProgram);

  server.on("/sync", handleSync);

  server.on("/lexicon", handleLixiconIndex);
  server.on("/lexiconCommand", handleLixiconCommand);

  lexiconSetup();
  // mise a jour OTA
#ifdef ESP8266
  httpUpdater.setup(&server);
#else
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
  
#endif
  // demarrage du server web
  server.begin();
  Serial.println("started");


  // setEvent(http_update, uint8_t hr, uint8_t min, uint8_t sec,
  //        uint8_t day, uint8_t mnth, uint16_t yr)

  events();

#ifdef POTAR_TEMP
  pinMode(POTAR_PIN, INPUT);   // set pin to output
#endif

}

unsigned long auto_timer = 0;
unsigned long auto_timer_delay = 10000;

String host = "http://coolhome.ovh/api";
String updatehost = "coolhome.ovh";



bool connectService()
{
  Serial.println("connectService");
  updateDatas();
  WiFiClient client;
  HTTPClient http;
  if (http.begin(client, host))
  {
    http.addHeader("Content-Type", "application/json");
    http.addHeader("CoolHomeDeviceId", String(getChipId()));
    http.addHeader("CoolHomeAccount", myconf.cloudlogin);
    DynamicJsonDocument request(2048);
    String json;
    
    request["version"] = SW_VERSION;
    request["platform"] = SW_PLATFORM;

    myconf.mode = myconf.mode & ~(1 << MODE_AUTO);
    

    request["sensorid"] = getChipId();
    request["heater"] = toaster;
    JsonArray sensors = request.createNestedArray("sensors");
    
    if (!isnan(dallas_temp) && dallas_ok && is_sensor(SID_DALLAS))
    {
      JsonObject dallas = sensors.createNestedObject();
      dallas["kind"] = "temp";
      dallas["name"] = "Integrated dallas";
      dallas["value"] = dallas_temp;
    }
    
    if (!isnan(dht_temp) && dht_ok && is_sensor(DHT11))
    {
      JsonObject dhtemp = sensors.createNestedObject();
      dhtemp["kind"] = "temp";
      dhtemp["name"] = "Integrated DHT11";
      dhtemp["value"] = dht_temp;
      Serial.println("add dhtemp to sensors");
    }
    
    if (!isnan(dht_hum) && dht_ok && is_sensor(DHT11))
    {
      Serial.println("Add humidity");
      JsonObject dhthum = sensors.createNestedObject();
      dhthum["kind"] = "hr";
      dhthum["name"] = "Integrated DHT11";
      dhthum["value"] = dht_hum;

    }
    
    if (is_mode(MODE_WATERING))
    {
      JsonObject dhthum = sensors.createNestedObject();
      dhthum["kind"] = "water";
      dhthum["name"] = "Watering";
      dhthum["value"] = (int)on_watering;
    }
    
    serializeJson(request, json);
    lastHttpRequest = json;
    Serial.println(json);
    
    int httpCode = http.POST(json);
    
    if (httpCode == HTTP_CODE_OK)
    {
      
      DynamicJsonDocument resp(2048);
      lastHttpResponse = http.getString();
      Serial.println(lastHttpResponse);

      deserializeJson(resp, lastHttpResponse);
      
      http.end();
      JsonVariant con = resp["connected"];
      
      Serial.print("connected : ");
      Serial.println(con.as<bool>());
      
      JsonVariant jdt = resp["date_time"];
      Serial.print("datetime : ");
      Serial.println(jdt.as<String>());
      
      // XP !!! bool heat_cons = ;
      bool heat_cons = resp["heater"];
      
      Serial.print("heat_cons : ");
      Serial.println(heat_cons);

      if (is_mode(MODE_WATERING))
      {
        int duration = resp["duration"];
        watering(duration * 60);
      } else 
        setToaster(heat_cons);
      
      httpConsigne = 1;
     
      if (resp["version"])
      {
        if (resp["version"] > SW_VERSION)
        {
          Serial.println("Need update");
          if (resp["firmware_url"])
          {
            const char* firmware_url = resp["firmware_url"];
            #ifdef ESP8266
            ESPhttpUpdate.update(client, firmware_url);
            #endif
          }
        }
      }
      
      return true;
    }
  }
  return false;
}

unsigned long last_watering= 0;
unsigned long last_watering_date = 0;



void watering(unsigned long duration)
{

  strdebug = "watering(";
  strdebug += duration;
  strdebug  += ") => ";
  if (duration > 0)
  {
    last_watering = duration;
    if (duration > 3600) duration = 3600; // 1h max
    duration *= 1000;
    
    if (start_watering_at == 0) {
      start_watering_at = millis();
      watering_duration = duration / 1000;
    }
    if (elapsed(start_watering_at) > duration)
    {
        if (on_watering)
          Serial.println("Stop Watering (end)");
        strdebug += "Stop Watering : time off / start: ";
        strdebug += start_watering_at;
        strdebug += "  elapsed : " ;
        strdebug += elapsed(start_watering_at);
        on_watering = false;
        start_watering_at = 0;
        stopToaster();
    } else
    {
        if (!on_watering)
          Serial.println("Start Watering");
        startToaster();
        on_watering = true;
        strdebug += "On watering";
     }
  } else if(on_watering) {
    Serial.println("Not Watering");
    strdebug += "Stop Watering : duration is null";
    stopToaster();
    on_watering = false;
    start_watering_at += 0;
  }
}
void watering()
{
  watering((unsigned long)getTarget());
}


void loop() {

  // strdebug = "";

#ifdef ESP8266
  MDNS.update(); // mise à jour MDNS / necessaire !
#endif
  server.handleClient(); // on verifie si on a une connexion http et on la gère
  dnsServer.processNextRequest();

  // unsigned long current_h = timezone.hour();

#ifdef POTAR_TEMP
  float v =  (float)analogRead(POTAR_PIN);
  float target = 20. + 40. * v / 1023.;
#endif
  http_update();
  if (is_mode(MODE_WATERING) && is_mode(MODE_AUTO)) watering();
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