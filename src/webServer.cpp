#include "webServer.h"
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ArduinoJson.h>

const char *ssid = "CherokeeLC";
const char *password = "00000000";

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81); // WebSocket на порту 81

int lastSbus[8] = {0};

const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Cherokee Light Controller</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; padding: 20px; }
    .channel, .setting { margin: 10px 0; }
    button { padding: 1em 2em; font-size: 1.2rem; margin-top: 0.8rem; }
    #heapInfo { margin-top: 10px; font-weight: bold; color: #333; }
  </style>
</head>
<body>
  <h2>SBUS Channels</h2>
  <div id="channels"></div>

  <h2>Состояние системы</h2>
  <button type="button" onclick="wtfmem()">Чо по памяти?</button>
  <div id="heapInfo">Память: ?</div>
  <div id="sbusFail">Ошибок sbus: ?</div>

  <h2>Настройки параметров</h2>
  <form id="paramForm">
    <h3>Каналы</h3>
    <div>
      throttle: <select id="throttleChannel" name="throttleChannel"></select>
      <label><input type="checkbox" name="throttleInverted">Инверсия логики</label>
      Центр: <input type="number" name="throttleChannelCenterPoint" min="0" max="2048" value="1024">
    </div>
    <div>
      gearbox: <select id="gearboxChannel" name="gearboxChannel"></select>
      <label><input type="checkbox" name="gearboxInverted">Инверсия логики</label>
      Центр: <input type="number" name="gearboxChannelCenterPoint" min="0" max="2048" value="1024">
    </div>
    <div>
      frontLock: <select id="frontLockChannel" name="frontLockChannel"></select>
      <label><input type="checkbox" name="frontLockInverted">Инверсия логики</label>
      Центр: <input type="number" name="frontLockChannelCenterPoint" min="0" max="2048" value="1024">
    </div>
    <div>
      rearLock: <select id="rearLockChannel" name="rearLockChannel"></select>
      <label><input type="checkbox" name="rearLockInverted">Инверсия логики</label>
      Центр: <input type="number" name="rearLockChannelCenterPoint" min="0" max="2048" value="1024">
    </div>
    <div>
      enableServer: <select id="enableServerChannel" name="enableServerChannel"></select>
      <label><input type="checkbox" name="enableServerInverted">Инверсия логики</label>
      Центр: <input type="number" name="enableServerChannelCenterPoint" min="0" max="2048" value="1024">
    </div>

    <h3>Задержки и уровни</h3>
    <div>Задержка поворотника вкл: <input name="blinkDelayOn" type="number" value="700" min="0" max="10000"></div>
    <div>Задержка поворотника выкл: <input name="blinkDelayOff" type="number" value="700" min="0" max="10000"></div>
    <div>Задержка стопов: <input name="stopLightDelay" type="number" value="3000" min="0" max="10000"></div>
    <div>Яркость габаритов: <input name="tailLightPwm" type="number" value="100" min="0" max="100">%</div>
    <div>Гистерезис газа: <input name="throttleHysteresis" type="number" value="0" min="-100" max="100"></div>

    <h3>Выходы</h3>
    <div>frontHazardLights: <select id="frontHazardLights" name="frontHazardLights"></select></div>
    <div>rearHazardLights: <select id="rearHazardLights" name="rearHazardLights"></select></div>
    <div>headlightFog: <select id="headlightFog" name="headlightFog"></select></div>
    <div>stopLights: <select id="stopLights" name="stopLights"></select></div>
    <div>rearLights: <select id="rearLights" name="rearLights"></select></div>

    <button type="button" onclick="saveSettings()">Сохранить параметры</button>
  </form>
  <form>
  <button type="button" onclick="reboot()">Перезагрузить</button>
  <button type="button" onclick="shutdown()">Отключить точку доступа и сервер</button>
  </form>

  <script>
    const ws = new WebSocket("ws://" + location.host + ":81");

    ws.onmessage = function(event) {
      try {
        const data = JSON.parse(event.data);
        // Обновление каналов
        if ("ch0" in data) {
          let html = "";
          for (let i = 0; i < 8; i++) {
            const val = data["ch" + i];
            html += `<div class='channel'>Канал ${i}: ${val !== undefined ? val : "—"}</div>`;
          }
          document.getElementById("channels").innerHTML = html;
        }

        if ("heap" in data) {
          document.getElementById("heapInfo").innerText = "Память: " + data.heap + " байт";
        }

        if ("sbusFail" in data) {
          document.getElementById("sbusFail").innerText = "Ошибок sbus: " + data.sbusFail;
        }

        if (data.type === "params") {
          for (const key in data) {
            if (key === "type") continue;

            const el = document.querySelector(`[name="${key}"]`);
            if (!el) continue;

            if (el.type === "checkbox") {
              el.checked = data[key];
            } else {
              el.value = String(data[key]);
            }
          }
        }
      } catch (e) {
        console.log("Not JSON:", event.data);
      }
    };

    function saveSettings() {
       if (!confirm("Вы уверены, что хотите сохранить настройки?")) return;

      const form = document.getElementById("paramForm");
      const formData = new FormData(form);
      let obj = { type: "setParams" };
      formData.forEach((v, k) => {
        if (v === "on") obj[k] = true;
        else obj[k] = isNaN(v) ? v : Number(v);
      });
      ws.send(JSON.stringify(obj));
      alert("Настройки отправлены.");
    }

    function reboot() {
      if (!confirm("Вы уверены, что хотите перезагрузить устройство?")) return;
      ws.send(JSON.stringify({ type: "reboot" }));
    }

    function wtfmem() {
      ws.send(JSON.stringify({ type: "wtfmem" }));
    }

    function shutdown() {
      if (!confirm("Отключить точку доступа и остановить сервер?")) return;
      ws.send(JSON.stringify({ type: "shutdown" }));
    }

    ws.onopen = () => {
      ws.send(JSON.stringify({ type: "getParams" }));
    };

    window.onload = () => {
      const channelFields = ["throttle", "gearbox", "frontLock", "rearLock", "enableServer"];
      channelFields.forEach(name => {
        const sel = document.getElementById(`${name}Channel`);
        sel.innerHTML = `<option value="-1">Не используется</option>` + [...Array(8)].map((_, i) =>
          `<option value="${i}">Канал ${i}</option>`).join("");
      });

      const outputs = ["d4", "d5", "d12", "d13", "d15"];
      const outFields = ["frontHazardLights", "rearHazardLights", "headlightFog", "stopLights", "rearLights"];
      outFields.forEach(id => {
        const sel = document.getElementById(id);
        outputs.forEach(pin => {
          const opt = document.createElement("option");
          opt.value = pin;
          opt.text = pin + (pin === "d15" ? "+d2" : "");
          sel.appendChild(opt);
        });
      });
    };
  </script>
</body>
</html>
)rawliteral";

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
    sets.headlightFog = getPinFromString(doc["headlightFog"] | "d15");
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

    doc["frontHazardLights"] = sets.frontHazardLights;
    doc["rearHazardLights"] = sets.rearHazardLights;
    doc["headlightFog"] = sets.headlightFog;
    doc["stopLights"] = sets.stopLights;
    doc["rearLights"] = sets.rearLights;
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
  for (int i = 0; i < 8; i++)
  {
    json += "\"ch" + String(i) + "\":" + String(sbus[i]);
    if (i < 7)
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

void WebServer::handleLoop()
{
  if (!serverActive)
    return;

  webSocket.loop();
  server.handleClient();

  bool changed = false;
  for (int i = 0; i < 8; i++)
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
}