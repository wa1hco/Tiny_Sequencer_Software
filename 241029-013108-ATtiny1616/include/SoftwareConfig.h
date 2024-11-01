
#ifndef SoftwareConfig_h
#define SoftwareConfig_h

#include <Arduino.h>
#include "Config.h"

// Public functions
sConfig_t InitConfigStruct(sConfig_t Config);
sConfig_t ReadConfig(                        int address);
void      WriteConfig(     sConfig_t Config, int address);
bool      isConfigValid(   sConfig_t Config);
void      PrintConfig(     sConfig_t Config);


#endif