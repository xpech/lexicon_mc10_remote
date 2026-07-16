#ifndef AZUR840_H
#define AZUR840_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <WebServer.h>

int azur840Setup();
void handleAzur840Index();
void handleAzur840Api();

#endif // AZUR840_H