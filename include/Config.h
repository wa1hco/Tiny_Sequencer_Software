#ifndef Config_h
#define Config_h

#include <Arduino.h>

// Configuration structure used for program and EEPROM
struct sConfig_t {
  struct sStep {        // Array of sequence step configs
    uint8_t TxPolarity; // Tx state, Open or Closed
    uint8_t Tx_msec;    // msec
    uint8_t Rx_msec;    // msec
  } Step[4]; 
  struct sRTS {         // RTS from serail port able to key Tx  
    uint8_t Enable;     // 0 disabled, 1 enabled
  } RTS;
  struct sCTS {         // CTS shows Tx ready
    uint8_t Enable;     // 0 disabled, 1 enabled
  } CTS;
  struct sTimer {       // Tx timeout timer
    unsigned int Time;  // sec, 0 means disabled
  } Timer;
  uint16_t CRC16;
};

#endif