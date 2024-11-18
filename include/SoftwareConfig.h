
#ifndef SOFTWARECONFIG_H
#define SOFTWARECONFIG_H

#include <Arduino.h>
#include "Config.h"

// Public functions
sConfig_t InitDefaultConfig();
sConfig_t ReadConfig(    int address);
void      WriteConfig(   int address, sConfig_t Config);
bool      UpdateConfig(  sConfig_t Config);
bool      isConfigValid( sConfig_t Config);
void      PrintConfig(   sConfig_t Config);

#endif