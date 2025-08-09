#ifndef LEXICON_H
#define LEXICON_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#ifdef ESP8266
#include <ESP8266WebServer.h>   
#else
#include <WebServer.h>
#endif
int lexiconSetup();
void handleLixiconRC5Command();
void handleLixiconCommand();
void handleLixiconIndex();
int getIntFromHexArg(const String &argName);

#endif // LEXICON_H
