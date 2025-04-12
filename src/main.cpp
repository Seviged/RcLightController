#include <Arduino.h>
#include "eepromStorage.h"
#include "lightController.h"

int sbus[7] = {0};

#include "webServer.h"
WebServer serv;
unsigned int sbusFails = 0;

EepromStorage store;
Settings sets;
uint8_t frame[24]; // SBUS received bytes. 1st byte of 25-byte frame is always 0x0F, and is not stored

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
const unsigned long HOLD_TIME = 4000; // 4 секунды
const unsigned long LIGHTS_HOLD_TIME = 2000; // 2 секунды

void enableServerFunc()
{
  if (serv.isEnabled())
    return;

  if(sets.enableServer.channel < 0)
  {
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

    if (millis() - enableTimerServer >= HOLD_TIME)
    {
      Serial.println("ENABLING SERVER...");
      blinkRearLights();
      serv.enable(); // включаем сервер
      Serial.println("SERVER ENABLED!!!");
    }
  }

  // если значение опустилось ниже нижнего порога — сбрасываем
  if (ch < sets.enableServer.centerPoint)
  {
    if (triggerStarted)
      Serial.println("Hold cancelled (debounced)");
    triggerStarted = false;
    enableTimerServer = 0;

    if (millis() - enableTimerServer >= LIGHTS_HOLD_TIME)
    {
      operateHeadLights(true);
    }
  }
}

void writeSettings()
{
  store.writeSettings(sets);
}

void setup()
{
  Serial.begin(100000, SERIAL_8E2);
  delay(100);

  setupOutputs();

  store.readSettings(sets);
  serv.setSettingsCallback(writeSettings);
  delay(300);
}

void loop()
{
  enableServerFunc();

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
      delay(1); // Задержка для разгрузки процессора
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
    // copy first 7 channels received in SBUS frame to CPPM outputs
    for (int i = 0; i < 7; i++)
    {
      sbus[i] = channel(i);
    }
  }
  operateThrottle();
  operateHazardLights();
  operateHeadLights();
  serv.sendSbusFailUpdate(sbusFails);
  serv.handleLoop();
}
