#include "webServer.h"
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ArduinoJson.h>

const char *ssid = "CherokeeLC";
const char *password = "00000000";

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81); // WebSocket на порту 81

int lastSbus[7] = {0};

WebServer::WebServer()
{
  begin();
}

void WebServer::begin()
{
  setupWebSocket();
  setupRoutes();
}

void WebServer::enable()
{
  if (serverActive)
    return;

  WiFi.softAP(ssid, password);
  Serial.println("SoftAP started: " + WiFi.softAPIP().toString());

  server.begin();
  webSocket.begin();
  serverActive = true;
  Serial.println("Web server enabled");
}

void WebServer::disable()
{
  if (!serverActive)
    return;

  webSocket.disconnect();
  server.stop();
  WiFi.softAPdisconnect(true);
  serverActive = false;
  Serial.println("Web server disabled");
}

bool WebServer::isEnabled() const
{
  return serverActive;
}

void WebServer::setupWebSocket()
{
  webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t *payload, size_t length)
                    {
    if (type == WStype_TEXT) {
      handleWSMessage(num, payload, length);
    } });
}

void WebServer::setupRoutes()
{
  server.on("/", HTTP_GET, []()
            { server.send_P(200, "text/html", html); });
}

// Функция для конвертации строки в число (например, d4 -> 4)
int getPinFromString(const String &pinStr)
{
  if (pinStr.startsWith("d"))
  {
    return pinStr.substring(1).toInt(); // удаляем 'd' и конвертируем в число
  }
  return -1; // если строка не начинается с "d", возвращаем -1 (не используется)
}

// Функция для конвертации строки в число (например, d4 -> 4)
int getChannelFromString(const String &pinStr)
{
  return pinStr.toInt(); // удаляем 'd' и конвертируем в число
}

// Функция для конвертации числа в строку (например, 4 -> d4)
String getStringFromPin(int pin)
{
  return "d" + String(pin);
}

// Метод для установки коллбэка
void WebServer::setSettingsCallback(std::function<void()> callback)
{
  settingsCallback = callback;
}

