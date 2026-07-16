
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
void startPower();
void stopPower();
void setPower(bool activ);


