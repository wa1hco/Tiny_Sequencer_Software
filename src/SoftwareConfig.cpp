
#include <Arduino.h>
#include <EEPROM.h>
#include <CRC16.h>
#include <CRC.h>

#include "Config.h"
#include "SoftwareConfig.h"
#include "HardwareConfig.h"

// Config structure functions
// read, write, put, verify, print

// Read Config from EEPROM
sConfig_t GetConfig(int address) {
  char Msg[80];

  sConfig_t Config;
  EEPROM.get(address, Config);

  snprintf(Msg, 80, "GetConfig: EEConf 0x%x, size %d, CRC 0x%x", &Config, sizeof(Config), Config.CRC16);
  Serial.println(Msg);

  return Config;
}

// Update Config in EEPROM
// compare new config with eeprom and update bytes as necessary
void PutConfig(int address, sConfig_t NewConf) {
  char Msg[80];
  snprintf(Msg, 80, "PutConfig: NewConf 0x%x, size %d, CRC 0x%x", &NewConf, sizeof(NewConf), NewConf.CRC16);
  Serial.println(Msg);

  sConfig_t EEConf = GetConfig(address);
  unsigned long EESize  = sizeof(EEConf);
  unsigned long NewSize = sizeof(NewConf);
  snprintf(Msg, 80, "PutConfig: EESize %d, NewSize %d", EESize, NewSize);
  if (EESize != NewSize) {
    char Msg[80];
    snprintf(Msg, 80, "PutConfig: Size mismatch EEConf 0x%x vs * NewConf 0x%x", EESize, NewSize );
    Serial.println(Msg);
    return;
  }
  // update the CRC
  NewConf.CRC16 = crc16((uint8_t *) &NewConf, sizeof(NewConf) - sizeof(NewConf.CRC16));
  EEPROM.put(0, NewConf);

  bool isChanged = false;
  for (uint8_t ii = 0; ii < EESize; ii++){
    if (((uint8_t *) &NewConf)[ii] != ((uint8_t *) &EEConf)[ii]) {
      char Msg[80];
      snprintf(Msg, 80, "PutConfig: bytes differ idx %d, new %d, prom %d",  ii, ((uint8_t *) &NewConf)[ii], ((uint8_t *) &EEConf)[ii]);
      Serial.println(Msg);
      isChanged = true;
    }
  }
  return;
}

bool isConfigValid(sConfig_t Config) {
  uint16_t CRCTest = calcCRC16((uint8_t*) &Config, sizeof(Config) - sizeof(Config.CRC16));

  char Msg[80];
  snprintf(Msg, 80, "IsConfigValid: Config 0x%x, size %d, CRC 0x%x", &Config, sizeof(Config), Config.CRC16);
  Serial.println(Msg);

  return (CRCTest == Config.CRC16);
}

// Default configuration, executed from Setup(), if necessary
// User defines contact closure state when in Rx mode not keyed
// User defines the step timing per the data sheet for the relay
// delays specifiec in msec after key asserted
sConfig_t InitDefaultConfig() {
  sConfig_t Config;
  Config.Step[0].TxPolarity = TX_CLOSED;    // closed on Tx, open on Rx, closed by driving pin high
  Config.Step[0].Tx_msec    = 50;           // msec relay assert time
  Config.Step[0].Rx_msec    = 50;           // msec relay release time
  Config.Step[1].TxPolarity = TX_CLOSED;
  Config.Step[1].Tx_msec    = 50;
  Config.Step[1].Rx_msec    = 50;
  Config.Step[2].TxPolarity = TX_CLOSED;
  Config.Step[2].Tx_msec    = 50;
  Config.Step[2].Rx_msec    = 50;
  Config.Step[3].TxPolarity = TX_CLOSED;
  Config.Step[3].Tx_msec    = 50;
  Config.Step[3].Rx_msec    = 50;
  Config.RTS.Enable         = true;         // RTS UP to key Tx
  Config.CTS.Enable         = true;         // CTS UP on ready to modulate
  Config.Timer.Time         = 120;          // sec, 0 means disabled
  Config.CRC16              = calcCRC16((uint8_t *) &Config, sizeof(Config) - sizeof(Config.CRC16));

  char Msg[80];
  snprintf(Msg, 80, "InitDefaultConfig: Config 0x%x, size %d, CRC 0x%x", &Config, sizeof(Config), Config.CRC16);
  Serial.println(Msg);
  return Config;
}

// pretty print the memory configuration on serial port
void PrintConfig(sConfig_t Config) {
  char Msg[80];
  snprintf(Msg, 80, "PrintConfig: Config 0x%x", (int) &Config);
  Serial.println(Msg);
  for(int ii = 0; ii < 4; ii++) {
    char Polarity_str[15] = "              ";
    if (Config.Step[ii].TxPolarity == TX_CLOSED) {
      strcpy(Polarity_str, "Closed on TX");
    } else {
      strcpy(Polarity_str, "Open   on TX");
    }
    char Msg[80];
    snprintf(Msg, 80, "Step %d, Contacts %s, TX delay %d, RX delay %d", ii, Polarity_str, Config.Step[ii].Tx_msec, Config.Step[ii].Rx_msec);
    Serial.println(Msg);
  }

  Serial.print(" RTS   ");
  if (Config.RTS.Enable == true){
    Serial.print("Enabled, ");
  } else {
    Serial.print("Disabled, ");
  }
  Serial.println();

  Serial.print(" CTS   ");
  if (Config.CTS.Enable == true){
    Serial.print("Enabled");
  } else {
    Serial.print("Disabled");
  }
  Serial.println();

  Serial.print(" Tx Timer ");
  Serial.print(Config.Timer.Time);
  Serial.println(" sec");
} // PrintConfig()
