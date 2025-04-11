#include <Arduino.h>
#include <SoftwareSerial.h>
#include "eepromStorage.h"
#include "lightController.h"

int sbus[8] = {0};
int outputBindings[4] = {0, 1, 2, 3}; // Привязка выходов к каналам SBUS

#include "webServer.h"
WebServer serv;
unsigned int sbusFails = 0;

EepromStorage store;
Settings sets;

#define RX (14)

//EspSoftwareSerial::UART sbusSerial;
uint8_t frame[24];  // SBUS received bytes. 1st byte of 25-byte frame is always 0x0F, and is not stored



int channel(int ch) {       // extract 11-bit channel[ch] value from frame. ch 0-15
  int k = ch > 7 ? 11 : 0;  // offset into frame array: 0 for channels 0-7, 12 for channels 8-15
  switch (ch % 8) {         // pattern repeats (except for k-offset) after 8 channels
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
  return -1;  // execution never reaches here, but this supresses a compliler warning
}

unsigned long enableTimerServer = 0;
bool triggerStarted = false;

// параметры фильтра
const int TRIGGER_THRESHOLD = 1500;
const int HYSTERESIS = 50;
const unsigned long HOLD_TIME = 4000; // 4 секунды

void enableServerFunc() {
  int ch = sbus[3];
  //Serial.println(ch); // отладка

  if (serv.isEnabled()) return;

  // если значение превышает порог с гистерезисом — начинаем отсчёт
  if (ch > TRIGGER_THRESHOLD + HYSTERESIS) {
    if (!triggerStarted) {
      enableTimerServer = millis();
      triggerStarted = true;
      Serial.println("Start hold (debounced)");
    }

    if (millis() - enableTimerServer >= HOLD_TIME) {
      Serial.println("ENABLING SERVER...");
      serv.enable(); // включаем сервер
      Serial.println("SERVER ENABLED!!!");
    }
  }

  // если значение опустилось ниже нижнего порога — сбрасываем
  if (ch < TRIGGER_THRESHOLD - HYSTERESIS) {
    if (triggerStarted) Serial.println("Hold cancelled (debounced)");
    triggerStarted = false;
    enableTimerServer = 0;
  }
}

void writeSettings()
{
  store.writeSettings(sets);
}

void setup() {
  //Serial.begin(74880);
  Serial.begin(100000, SERIAL_8E2);
  //pinMode(12, OUTPUT);
  //digitalWrite(12, LOW);
  // Get current baud rate
	//sbusSerial.begin(100000, EspSoftwareSerial::SWSERIAL_8E2, RX, -1, false);
	// Only half duplex this way, but reliable TX timings for high bps
	//sbusSerial.enableIntTx(false);
  delay(100);
  //analogWrite(12, 127);

  setupOutputs();

  store.readSettings(sets);
  sets.gearbox.centerPoint = 666;
  serv.setSettingsCallback(writeSettings);
  delay(300);
}

void loop() {
  //delay(1000);
  //digitalWrite(12, HIGH);
  //delay(2000);
  //digitalWrite(12, LOW);
 // server.handleClient();
  //int br = Serial.baudRate();
  // Will print "Serial is 57600 bps"
  //Serial.printf("Serial is %d bps", br);
  //Serial.println("Software serial onReceive() event test started");

  //Serial.print("Free heap: ");
//Serial.println(ESP.getFreeHeap());

enableServerFunc();

bool frameReceived = false;
while (Serial.available()) 
{
  //digitalWrite(2, HIGH);
  // try to receive a 25-byte frame that begins with 0x0F and ends with 0x00
  // scan until an 0x0F byte is received, or the receive buffer is empty
  int b = Serial.read();
  //Serial.print(b,HEX);
  //Serial.printf(" ");
  if (b != 0x0F) 
  {
    continue;
  }
  unsigned long timeout = millis() + 4000;
  //Serial.printf("0x0F FOUND\n");
  frame[23] = 0xFF;  // a good received frame overwrites this with 0x00
  // now attempt to receive 24 bytes, or time out and abort after 4ms
  int i = 0;
 while (i < 24) {
  if (millis() > timeout) {
    //Serial.printf("Timeout exceeded\n");
    //++sbusFails;
    break;
  }
  if (Serial.available()) {
    i += Serial.readBytes(frame + i, min(Serial.available(), 24 - i));
  }
  delay(1);  // Задержка для разгрузки процессора
}
  if (i == 24 && frame[23] == 0x000) {
    frameReceived = true;
    break;
  }
  else
  {
    ++sbusFails;
    //Serial.printf(" checksumErr\n");
  }
}

if (frameReceived) {
    // copy first 8 channels received in SBUS frame to CPPM outputs

    for (int i = 0; i < 8; i++) {
      sbus[i] = channel(i);
      //if(i == 7) Serial.println(sbus[i]);
      //else {Serial.print(sbus[i]);
      //Serial.print(" ");
      //}
    }    
    //analogWrite(12, sbus[1]/8);
    
  }
  operateThrottle();
  operateHazardLights();
  serv.sendSbusFailUpdate(sbusFails); 
  serv.handleLoop();
}
