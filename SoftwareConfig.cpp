
#include <Arduino.h>
#include <EEPROM.h>
#include <CRC16.h>
#include <CRC.h>

#include "Config.h"
#include "SoftwareConfig.h"

// Write Config to EEPROM
void WriteConfig(sConfig_t Config, int address) {
  EEPROM.put(address, Config);
}

// Read Config from EEPROM
sConfig_t ReadConfig(int address) {
  sConfig_t Config;
  return EEPROM.get(address, Config);
}

bool isConfigValid(sConfig_t Config) {
  uint16_t CRCTest = calcCRC16((uint8_t*) &Config, sizeof(Config) - sizeof(Config.CRC16));
  return (CRCTest == Config.CRC16);
}

// Default configuration, executed from Setup(), if necessary
// User defines contact closure state when in Rx mode not keyed
// User defines the step timing per the data sheet for the relay
// delays specifiec in msec after key asserted
sConfig_t InitConfigStruct(sConfig_t Config) {
  Config.Step[0].Polarity = LOW; // open on Rx 
  Config.Step[0].Assert   = 250;  // msec relay assert time
  Config.Step[0].Release  = 200;  // msec relay release time
  Config.Step[1].Polarity = LOW;
  Config.Step[1].Assert   = 251;
  Config.Step[1].Release  = 201;
  Config.Step[2].Polarity = LOW;
  Config.Step[2].Assert   = 252;
  Config.Step[2].Release  = 202;
  Config.Step[3].Polarity = LOW;
  Config.Step[3].Assert   = 253;
  Config.Step[3].Release  = 203;
  Config.Key.Enable       = true; // Hardware key line
  Config.Key.Polarity     = LOW;  // Ground to key
  Config.RTS.Enable       = true; // Serial control signal
  Config.RTS.Polarity     = LOW;  // voltage on MCU pin, (normally high)
  Config.CTS.Enable       = true; // Low mean tx
  Config.CTS.Polarity     = LOW;  // voltage on MCU pin, (normally high)
  Config.Timer.Enable     = true;
  Config.Timer.Time       = 10;   // sec 
  Config.CRC16            = calcCRC16( (uint8_t*) &Config, sizeof(Config) - sizeof(Config.CRC16));
  return Config;
}

// EEPROM functions
// read, write and update state
// verify contents
//   Step form, assert, release
//   Key enable, polarity
//   RTS enable, polarity
//   CTS enable, polarity
//   Timer enable, time
// write config from program constants, set checksum
// update config after user input, set checksum
// read config when needed
// check config checksum
// 
void PrintConfig(sConfig_t Config) {
  Serial.println("Step Form TxTime RxTime");
  for(int ii = 0; ii < 4; ii++) {
    Serial.print("   ");
    Serial.print(ii);
    Serial.print("   ");
    if (Config.Step[ii].Polarity == 1) {
      Serial.print("NO");
    } else {
      Serial.print("NC");
    }
    Serial.print("    ");
    Serial.print(Config.Step[ii].Assert);
    Serial.print("    ");
    Serial.print(Config.Step[ii].Release);
    Serial.println();
  }
  Serial.print(" Key   ");
  if (Config.Key.Enable == true){
    Serial.print("Enabled");
  } else {
    Serial.print("Disabled");
  }
  Serial.print(", Asserted = ");
  if (Config.Key.Polarity == HIGH) {
    Serial.print("High");
  } else {
    Serial.print("Low");
    Serial.println();
  }
  Serial.print(" RTS   ");
  if (Config.RTS.Enable == true){
    Serial.print("Enabled");
  } else {
    Serial.print("Disabled");
  }
  Serial.print(", Asserted = ");
  if (Config.RTS.Polarity == HIGH) {
    Serial.print("High");
  } else {
    Serial.print("Low");
    Serial.println();
  }
  Serial.print(" Timer ");
  if (Config.Timer.Enable == true) {
    Serial.print("Enabled");
  } else {
    Serial.print("Disabled");
  }
  Serial.print(", Time ");
  Serial.print(Config.Timer.Time);
  Serial.println(" sec");
}
