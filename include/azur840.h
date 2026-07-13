#ifndef AZUR840_H
#define AZUR840_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#ifdef ESP8266
#include <ESP8266WebServer.h>
#else
#include <WebServer.h>
#endif

int azur840Setup();
void handleAzur840Api();

#endif // AZUR840_H