#ifndef WEBSERVERS_H
#define WEBSERVERS_H
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "eepromStorage.h"

class WebServer
{
public:
    WebServer();

    void begin();      // Инициализация
    void handleLoop(); // Вызывать в loop для отправки изменений

    void enable();          // Включить точку доступа и сервер
    void disable();         // Отключить точку доступа и сервер
    bool isEnabled() const; // Проверить состояние

    void sendSbusFailUpdate(long fails, bool failsafe, bool force = false);
    void setSettingsCallback(std::function<void(bool)> callback); // Новый метод для коллбэка

private:
    void setupWebSocket();
    void setupRoutes();
    void handleWSMessage(uint8_t clientNum, uint8_t *data, size_t len);
    void sendChannelUpdate();
    void sendHeapUpdate();

    bool serverActive = false;

    std::function<void(bool)> settingsCallback; // Коллбэк для передачи настроек наружу
    unsigned long lastHeap = 0;
    unsigned long lastSbusFails = 0;
    bool isFailsafe = false;
    long someFails = -1;

    bool saveSettingsFlag = false;
    bool eraseSettingsFlag = false;
    bool getMemFlag = false;
    bool rebootFlag = false;
    bool disableServerFlag = false;
    bool updateInitValuesFlag = false;
};

extern int sbus[7];
extern Settings sets;

