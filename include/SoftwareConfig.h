
#ifndef SOFTWARECONFIG_H
#define SOFTWARECONFIG_H

#include <Arduino.h>
#include "Config.h"

// Public functions
sConfig_t InitDefaultConfig();             // initialze config structure in memory
sConfig_t GetConfig(uint8_t address); // read config from EEPROM
void PutConfig(uint8_t address, sConfig_t Config); // write config if needed, update CRC16
uint16_t CalcCRC(sConfig_t Config);
bool isConfigValid(sConfig_t Config); // check config.CRC16
void PrintConfig(sConfig_t Config); // pretty print config on serial port

#endif
