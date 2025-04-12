#include "lightController.h"

void setupOutputs()
{
    if (sets.frontHazardLights >= 0)
        pinMode(sets.frontHazardLights, OUTPUT);
    if (sets.rearHazardLights >= 0)
        pinMode(sets.rearHazardLights, OUTPUT);
    if (sets.headlight >= 0)
        pinMode(sets.headlight, OUTPUT);
    if (sets.foglight >= 0)
        pinMode(sets.foglight, OUTPUT);
    if (sets.stopLights >= 0)
        pinMode(sets.stopLights, OUTPUT);
    if (sets.rearLights >= 0)
        pinMode(sets.rearLights, OUTPUT);
}

bool headLightEnable = true;
bool fogLightEnable = true;


void blinkRearLights()
{
    if(sets.rearLights < 0) return;
    digitalWrite(sets.rearLights, HIGH);
    delay(300);
    digitalWrite(sets.rearLights, LOW);
}

void _operateLights(bool enabled, int pin)
{
    if(pin < 0) return;

    if (enabled)
    {
        digitalWrite(pin, HIGH);
    }
    else
    {
        digitalWrite(pin, LOW);
    }
}

void operateHeadLights(bool toggle)
{
    if (toggle)
    {
        if(fogLightEnable && headLightEnable)
        {
            fogLightEnable = false;
            headLightEnable = true;
        }
        else if(headLightEnable)
        {
            fogLightEnable = true;
            headLightEnable = false;
        }
        else if(fogLightEnable)
        {
            fogLightEnable = false;
            headLightEnable = false;
        }
        else
        {
            fogLightEnable = true;
            headLightEnable = true;
        }
    }

    _operateLights(headLightEnable, sets.headlight);
    _operateLights(fogLightEnable, sets.foglight);
}

unsigned long stopTimeout = 0;
bool stopFlag = false;
bool reverseFlag = false;

void operateThrottle()
{
    if (sets.throttle.channel < 0)
        return;

    int minCenterTh = sets.throttle.centerPoint - sets.throttleHysteresis;
    int maxCenterTh = sets.throttle.centerPoint + sets.throttleHysteresis;

    int currentValue = sbus[sets.throttle.channel];
    if (sets.throttle.inverted)
        currentValue = 2048 - currentValue;
    // Serial.println(sets.throttle.channel);
    // Serial.println(sets.stopLights);
    if (currentValue < minCenterTh)
    {
        // reverse
        stopFlag = false;
        stopTimeout = 0;
        reverseFlag = true;
    }
    else if (currentValue > maxCenterTh)
    {
        // forward
        stopFlag = false;
        stopTimeout = 0;
        reverseFlag = false;
    }
    else
    {
        // center
        stopFlag = true;
        reverseFlag = false;
    }

    if (stopFlag)
    {
        if (stopTimeout == 0)
        {
            analogWrite(sets.stopLights, 255);
            stopTimeout = millis() + sets.stopLightDelay;
        }
        else if (millis() > stopTimeout)
        {
            analogWrite(sets.stopLights, sets.tailLightPwm);
        }
    }
    else
    {
        analogWrite(sets.stopLights, sets.tailLightPwm);
    }

    if (reverseFlag)
    {
        digitalWrite(sets.rearLights, HIGH);
    }
    else
    {
        digitalWrite(sets.rearLights, LOW);
    }
}

unsigned long hazardOnTimeout = 0;
unsigned long hazardOffTimeout = 1;
bool blinkFront = false;
bool blinkRear = false;

void operateHazardLights()
{
    if (sets.gearbox.channel < 0 || sets.frontLock.channel < 0 || sets.rearLock.channel < 0)
        return;

    int gbValue = sbus[sets.gearbox.channel];
    if (sets.gearbox.inverted)
        gbValue = 2048 - gbValue;

    int frontLockValue = sbus[sets.frontLock.channel];
    if (sets.frontLock.inverted)
        frontLockValue = 2048 - frontLockValue;

    int rearLockValue = sbus[sets.rearLock.channel];
    if (sets.rearLock.inverted)
        rearLockValue = 2048 - rearLockValue;

    if (gbValue < sets.gearbox.centerPoint)
    {
        blinkFront = false;
        blinkRear = false;
        // first
        if (frontLockValue < sets.frontLock.centerPoint)
        {
            digitalWrite(sets.frontHazardLights, HIGH);
        }
        else
        {
            digitalWrite(sets.frontHazardLights, LOW);
        }

        if (rearLockValue < sets.rearLock.centerPoint)
        {
            digitalWrite(sets.rearHazardLights, HIGH);
        }
        else
        {
            digitalWrite(sets.rearHazardLights, LOW);
        }
    }
    else
    {
        // second
        if (frontLockValue < sets.frontLock.centerPoint)
        {
            // digitalWrite(sets.frontHazardLights, LOW);
            blinkFront = true;
        }
        else
        {
            blinkFront = false;
            digitalWrite(sets.frontHazardLights, HIGH);
        }

        if (rearLockValue < sets.rearLock.centerPoint)
        {
            // digitalWrite(sets.rearHazardLights, LOW);
            blinkRear = true;
        }
        else
        {
            blinkRear = false;
            digitalWrite(sets.rearHazardLights, HIGH);
        }
    }

    bool burnNow = false;
    unsigned long curTime = millis();
    if (hazardOnTimeout > 0 && curTime > hazardOnTimeout)
    {
        hazardOnTimeout = 0;
        hazardOffTimeout = sets.blinkDelayOff;
        burnNow = false;
    }
    else if (hazardOffTimeout > 0 && curTime > hazardOffTimeout)
    {
        hazardOffTimeout = 0;
        hazardOnTimeout = sets.blinkDelayOn;
        burnNow = true;
    }

    if (blinkFront)
    {
        if (burnNow)
            digitalWrite(sets.frontHazardLights, HIGH);
        else
            digitalWrite(sets.frontHazardLights, LOW);
    }

    if (blinkRear)
    {
        if (burnNow)
            digitalWrite(sets.rearHazardLights, HIGH);
        else
            digitalWrite(sets.rearHazardLights, LOW);
    }
}
