#ifndef Config_h
#define Config_h

#include <Arduino.h>

// Configuration structure used for program and EEPROM
struct sConfig_t {
  struct sStep {            // Array of sequence step configs
    uint8_t    RxPolarity;  // Rx state, "normal" state Open or Closed
    uint8_t    Tx_msec;     // msec
    uint8_t    Rx_msec;     // msec
  } Step[4];                // sequencer has 3 steps
  bool         RTSEnable;   // true enabled, false disabled
  bool         CTSEnable;   // true enabled, false disabled
  unsigned int Timeout;     // sec, Tx timeout timer0 means disabled
  uint16_t     CRC16;       // check for valid configuration table
}; 
#endif