void WebServer::handleWSMessage(uint8_t clientNum, uint8_t *data, size_t len)
{
  String msg = String((char *)data);

  if (!msg.startsWith("{"))
    return;

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, msg);

  if (error)
  {
    Serial.println("JSON Parse Failed");
    return;
  }

  String type = doc["type"];

  if (type == "setParams")
  {
    String debugOut;
    serializeJsonPretty(doc, debugOut);
    Serial.println("Received JSON:");
    Serial.println(debugOut);
    // Получаем новые значения настроек и обновляем объект settings

    sets.throttle.channel = doc["throttleChannel"] | -1;
    sets.throttle.inverted = doc["throttleInverted"] | false;
    sets.throttle.centerPoint = doc["throttleChannelCenterPoint"] | 1024;
    Serial.println(sets.throttle.channel);

    sets.gearbox.channel = doc["gearboxChannel"] | -1;
    sets.gearbox.inverted = doc["gearboxInverted"] | false;
    sets.gearbox.centerPoint = doc["gearboxChannelCenterPoint"] | 1024;

    sets.frontLock.channel = doc["frontLockChannel"] | -1;
    sets.frontLock.inverted = doc["frontLockInverted"] | false;
    sets.frontLock.centerPoint = doc["frontLockChannelCenterPoint"] | 1024;

    sets.rearLock.channel = doc["rearLockChannel"] | -1;
    sets.rearLock.inverted = doc["rearLockInverted"] | false;
    sets.rearLock.centerPoint = doc["rearLockChannelCenterPoint"] | 1024;

    sets.enableServer.channel = doc["enableServerChannel"] | -1;
    sets.enableServer.inverted = doc["enableServerInverted"] | false;
    sets.enableServer.centerPoint = doc["enableServerChannelCenterPoint"] | 1024;

    sets.blinkDelayOn = doc["blinkDelayOn"] | 700;
    sets.blinkDelayOff = doc["blinkDelayOff"] | 700;
    sets.stopLightDelay = doc["stopLightDelay"] | 3000;
    sets.tailLightPwm = constrain(doc["tailLightPwm"] | 100, 0, 100);
    sets.throttleHysteresis = doc["throttleHysteresis"] | 0;

    sets.frontHazardLights = getPinFromString(doc["frontHazardLights"] | "d4");
    sets.rearHazardLights = getPinFromString(doc["rearHazardLights"] | "d5");
    sets.foglight = getPinFromString(doc["foglight"] | "d14");
    sets.headlight = getPinFromString(doc["headlight"] | "d15");
    sets.stopLights = getPinFromString(doc["stopLights"] | "d12");
    sets.rearLights = getPinFromString(doc["rearLights"] | "d13");

    Serial.println("Настройки обновлены.");

    // Если есть коллбэк, передаем обновленные настройки наружу
    if (settingsCallback)
      settingsCallback();
  }
  else if (type == "reboot")
  {
    Serial.println("Reboot command received");
    WiFi.forceSleepBegin();
    wdt_reset();
    ESP.restart();
  }
  else if (type == "shutdown")
  {
    disable();
  }
  else if (type == "wtfmem")
  {
    sendHeapUpdate();
  }
  else if (type == "getParams")
  {
    JsonDocument doc;
    doc["type"] = "params";

    doc["throttleChannel"] = sets.throttle.channel;
    doc["throttleInverted"] = sets.throttle.inverted;
    doc["throttleChannelCenterPoint"] = sets.throttle.centerPoint;

    doc["gearboxChannel"] = sets.gearbox.channel;
    doc["gearboxInverted"] = sets.gearbox.inverted;
    doc["gearboxChannelCenterPoint"] = sets.gearbox.centerPoint;

    doc["frontLockChannel"] = sets.frontLock.channel;
    doc["frontLockInverted"] = sets.frontLock.inverted;
    doc["frontLockChannelCenterPoint"] = sets.frontLock.centerPoint;

    doc["rearLockChannel"] = sets.rearLock.channel;
    doc["rearLockInverted"] = sets.rearLock.inverted;
    doc["rearLockChannelCenterPoint"] = sets.rearLock.centerPoint;

    doc["enableServerChannel"] = sets.enableServer.channel;
    doc["enableServerInverted"] = sets.enableServer.inverted;
    doc["enableServerChannelCenterPoint"] = sets.enableServer.centerPoint;

    doc["blinkDelayOn"] = sets.blinkDelayOn;
    doc["blinkDelayOff"] = sets.blinkDelayOff;
    doc["stopLightDelay"] = sets.stopLightDelay;
    doc["tailLightPwm"] = sets.tailLightPwm;
    doc["throttleHysteresis"] = sets.throttleHysteresis;

    doc["frontHazardLights"] = getStringFromPin(sets.frontHazardLights);
    doc["rearHazardLights"] = getStringFromPin(sets.rearHazardLights);
    doc["foglight"] = getStringFromPin(sets.foglight);
    doc["headlight"] = getStringFromPin(sets.headlight);
    doc["stopLights"] = getStringFromPin(sets.stopLights);
    doc["rearLights"] = getStringFromPin(sets.rearLights);
    String out;
    serializeJson(doc, out);
    webSocket.sendTXT(clientNum, out);
  }
}

void WebServer::sendChannelUpdate()
{
  if (!serverActive)
    return;

  String json = "{";
  for (int i = 0; i < 7; i++)
  {
    json += "\"ch" + String(i) + "\":" + String(sbus[i]);
    if (i < 6)
      json += ",";
  }
  json += "}";
  webSocket.broadcastTXT(json);
}

void WebServer::sendHeapUpdate()
{
  if (!serverActive)
    return;
  String json = "{\"heap\":" + String(ESP.getFreeHeap()) + "}";
  webSocket.broadcastTXT(json);
}

void WebServer::sendSbusFailUpdate(unsigned int fails)
{
  if (!serverActive || millis() - lastSbusFails < 300)
    return;
  lastSbusFails = millis();
  String json = "{\"sbusFail\":" + String(fails) + "}";
  webSocket.broadcastTXT(json);
}

unsigned long lastUptimeSent = 0;

void WebServer::handleLoop()
{
  if (!serverActive)
    return;

  webSocket.loop();
  server.handleClient();

  bool changed = false;
  for (int i = 0; i < 7; i++)
  {
    if (sbus[i] != lastSbus[i])
    {
      changed = true;
      break;
    }
  }

  if (changed)
  {
    memcpy(lastSbus, sbus, sizeof(sbus));
    sendChannelUpdate();
  }

  unsigned long now = millis();
  if (now - lastUptimeSent > 1000) {
    lastUptimeSent = now;

    JsonDocument doc;
    doc["uptime"] = now / 1000; // в секундах

    String json;
    serializeJson(doc, json);
    webSocket.broadcastTXT(json);
  }
}