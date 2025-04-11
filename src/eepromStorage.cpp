#include "eepromStorage.h"
#include <Arduino.h>
#include <ESP_EEPROM.h>


EepromStorage::EepromStorage() 
{
    EEPROM.begin(sizeof(Settings));    
}

void EepromStorage::readSettings(Settings &sets) {
    EEPROM.get(0, sets);
}

bool EepromStorage::writeSettings(Settings &sets) {
    EEPROM.put(0, sets);
    bool ok = EEPROM.commit();
    Serial.println((ok) ? "Commit OK" : "Commit failed");
    return ok;
}

bool EepromStorage::restoreDefaultSettings() {
    Settings sets;
    return writeSettings(sets);
}
