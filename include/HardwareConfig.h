#ifndef HardwareConfig_h
#define HardwareConfig_h

#include "Config.h"
#include <Arduino.h> // get the pin names

//  ATTinyX16, QFN-20, Hardware Pin Definitions, lower case means secondary pin location
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
// Pin      |1  |2  |3  |4  |5  |6  |7  |8  |9  |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |20 |
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
// Port     |PA2|PA3|GND|VCC|PA4|PA5|PA6|PA7|PB5|PB4|PB3|PB2|PB1|PB0|PC0|PC1|PC2|PC3|PA0|PA1|
// Power    |   |   |GND|VCC|   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// TCA0     |   |   |   |   |WO4|WO5|   |   |wo2|wo1|wo0|WO2|WO1|WO0|   |   |   |wo3|WO3|   |
// UPDI     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |PDI|   |
// ADC0     |A2 |A3 |A2 |   |A4 |A5 |A6 |A7 |A8 |A9 |   |   |A10|A11|   |A17|A14|A15|A0 |A1 |
// ADC1     |   |   |   |   |A0 |A1 |A2 |A3 |   |   |   |   |   |   |A6 |A7 |A8 |A9 |   |   |
// TWI0     |scl|   |   |   |   |   |   |   |   |   |   |   |SDA|SCL|   |   |   |   |   |sda|
// USART0   |rxd|xck|   |   |   |   |   |   |   |   |RXD|TXD|XCK|DIR|   |   |   |   |   |txd|
// CLOCK    |   |EXT|   |   |   |   |   |   |   |   |OSC|OSC|   |   |   |   |   |   |   |   |
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

// Sequencer board use of pins
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
// Pins     |1  |2  |3  |4  |5  |6  |7  |8  |9  |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |20 |  
// Port     |PA2|PA3|GND|VCC|PA4|PA5|PA6|PA7|PB5|PB4|PB3|PB2|PB1|PB0|PC0|PC1|PC2|PC3|PA0|PA1|
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|  
// SeqOut   |   |   |   |   |   |   |   |   |   |   |   |   |   |S4 |S3 |S2 |S1 |   |   |   |
// Serial   |CTS|   |   |   |   |   |   |   |   |   |RXD|TXD|   |   |   |   |   |   |   |RTS|
// Key      |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |KEY|   |   |
// LED      |   |LED|   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// UPDI     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |PDI|   |
// EXTRA    |   |   |   |   |X1 |X2 |X3 |X4 |X5 |X6 |   |   |   |   |   |   |   |   |   |   |
//--------- |---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
// Unused   |   |   |   |   |   |   |   |   |   |   |   |   |13 |   |   |   |   |   |   |   |
//--------- |---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|

// Hardware Connections for ATTinyX16
//      Pin Name  Port Name    SOIC-20,   Adafruit Tiny1616 breakout
#define KEYPIN    PIN_PC3   // MCU 15,    JP4 8
#define RTSPIN    PIN_PA1   // MCU 17,    JP4 6
#define CTSPIN    PIN_PA2   // MCU 18,    JP4 5
#define LEDPIN    PIN_PA3   // MCU 12,    JP2 10
#define XTRA1PIN  PIN_PA4   // MCU  2,    JP2 1
#define XTRA2PIN  PIN_PA5   // MCU  3,    JP2 2
#define XTRA3PIN  PIN_PA6   // MCU  4,    JP2 3
#define XTRA4PIN  PIN_PA7   // MCU  5,    JP2 4
#define XTRA5PIN  PIN_PB5   // MCU  6,    JP2 5
#define XTRA6PIN  PIN_PB4   // MCU  7,    JP2 6

// Key input hardware connection
// Optoisolator has LED that drives NPN transistor 
// Key input MCU pin is pulled up internally, NPN pulls it down
// Usual configuration, Key input high in Rx and pull to gnd for Tx
// Hardware has current limiting and overvoltage protection
// 5 to 15 V on KEY+, open or ground on KEY-
// Other wiring may warrant a change in this
// define the LED diode state to MCU KEYPIN state
#define KEY_OPTO_ON  LOW   // state of MCU pin in Tx
#define KEY_OPTO_OFF HIGH  // state of MCU pin in RX

// RTS input hardware
// Serial port has RTS\ output pin (low means RTS UP)
// RTS can be UP or DOWN
#define KEY_RTS_UP   LOW
#define KEY_RTS_DOWN HIGH

// CTS output hardware
// Serial port has CTS\ output pin (low means CTS UP)
#define CTS_UP    LOW
#define CTS_DOWN HIGH

// Sequencer board design drives high to light the LED in the optoisolator, which causes the contacts to close,
// Optoisolator LED drives gates of back to back MOSFETS, so normally open
#define CLOSED HIGH  // On TX, drive MCU pin high to close output contacts
#define OPEN   LOW   // On TX, drvie MCU pin low to open output contacts

// Hardware design 
// defines how the optoisolators connect to MCU, 
// Steps are numbered from 0 to 3

// Map statepin names to state according to board wiring
// On RX to TX sequence, S1T, S2T, S3T, S4T
// on TX to RX sequence, S4T, S3T, S2T, S1T
#define RX_PIN  0
#define S1T_PIN PIN_PC2
#define S2T_PIN PIN_PC1
#define S3T_PIN PIN_PC0
#define S4T_PIN PIN_PB0
#define S1R_PIN S1T_PIN
#define S2R_PIN S2T_PIN
#define S3R_PIN S3T_PIN
#define S4R_PIN S4T_PIN
#define TX_PIN  0

// Public function
void InitPins();

#endif