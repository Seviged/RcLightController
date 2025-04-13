#include <Arduino.h>
#include "eepromStorage.h"
#include "lightController.h"

int sbus[7] = {0};

#include "webServer.h"
WebServer serv;
long sbusFails = 0;

EepromStorage store;
Settings sets;
uint8_t frame[24] = {0}; // SBUS received bytes. 1st byte of 25-byte frame is always 0x0F, and is not stored

int channel(int ch)
{                          // extract 11-bit channel[ch] value from frame. ch 0-15
  int k = ch > 7 ? 11 : 0; // offset into frame array: 0 for channels 0-7, 12 for channels 8-15
  switch (ch % 8)
  { // pattern repeats (except for k-offset) after 8 channels
  case 0:
    return (int)frame[0 + k] | ((((int)frame[1 + k]) & 0x07) << 8);
  case 1:
    return ((int)(frame[1 + k] & 0xF8) >> 3) | ((((int)frame[2 + k]) & 0x3F) << 5);
  case 2:
    return ((int)(frame[2 + k] & 0xC0) >> 6) | ((((int)frame[3 + k])) << 2) | ((((int)frame[4 + k]) & 0x01) << 10);
  case 3:
    return ((int)(frame[4 + k] & 0xFE) >> 1) | ((((int)frame[5 + k]) & 0x0F) << 7);
  case 4:
    return ((int)(frame[5 + k] & 0xF0) >> 4) | ((((int)frame[6 + k]) & 0x7F) << 4);
  case 5:
    return ((int)(frame[6 + k] & 0x80) >> 7) | ((((int)frame[7 + k])) << 1) | ((((int)frame[8 + k]) & 0x03) << 9);
  case 6:
    return ((int)(frame[8 + k] & 0xFC) >> 2) | ((((int)frame[9 + k]) & 0x1F) << 6);
  case 7:
    return ((int)(frame[9 + k] & 0xE0) >> 5) | (((int)frame[10 + k]) << 3);
  }
  return -1; // execution never reaches here, but this supresses a compliler warning
}

unsigned long enableTimerServer = 0;
bool triggerStarted = false;

// параметры фильтра
const unsigned long HOLD_TIME = 3000; // 3 секунды
const unsigned long LIGHTS_HOLD_TIME = 900; // 0.9 секунды

void enableServerFunc()
{
  if(sets.enableServer.channel < 0 && !serv.isEnabled())
  {
    Serial.println(sets.enableServer.channel);
    Serial.println("ENABLING SERVER...");
      serv.enable(); // включаем сервер
      Serial.println("SERVER ENABLED!!!");
  }

  int ch = sbus[sets.enableServer.channel];
  if(sets.enableServer.inverted) ch = 2048 - ch;

  // если значение превышает порог с гистерезисом — начинаем отсчёт
  if (ch > sets.enableServer.centerPoint)
  {
    if (!triggerStarted)
    {
      enableTimerServer = millis();
      triggerStarted = true;
      Serial.println("Start hold (debounced)");
    }

    if (millis() - enableTimerServer >= HOLD_TIME && !serv.isEnabled())
    {
      Serial.println("ENABLING SERVER...");
      blinkRearLights(500);
      serv.enable(); // включаем сервер
      Serial.println("SERVER ENABLED!!!");
    }
  }

  if (millis() - enableTimerServer >= LIGHTS_HOLD_TIME && triggerStarted)
  {
    blinkRearLights(300);
  }

  // если значение опустилось ниже нижнего порога — сбрасываем
  if (ch < sets.enableServer.centerPoint && enableTimerServer > 0)
  {
    if (millis() - enableTimerServer >= LIGHTS_HOLD_TIME && millis() - enableTimerServer < HOLD_TIME)
    {
      operateHeadLights(true);
    }

    if (triggerStarted)
      Serial.println("Hold cancelled (debounced)");
    triggerStarted = false;
    enableTimerServer = 0;
  }
}

void writeSettings(bool erase)
{
  if(erase)
  {
    store.restoreDefaultSettings();
    WiFi.forceSleepBegin();
    wdt_reset();
    ESP.restart();
  }
  store.writeSettings(sets);
}

int failsafeMarker = 0;

void setup()
{
  Serial.setRxBufferSize(64);
  Serial.begin(100000, SERIAL_8E2);
  delay(100);

  store.readSettings(sets);

  setupOutputs();

  serv.setSettingsCallback(writeSettings);
  delay(300);
}

void loop()
{
  enableServerFunc();

  bool failsafe = false;
  bool frameReceived = false;
  while (Serial.available())
  {
    //  try to receive a 25-byte frame that begins with 0x0F and ends with 0x00
    //  scan until an 0x0F byte is received, or the receive buffer is empty
    int b = Serial.read();
    if (b != 0x0F)
    {
      continue;
    }
    unsigned long timeout = millis() + 4000;
    frame[23] = 0xFF; // a good received frame overwrites this with 0x00
    // now attempt to receive 24 bytes, or time out and abort after 4ms
    int i = 0;
    while (i < 24)
    {
      if (millis() > timeout)
      {
        break;
      }
      if (Serial.available())
      {
        i += Serial.readBytes(frame + i, min(Serial.available(), 24 - i));
      }
      //delay(1); // Задержка для разгрузки процессора
    }

    if (i == 24 && frame[23] == 0x000)
    {
      frameReceived = true;
      break;
    }
    else
    {
      ++sbusFails;
    }
  }

  if (frameReceived)
  {
    bool canRead = true;
    if (frame[22] & 0x08) 
    {
      failsafeMarker = 4000;
      //delay(1);
      canRead = false;
      failsafe = true;
    }
    if (frame[22] & 0x04) 
    {
      canRead = false;
      //++sbusFails;
    }

    // copy first 7 channels received in SBUS frame to CPPM outputs
    if(canRead)
    {
      for (int i = 0; i < 7; i++)
      {
        sbus[i] = channel(i);
        failsafeMarker = 0;
      }
    }
  }

  if(failsafeMarker < 4000) ++failsafeMarker;
  operateThrottle();
  operateHazardLights(failsafeMarker > 3000);
  operateHeadLights();
  serv.handleLoop();
  serv.sendSbusFailUpdate(sbusFails, failsafe);
}
