#ifndef EESTOR_H
#define EESTOR_H

struct ChannelConfig {
    int channel = -1;   // канал SBUS (или -1, если не используется)
    bool inverted = false; // инвертирование
    int centerPoint = 1024; // центр
};

struct Settings {
    ChannelConfig throttle;
    ChannelConfig gearbox;
    ChannelConfig frontLock;
    ChannelConfig rearLock;
    ChannelConfig enableServer;

    int blinkDelayOn = 700;  // по умолчанию
    int blinkDelayOff = 700; // по умолчанию
    int stopLightDelay = 3000; // по умолчанию
    int tailLightPwm = 20; // 0-100
    int throttleHysteresis = 50; // +-100

    int frontHazardLights = 4;  // d4 (например)
    int rearHazardLights = 5;   // d5
    int headlightFog = 15;      // d15
    int stopLights = 12;        // d12
    int rearLights = 13;        // d13
};


class EepromStorage
{
public:
    EepromStorage();

    void readSettings(Settings &sets);
    bool writeSettings(Settings &sets);
    bool restoreDefaultSettings();


};


#endif