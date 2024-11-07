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

#include "HardwareConfig.h"
#include "SoftwareConfig.h"
#include "Config.h"
#include "SequencerStateMachine.h"
#include "UserInterface.h"
#include "Global.h"

sConfig_t Config;

//  Setup ATtiny_TimerInterrupt library
#if !( defined(MEGATINYCORE) )
  #error The interrupt is designed only for MEGATINYCORE megaAVR board! Please check your Tools->Board setting
#endif
// These define's must be placed at the beginning before #include "megaAVR_TimerInterrupt.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0

// Select USING_FULL_CLOCK      == true for  20/16MHz to Timer TCBx => shorter timer, but better accuracy
// Select USING_HALF_CLOCK      == true for  10/ 8MHz to Timer TCBx => shorter timer, but better accuracy
// Select USING_250KHZ          == true for 250KHz to Timer TCBx => longer timer,  but worse  accuracy
// Not select for default 250KHz to Timer TCBx => longer timer,  but worse accuracy
#define USING_FULL_CLOCK      true
#define USING_HALF_CLOCK      false
#define USING_250KHZ          false         // Not supported now

// Try to use RTC, TCA0 or TCD0 for millis()
#define USE_TIMER_0           true          // Check if used by millis(), Servo or tone()
#define USE_TIMER_1           false         // Check if used by millis(), Servo or tone()

#if USE_TIMER_0
  #define CurrentTimer   ITimer0
#elif USE_TIMER_1
  #define CurrentTimer   ITimer1
#else
  #error You must select one Timer  
#endif

// To be included only in main(), .ino with setup() to avoid `Multiple Definitions` Linker Error
#include "ATtiny_TimerInterrupt.h"

#define TIMER1_INTERVAL_MS    10
#define TIMER1_FREQUENCY      (float) (1000.0f / TIMER1_INTERVAL_MS)

#define ADJUST_FACTOR         ( (float) 0.99850 )

// Called from an timer interrupt
void SequencerISR() {
  static long          TxTimer_msec; 
  int                  TimeIncrement;
  unsigned long        TimeNow;
  static unsigned long TimePrevious = millis(); // initialize
  bool Key;        // used by state machine, combined from hardware and timeout
  bool KeyTimeOut; // used by Tx timout management in loop()

  TimeNow = millis();
  TimeIncrement = (unsigned int)(TimeNow - TimePrevious);
  TimePrevious = TimeNow;

  // two methods of keying, logic converts input to positive logic 
  bool KeyInput = Config.Key.Enable & !( (bool) digitalRead(KEYPIN) ^ (bool) Config.Key.Polarity); // hardware Key interface, high = asserted
  bool RTSInput = Config.RTS.Enable & !( (bool) digitalRead(RTSPIN) ^ (bool) Config.RTS.Polarity); // USB serial key interface, high = asserted

  if (!KeyInput & !RTSInput) {  // if unkeyed, reset the tx timeout timer
    TxTimer_msec = (long) Config.Timer.Time * 1000; //sec to msec
    KeyTimeOut = false;
  } else {
    TxTimer_msec -= (long) TimeIncrement;
  }
  
  // if Tx timer timeout, set KeyTimeOut flag
  if (TxTimer_msec <= 0) {
    TxTimer_msec = 0;               // keep timer from underflowing
    KeyTimeOut = true;
  }
  Key = (KeyInput || RTSInput) & !(Config.Timer.Enable & KeyTimeOut);  // key in OR RTS AND NOT timeout

  StateMachine(Config, Key, TimeIncrement);
}

void setup() {  
  Serial.begin(19200);
  Serial.println();
  Serial.println("Sequencer startup");

  // Check Configure and init if necessary
  // Read EEPROM
  // TODO add function to wear level the eeprom
  Config = ReadConfig( 0 );  // address 0
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

  CurrentTimer.init();
  if (CurrentTimer.attachInterruptInterval(TIMER1_INTERVAL_MS * ADJUST_FACTOR, SequencerISR))
  {
    Serial.print(F("Starting ITimer OK, millis() = ")); Serial.println(millis());
  }
  else {
    Serial.println(F("Can't set ITimer. Select another freq. or timer"));
  }
  
} // setup()

// this loop is entered seveal seconds after setup()
// Tx timout is managed at the loop() level, outside of state machine
void loop() {
  static bool FirstPass = true;

  // First time through the loop read the configuration data structure from EEPROM
  // setup() should have ensured a valid configuration or reinitialized
  if (FirstPass) {   
    Config = ReadConfig(0);
    FirstPass = false;
  }
 
  Config = UserConfig(Config);

  #define LOOPTIMEINTERVAL 30 // msec
  delay(LOOPTIMEINTERVAL);
}

