// Tiny Sequencer
// Licensed under GPL V3, copyright 2024 by jeff millar, wa1hco

// on Key asserted, transition thru states to Tx, assuming key remain asserted
//   state S1T, assert relay 1, wait relay 1 assert time; usually antenna transfer relay
//   state S2T, assert relay 2, wait relay 2 assert time; usually transverter relays
//   state S3T, assert relay 3, wait relay 3 assert time; usually (something)
//   state S4T, assert relay 4, wait relay 4 assert time; transmitter
//   state Tx, transmit ready, assert CTS
//   
// on key released, transition thru states to Rx, assuming key remains released
//   State S4R, release relay 4, release CTS, wait relay 4 release time
//   State S3R, release relay 3, wait relay 3 release time
//   State S2R, release relay 2, wait relay 2 release time
//   State S1R, release relay 1, wait relay 1 release time 
//   receive ready

// on key released during transition to transmit
//   release relay immediately
//   wait relay release time
//   transition to next release step

// on key asserted during transition to receive
//   assert relay immediately
//   wait relay asert time
//   enter transition to next transmit step

// state machine has 
//   Rx state holds as long as key released
//   Tx state holds as long as key asserted
//   4 Tx timed states for relays asserted
//   4 Rx timed states for relays released
//   Tx states exit to corresponding Rx state if key released
//   Rx states ex it to corresponding Tx State if key asserted
//   Timed states set timers on transition into that state
//   Timer values set based on relay close and open times, may be different
//   Timeout causes transition to next state in Rx to Tx or Rx to Tx sequence

// Keying
//   Key inputs are external wire, RTS from USB serial port
//   Key inputs configured as asserted HICH or LOW
//   Key Gate is used for timout, if asserted as if unkeyed
//   RX state will reset key gate if key signal and RTS both released

// Timeout
//   If external key signal asserted Tx until timeout
//   Assert a key gate, triggers transition to Rx
//   Rx waits for Key and RTS signals to release, clears key gate

// TODO
//   Timeout on Tx held too long
//   CTS asserted when in Tx state

#include "HardwareConfig.h"
#include "SoftwareConfig.h"
#include "SequencerStateMachine.h"
#include "UserInterface.h"
#include "Global.h"

void setup() {  
  Serial.begin(19200);
  Serial.println();
  Serial.println("Sequencer startup");

  // Check Configure and init if necessary
  // Read EEPROM
  // TODO add function to wear level the eeprom
  sConfig_t Config = ReadConfig( 0 );  // address 0
  if (isConfigValid(Config)) {
    Serial.println("setup: Config ok, CRC match");
  } else {
    Serial.print("setup: Config invalid, CRC mismatch ");
    Serial.print(" write defaults to config");
    Serial.println();

    Config = InitConfigStruct(Config); // write default values to Config structure
    WriteConfig(Config, 0);  //save Config structure to EEPROM address 0
  } // if CRC match

  // Define the pins, configure i/o
  InitPins();
  
  // Define the form (NO/NC) of step relays wired into the board
  digitalWrite(S1T_PIN, (uint8_t) Config.Step[0].Polarity); // config as receive mode 
  digitalWrite(S2T_PIN, (uint8_t) Config.Step[1].Polarity); // config as receive mode 
  digitalWrite(S3T_PIN, (uint8_t) Config.Step[2].Polarity); // config as receive mode 
  digitalWrite(S4T_PIN, (uint8_t) Config.Step[3].Polarity); // config as receive mode 


} // setup()

// this loop is entered seveal seconds after setup()
// Tx timout is managed at the loop() level, outside of state machine
void loop() {
  bool Key;        // used by state machine, combined from hardware and timeout
  bool KeyTimeOut; // used by Tx timout management in loop()
  static long TxTimer_msec; 
  static bool FirstPass = true;
  static sConfig_t Config;

  int                  TimeLoop;
  unsigned long        TimeNow;
  static unsigned long TimePrevious = millis(); // initialize

  // First time through the loop read the configuration data structure from EEPROM
  if (FirstPass) {   
    Config = ReadConfig(0);
    FirstPass = false;
  }
 
  // time calculation for this pass through the loop
  TimeNow = millis();
  // Calculate time increment for this pass through loop()
  TimeLoop = (unsigned int)(TimeNow - TimePrevious);
  TimePrevious = TimeNow;  // always, either first or later pass

  // two methods of keying, logic converts input to positive logic 
  bool KeyInput = Config.Key.Enable & !( (bool) digitalRead(KEYPIN) ^ (bool) Config.Key.Polarity); // hardware Key interface, high = asserted
  bool RTSInput = Config.RTS.Enable & !( (bool) digitalRead(RTSPIN) ^ (bool) Config.RTS.Polarity); // USB serial key interface, high = asserted

  if (!KeyInput & !RTSInput) {  // if unkeyed, reset the tx timeout timer
    TxTimer_msec = (long) Config.Timer.Time * 1000; //sec to msec
    KeyTimeOut = false;
  } else {
    TxTimer_msec -= (long) TimeLoop;
  }
  
  // if Tx timer timeout, set KeyTimeOut flag
  if (TxTimer_msec <= 0) {
    TxTimer_msec = 0;               // keep timer from underflowing
    KeyTimeOut = true;
  }
  Key = (KeyInput || RTSInput) & !(Config.Timer.Enable & KeyTimeOut);  // key in OR RTS AND NOT timeout

  StateMachine(Config, Key, TimeLoop);
  Config = UserConfig(Config);

  #define LOOPTIMEINTERVAL 10 // msec
  delay(LOOPTIMEINTERVAL);
}

