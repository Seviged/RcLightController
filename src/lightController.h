#ifndef LIGHTCONTROLLER_H
#define LIGHTCONTROLLER_H

#include <Arduino.h>
#include "eepromStorage.h"

extern Settings sets;
extern int sbus[7];

void setupOutputs();

void blinkRearLights();
void operateHeadLights(bool toggle = false);

void operateThrottle();
void operateHazardLights();

#endif