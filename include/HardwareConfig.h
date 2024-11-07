#ifndef HardwareConfig_h
#define HardwareConfig_h

#include <Arduino.h> // get the pin names

//  ATTinyX16, SOIC-20, Hardware Pin Definitions
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
// Pin      |1  |2  |3  |4  |5  |6  |7  |8  |9  |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |20 |
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
// Port     |   |PA4|PA5|PA6|PA7|PB5|PB4|PB3|PB2|PB1|PB0|PC0|PC1|PC2|PC3|PA0|PA1|PA2|PA3|   |
// Power    |PWR|   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |GND|
// PWM      |   |   |   |   |   |   |   |8  |9  |   |   |12 |13 |14 |15 |   |   |   |   |   |
// UPDI     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |PDI|   |   |   |   |
// Analog   |A0~|A1~|A2 |A3 |A4 |A5 |   |   |   |A8~|A9~|A2 |A3 |   |   |A17|A14|A15|A16|   |
// I2C      |   |   |   |   |   |   |   |   |   |SDA|SCL|   |   |   |   |   |   |   |   |   |
// Serial1  |   |   |   |   |   |   |   |RXD|TXD|   |   |   |   |   |   |   |   |   |   |   |
// DAC      |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// CLOCK    |   |   |   |   |   |   |   |OSC|OSC|   |   |   |   |   |   |   |   |   |EXT|   |
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

// Sequencer board use of pins
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
// Pins     |1  |2  |3  |4  |5  |6  |7  |8  |9  |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |20 |  
// Port     |   |PA4|PA5|PA6|PA7|PB5|PB4|PB3|PB2|PB1|PB0|PC0|PC1|PC2|PC3|PA0|PA1|PA2|PA3|   |
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|  
// SeqOut   |   |   |   |   |   |   |   |   |   |   |S3 |   |S2 |S1 |   |   |   |   |S4 |   |
// Serial   |   |   |   |   |   |   |   |RXD|TXD|   |   |   |   |   |   |   |RTS|CTS|   |   |
// Key      |   |   |   |   |   |   |   |   |   |   |   |   |   |   |KEY|   |   |   |   |   |
// LED      |   |   |   |   |   |   |   |   |   |   |   |LED|   |   |   |   |   |   |   |   |
// UPDI     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |PDI|   |   |   |   |
// EXTRA    |   |1  |2  |3  |4  |5  |6  |   |   |   |   |   |   |   |   |   |   |   |   |   |
//--------- |---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
// Unused   |   |   |   |   |   |   |   |   |   |10 |   |   |   |   |   |   |   |   |   |   |
//--------- |---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

// Hardware Connections for ATTinyX16
//      Pin Name  Port Name    SOIC-20,   Adafruit Tiny1616 breakout
#define KEYPIN    PIN_PC3   // MCU 15,    JP4 8
#define RTSPIN    PIN_PA1   // MCU 17,    JP4 6
#define CTSPIN    PIN_PA2   // MCU 18,    JP4 5
#define LEDPIN    PIN_PC0   // MCU 12,    JP2 10
#define XTRA1PIN  PIN_PA4   // MCU  2,    JP2 1
#define XTRA2PIN  PIN_PA5   // MCU  3,    JP2 2
#define XTRA3PIN  PIN_PA6   // MCU  4,    JP2 3
#define XTRA4PIN  PIN_PA7   // MCU  5,    JP2 4
#define XTRA5PIN  PIN_PB5   // MCU  6,    JP2 5
#define XTRA6PIN  PIN_PB4   // MCU  7,    JP2 6

// Hardware Configuration, how the hardware connects to software
// Sequencer board design drives high to light the LED in the optoisolator, which causes the contacts to close,
// Optoisolator LED drives gates of back to back MOSFETS, so normally open
#define CLOSED HIGH  // drive MCU pin high to close output contacts on optoisolator
#define OPEN    LOW

// Hardware design 
// defines how the optoisolators connect to MCU, 
// Steps are numbered from 0 to 3

// Map statepin names to state according to board wiring
#define RX_PIN  0
#define S1T_PIN PIN_PC2
#define S2T_PIN PIN_PC1
#define S3T_PIN PIN_PB0
#define S4T_PIN PIN_PA3
#define S1R_PIN S1T_PIN
#define S2R_PIN S2T_PIN
#define S3R_PIN S3T_PIN
#define S4R_PIN S4T_PIN
#define TX_PIN  0

// Public function
void InitPins(void);

#endif