#ifndef Config_h
#define Config_h

#include <Arduino.h>

// Configuration structure used for program and EEPROM
struct sConfig_t {
  struct sStep {      // Array of sequence step configs
    uint8_t Polarity; // Rx state, Open (0) or Closed (1)
    uint8_t Assert;   // msec
    uint8_t Release;  // msec
  } Step[4]; 
  struct sKey {        // Key hardware input able to key Tx
    uint8_t Enable;    // 0 disabled, 1 enabled
    uint8_t Polarity;  // 0 active low, 1 active high
  } Key;
  struct sRTS {        // RTS from serail port able to key Tx  
    uint8_t Enable;    // 0 disabled, 1 enabled
    uint8_t Polarity;  // 0 active low, 1 active high
  } RTS;
  struct sCTS {        // CTS shows Tx ready
    uint8_t Enable;    // 0 disabled, 1 enabled
    uint8_t Polarity;  // 0 active low, 1 active high
  } CTS;
  struct sTimer {      // Tx timeout timer
    uint8_t Enable;    // 0 disabled, 1 enabled
    unsigned int Time; // sec
  } Timer;
  uint16_t CRC16;
};

#endif