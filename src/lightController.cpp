#include "lightController.h"

void setupOutputs()
{
    pinMode(sets.frontHazardLights, OUTPUT);
    pinMode(sets.rearHazardLights, OUTPUT);
    pinMode(sets.headlightFog, OUTPUT);
    pinMode(sets.stopLights, OUTPUT);
    pinMode(sets.rearLights, OUTPUT);

    digitalWrite(sets.headlightFog, HIGH);
}


unsigned long stopTimeout = 0;
bool stopFlag = false;
bool reverseFlag = false;


void operateThrottle()
{
    int minCenterTh = sets.throttle.centerPoint - sets.throttleHysteresis;
    int maxCenterTh = sets.throttle.centerPoint + sets.throttleHysteresis;

    int currentValue = sbus[sets.throttle.channel];
    if(sets.throttle.inverted) currentValue = 2048 - currentValue;
    //Serial.println(sets.throttle.channel);
    //Serial.println(sets.stopLights);
    if(currentValue < minCenterTh)
    {
        //reverse
        stopFlag = false;
        stopTimeout = 0;
        reverseFlag = true;
    }
    else if(currentValue > maxCenterTh)
    {
        //forward
        stopFlag = false;
        stopTimeout = 0;
        reverseFlag = false;
    }
    else
    {
        //center
        stopFlag = true;
        reverseFlag = false;
    }

    if(stopFlag)
    {
        if(stopTimeout == 0)
        {
            analogWrite(sets.stopLights, 255);
            stopTimeout = millis() + sets.stopLightDelay;
        }
        else if(millis() > stopTimeout)
        {
            analogWrite(sets.stopLights, sets.tailLightPwm);
        }
    }
    else
    {
        analogWrite(sets.stopLights, sets.tailLightPwm);
    }

    if(reverseFlag)
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
    int centerGb = sets.gearbox.centerPoint;
    if(sets.gearbox.inverted) centerGb = 2048 - centerGb;

    int centerFlock = sets.frontLock.centerPoint;
    if(sets.frontLock.inverted) centerFlock = 2048 - centerFlock;

    int centerRlock = sets.rearLock.centerPoint;
    if(sets.rearLock.inverted) centerRlock = 2048 - centerRlock;

    if(centerGb < 800)
    {
        blinkFront = false;
        blinkRear = false;
        //first
        if(centerFlock < 800)
        {
            digitalWrite(sets.frontHazardLights, HIGH);
        }
        else
        {
            digitalWrite(sets.frontHazardLights, LOW);
        }

        if(centerRlock < 800)
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
        //second
        if(centerFlock < 800)
        {
            //digitalWrite(sets.frontHazardLights, LOW);
            blinkFront = true;
        }
        else
        {
            blinkFront = false;
            digitalWrite(sets.frontHazardLights, HIGH);
        }

        if(centerRlock < 800)
        {
            //digitalWrite(sets.rearHazardLights, LOW);
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
    if(hazardOnTimeout > 0 && curTime > hazardOnTimeout)
    {
        hazardOnTimeout = 0;
        hazardOffTimeout = sets.blinkDelayOff;
        burnNow = false;
    }
    else if(hazardOffTimeout > 0 && curTime > hazardOffTimeout)
    {
        hazardOffTimeout = 0;
        hazardOnTimeout = sets.blinkDelayOn;
        burnNow = true;
    }


    if(blinkFront)
    {
        if(burnNow) digitalWrite(sets.frontHazardLights, HIGH);
        else digitalWrite(sets.frontHazardLights, LOW);
    }

    if(blinkRear)
    {
        if(burnNow) digitalWrite(sets.rearHazardLights, HIGH);
        else digitalWrite(sets.rearHazardLights, LOW);
    }
    
}
