
#include <Arduino.h>
#include <FS.h>
#define SPIFFS LITTLEFS
#include <LittleFS.h>


/* */
void set_mode(int mode, bool state);

/* config */
bool is_mode(int mode);
bool is_sensor(int sensor_id);
void set_sensor(int sensor_id, bool state);


/* */
void startToaster();
void stopToaster();
void setToaster(bool activ);


/*  html */
String htmlHead(String title, int refresh);
String cardBegin(String title);
String htmlCheck(String title, String name, bool state);
String htmlCheck(String title, String name, bool state);
String htmlFoot();


/* Clood */
bool connectService();

