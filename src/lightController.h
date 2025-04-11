#ifndef LIGHTCONTROLLER_H
#define LIGHTCONTROLLER_H

#include <Arduino.h>
#include "eepromStorage.h"

extern Settings sets;
extern int sbus[8];

void setupOutputs();

void operateLights();

void operateThrottle();
void operateHazardLights();

#endif