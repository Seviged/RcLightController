#ifndef WEBSERVERS_H
#define WEBSERVERS_H
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include "eepromStorage.h"

class WebServer {
public:
    WebServer();

    void begin();                // Инициализация
    void handleLoop();           // Вызывать в loop для отправки изменений

    void enable();               // Включить точку доступа и сервер
    void disable();              // Отключить точку доступа и сервер
    bool isEnabled() const;      // Проверить состояние

    void sendSbusFailUpdate(unsigned int fails);
    void setSettingsCallback(std::function<void()> callback); // Новый метод для коллбэка


private:
    void setupWebSocket();
    void setupRoutes();
    void handleWSMessage(uint8_t clientNum, uint8_t *data, size_t len);
    void sendChannelUpdate();
    void sendHeapUpdate();

    bool serverActive = false;

    std::function<void()> settingsCallback;  // Коллбэк для передачи настроек наружу
    unsigned long lastHeap = 0;
    unsigned long lastSbusFails = 0;
};

extern int sbus[8];
extern int outputBindings[4];
extern Settings sets;

#endif