const char html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
      <meta charset="UTF-8">
      <title>Cherokee Light Controller</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        * { box-sizing: border-box; }
        body {
          font-family: sans-serif;
          margin: 0;
          padding: 20px;
          background: #f5f5f5;
          color: #333;
        }
    
        h2, h3 {
          margin-top: 1.5rem;
          color: #222;
        }
    
        .card {
          background: white;
          border-radius: 10px;
          padding: 1rem 1.5rem;
          margin-bottom: 1.5rem;
          box-shadow: 0 2px 6px rgba(0,0,0,0.1);
        }
    
        .channel, .setting {
          margin: 0.5rem 0;
        }
    
        label {
          display: inline-flex;
          align-items: center;
          margin-left: 1rem;
        }
    
        input[type="number"], select {
          padding: 0.4em;
          font-size: 1rem;
          margin-left: 0.5rem;
        }
    
        button {
          padding: 0.8rem 1.2rem;
          font-size: 1rem;
          border: none;
          background-color: #007BFF;
          color: white;
          border-radius: 5px;
          cursor: pointer;
          margin: 0.5rem 0.3rem;
        }
    
        button:hover {
          background-color: #0056b3;
        }
    
        .btn-group {
          display: flex;
          flex-wrap: wrap;
          gap: 0.5rem;
        }
    
        #heapInfo, #sbusFail {
          font-weight: bold;
          margin-top: 0.5rem;
        }
    
        .form-group {
          display: grid;
          grid-template-columns: 1fr 1fr;
          gap: 1rem;
        }
    
        .form-subgroup {
          display: flex;
          flex-direction: column;
          gap: 0.5rem;
        }
    
        @media (max-width: 700px) {
          .form-group {
            grid-template-columns: 1fr;
          }
    
          .btn-group {
            flex-direction: column;
          }
        }
      </style>
    </head>
    <body>
      <h1>Cherokee Light Controller</h1>
    
      <div class="card">
        <h2>Каналы SBUS</h2>
        <div id="channels"></div>
      </div>
    
      <div class="card">
        <h2>Состояние системы</h2>
        <div class="btn-group">
          <button onclick="wtfmem()">Чо по памяти?</button>
        </div>
        <div id="heapInfo">Память: ?</div>
        <div id="sbusFail">Ошибок sbus: ?</div>
        <div id="failsafe">Failsafe: ?</div>
        <div>Uptime: <span id="uptime">—</span></div>
      </div>
    
      <div class="card">
        <h2>Настройки</h2>
        <form id="paramForm">
          <h3>Каналы управления</h3>
          <div class="form-group">
            <div class="form-subgroup">
              throttle:
              <select id="throttleChannel" name="throttleChannel"></select>
              <label><input type="checkbox" name="throttleInverted">Инверсия</label>
              Центр: <input type="number" name="throttleChannelCenterPoint" min="0" max="2048" value="1024">
            </div>
            <div class="form-subgroup">
              gearbox:
              <select id="gearboxChannel" name="gearboxChannel"></select>
              <label><input type="checkbox" name="gearboxInverted">Инверсия</label>
              Центр: <input type="number" name="gearboxChannelCenterPoint" min="0" max="2048" value="1024">
            </div>
            <div class="form-subgroup">
              frontLock:
              <select id="frontLockChannel" name="frontLockChannel"></select>
              <label><input type="checkbox" name="frontLockInverted">Инверсия</label>
              Центр: <input type="number" name="frontLockChannelCenterPoint" min="0" max="2048" value="1024">
            </div>
            <div class="form-subgroup">
              rearLock:
              <select id="rearLockChannel" name="rearLockChannel"></select>
              <label><input type="checkbox" name="rearLockInverted">Инверсия</label>
              Центр: <input type="number" name="rearLockChannelCenterPoint" min="0" max="2048" value="1024">
            </div>
            <div class="form-subgroup">
              enableServer:
              <select id="enableServerChannel" name="enableServerChannel"></select>
              <label><input type="checkbox" name="enableServerInverted">Инверсия</label>
              Центр: <input type="number" name="enableServerChannelCenterPoint" min="0" max="2048" value="1024">
            </div>
          </div>
    
          <h3>Параметры</h3>
          <div class="form-group">
            <div class="form-subgroup">
              Задержка поворотника вкл (мс): <input name="blinkDelayOn" type="number" value="700" min="0" max="10000">
              Задержка поворотника выкл (мс): <input name="blinkDelayOff" type="number" value="700" min="0" max="10000">
              Задержка стопов (мс): <input name="stopLightDelay" type="number" value="3000" min="0" max="10000">
            </div>
            <div class="form-subgroup">
              Яркость габаритов (0%-100%): <input name="tailLightPwm" type="number" value="100" min="0" max="100">
              Гистерезис газа (ед.): <input name="throttleHysteresis" type="number" value="0" min="-1000" max="1000">
            </div>
            <div class="form-subgroup">
              <label><input type="checkbox" name="headlightAtStart">Вкл. ближн. света при запуске</label>
              <label><input type="checkbox" name="foglightAtStart">Вкл. противотум. фар при запуске</label>
            </div>
          </div>
    
          <h3>Выходы</h3>
          <div class="form-group">
            <div class="form-subgroup">
              frontHazardLights: <select id="frontHazardLights" name="frontHazardLights"></select>
              rearHazardLights: <select id="rearHazardLights" name="rearHazardLights"></select>
              foglight: <select id="foglight" name="foglight"></select>
            </div>
            <div class="form-subgroup">
              headlight: <select id="headlight" name="headlight"></select>
              stopLights: <select id="stopLights" name="stopLights"></select>
              rearLights: <select id="rearLights" name="rearLights"></select>
            </div>
          </div>
    
          <div class="btn-group">
            <button type="button" onclick="saveSettings()">Сохранить параметры</button>
            <button type="button" onclick="resetEeprom()">Вернуть параметры по умолчанию</button>
          </div>
        </form>
      </div>
    
      <div class="card">
        <h2>Управление системой</h2>
        <div class="btn-group">
          <button type="button" onclick="reboot()">Перезагрузить</button>
          <button type="button" onclick="shutdown()">Отключить точку доступа и сервер</button>
        </div>
      </div>
    
      <script>
        let ws;
        let reconnectInterval = 3000; // Интервал переподключения в мс

        function initWebSocket() {
        ws = new WebSocket("ws://" + location.host + ":81");

        ws.onopen = () => {
            console.log("WebSocket подключен");
            ws.send(JSON.stringify({ type: "getParams" }));
        };

        ws.onmessage = function(event) {
            try {
            const data = JSON.parse(event.data);
            if ("ch0" in data) {
                let html = "";
                for (let i = 0; i < 7; i++) {
                html += `<div class='channel'>Канал ${i}: ${data["ch" + i] ?? "—"}</div>`;
                }
                document.getElementById("channels").innerHTML = html;
            }
            if ("uptime" in data) {
                const uptimeEl = document.getElementById("uptime");
                const seconds = Number(data.uptime);
                const minutes = Math.floor(seconds / 60);
                const hours = Math.floor(minutes / 60);
                uptimeEl.textContent = `${hours}h ${minutes % 60}m ${seconds % 60}s`;
            }
            if ("heap" in data) document.getElementById("heapInfo").innerText = "Память: " + data.heap + " байт";
            if ("sbusFail" in data) document.getElementById("sbusFail").innerText = "Ошибок sbus: " + data.sbusFail;
            if ("failsafe" in data) document.getElementById("failsafe").innerText = "Failsafe: " + (data.failsafe > 0 ? "true" : "false");
            if (data.type === "params") {
                for (const key in data) {
                if (key === "type") continue;
                const el = document.querySelector(`[name="${key}"]`);
                if (!el) continue;
                el.type === "checkbox" ? el.checked = data[key] : el.value = String(data[key]);
                }
            }
            } catch (e) {
            console.log("Not JSON:", event.data);
            }
        };

        ws.onclose = function() {
            console.warn("WebSocket отключен. Попытка переподключения через " + reconnectInterval / 1000 + " сек...");
            setTimeout(initWebSocket, reconnectInterval);
        };

        ws.onerror = function(err) {
            console.error("WebSocket ошибка:", err);
            ws.close();
        };
        }
    
        function saveSettings() {
            if (!confirm("Вы уверены, что хотите сохранить настройки?")) return;
            const form = document.getElementById("paramForm");
            const formData = new FormData(form);
            let obj = { type: "setParams" };

            formData.forEach((v, k) => {
                obj[k] = isNaN(v) ? v : Number(v);
            });

            form.querySelectorAll('input[type="checkbox"]').forEach(cb => {
                obj[cb.name] = cb.checked;
            });

            console.log("Отправка настроек:", obj); // Добавлено для отладки
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

        function resetEeprom() {
          if (!confirm("Записать в EEPROM значения по умолчанию? Это полностью сотрет конфигурацию, и контроллер будет перезагружен!")) return;
          ws.send(JSON.stringify({ type: "reseteeprom" }));
        }
    
        window.onload = () => {
          initWebSocket();  
          const channels = ["throttle", "gearbox", "frontLock", "rearLock", "enableServer"];
          channels.forEach(name => {
            const sel = document.getElementById(`${name}Channel`);
            sel.innerHTML = '<option value="-1">Не используется</option>' + [...Array(7)].map((_, i) =>
              `<option value="${i}">Канал ${i}</option>`).join("");
          });
    
          const outputs = ["d4", "d5", "d12", "d13", "d14", "d15"];
          const outFields = ["frontHazardLights", "rearHazardLights", "foglight", "headlight", "stopLights", "rearLights"];
          outFields.forEach(id => {
            const sel = document.getElementById(id);
            outputs.forEach(pin => {
              const opt = document.createElement("option");
              opt.value = pin;
              opt.text = pin;
              sel.appendChild(opt);
            });
          });
        };
      </script>
    </body>
    </html>
    )rawliteral";

#endif