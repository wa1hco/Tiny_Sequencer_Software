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

// State Machine ASCII art, legend () for state name, [] for event name 
//                           
//                 |--->(    Rx      )--->|
//                 |    (tst keysig  )    |
//                 |    (clr key gate)    v
//              [unkey]                 [key]              
//                 ^                      | 
//        |---->\  | /----->[key]----->\  v  /<----|           
//    [delay]    (S1R)                  (S1T)    [delay]
//        |<----/  ^ \<----[unkey]<----/  |  \>----|         
//                 |                      |
//        |---->\  | /----->[key]----->\  v  /<----|           
//    [delay]    (S1R)                  (S1T)    [delay]
//        |<----/  ^ \<----[unkey]<----/  |  \>----|         
//                 |                      |
//        |---->\  | /----->[key]----->\  v  /<----|           
//    [delay]    (S1R)                  (S1T)    [delay]
//        |<----/  ^ \<----[unkey]<----/  |  \>----|         
//                |                       |
//        |---->\ |  /----->[key]----->\  v  /<----|           
//    [delay]    (S1R)                  (S1T)    [delay]
//        |<----/ ^  \<----[unkey]<----/  |  \>----|         
//                |                       v
//             [unkey]                  [key]                     
//                  \                   /
//                   <--(    Tx     )<--
//                      (  Timeout  )
//                      (Set KeyGate)         
//  

// TODO
//   Timeout on Tx held too long
//   CTS asserted when in Tx state

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

#include <EEPROM.h>

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

#define LOOPTIMEINTERVAL 10 // msec

// User Configuration

// Sequencer board design drives high to light the LED in the optoisolator, which causes the contacts to close,
// Optoisolator LED drives gates of back to back MOSFETS, so normally open
// define hardware design of sequencer contacts in terms of MCU output pin
#define CLOSED HIGH  // drive MCU pin high to assert
#define OPEN    LOW

// User defines contact closure state when in Rx mode not keyed
// User defines the step timing per the data sheet for the relay
// delays specifiec in msec after key asserted
enum           States { Rx,     S1T,     S2T,     S3T,     S4T,   Tx,     S4R,     S3R,     S2R,     S1R};  // numbered 0 to 9
String StateNames[] = {"Rx",   "S1T",   "S2T",   "S3T",   "S4T", "Tx",   "S4R",   "S3R",   "S2R",   "S1R"}; // for debug messages
uint8_t StepIndex[] = { 99,       0,       1,       2,       3,   99,       3,       2,       1,       0 }; // step index for each state

// Hardware design defines how the optoisolators connect to MCU, 
// Steps are numbered from 0 to 3
int StepPins[]      = {PIN_PC2, PIN_PC1, PIN_PB0, PIN_PA3}; // Hardware config in software terms
//  SOIC-20 Pin             14       13       11       19   // MCU hardware config board terms
//  Tiny1616 Breakout   JP4-12   JP4-10   JP2-10    JP4-4   // Breakout board config

// Configuration structure used for program and EEPROM
struct sConfig {
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
  int Check;
} Config;

#define DEBUGLEVEL  1     // 

// Global state machine variables
static int           StateNext = Rx;
static int           State     = Rx;
static int           StatePrevious;
uint8_t              StateCnt  = sizeof(States);
int                  TimeLoop;
unsigned long        TimeNow;
static unsigned long TimePrevious = millis(); // initialize

// function prototypes
void StateMachine(bool Key, int TimeLoop);
void printConfig(void);
void printEEPROM(void);
void initConfigStruct(void);

// Commom state timer and state transition function
// Called with new State if timer times out
// On first call to state, set the timer and assert the step output
// Returns the next state for the state machine if timer still running
// Timer set from table of assert and release times for each step
// State and StatePrevious are global and maintained at high level

// called from each state of state machine every time through loop()
// detect is this is the first call after state change
//   initialize the timer based on state
//   run the timer on the current state
// when timer completes, return the new state
int StateTimer(int StateNew) {  // states are 0, 1, 2, 3
  static int StepTimer;
  uint8_t StepIdx = StepIndex[State]; // convert from state number to Step structure index
  if (StatePrevious != State) {  // initialize timer on state change event
    StepTimer = Config.Step[StepIdx].Release;
    StateEntryMsg(StatePrevious, State, StepTimer);
    if ((State >= 1) & (State <=4)) { // States for transition from Rx to Tx 
      digitalWrite(StepPins[StepIdx], (uint8_t) !Config.Step[StepIdx].Polarity);
    }
    if ((State >= 6) & (State <= 9)) {  // States for transition from Tx to Rx
      digitalWrite(StepPins[StepIdx], (uint8_t) Config.Step[StepIdx].Polarity);
    }
    return State;
  } else {  // timer running, check for timeout
    StepTimer -= TimeLoop;
    //Serial.print("State S1T, StepTimer ");
    //Serial.print(StepTimer);
    //Serial.println(" ms");
    if (StepTimer <= 0) {
      return StateNew;
    } else {
      return State;
    } // if timeout
  } // if/else statechange
} // StateStepTimer()

// common message function for all states
void StateEntryMsg(int StatePrevious, int State, int StepTimer) {
  uint8_t StepIdx = StepIndex[State];
  if (DEBUGLEVEL > 0) {
    Serial.print("Enter State ");
    Serial.print(StateNames[State]);
    Serial.print(" from ");
    Serial.print(StateNames[StatePrevious]);
    if (StepTimer == 0) {  // skip timer if not specified
      Serial.println();
    } else {
      Serial.print(", timer ");
      Serial.print(StepTimer);
      Serial.print(" ms, Pin ");
      Serial.print(StepPins[StepIdx]);
      Serial.print(", Polarity ");
      if ((State >= 1) & (State <= 4)) {
        Serial.print(!Config.Step[StepIdx].Polarity); 
      }
      if ((State >= 6) & (State <= 9)) {
        Serial.print(Config.Step[StepIdx].Polarity);
      }
      Serial.println();
    } // if timer
  } // if debuglevel
} // StateEntryMessage

void setup() {  
  Serial.begin(19200);
  delay(1000);
  Serial.println();
  Serial.println("Sequencer startup");
  initConfigStruct();
  printConfig();

  // pin configuration
  pinMode(  KEYPIN, INPUT_PULLUP); 
  pinMode(  RTSPIN, INPUT_PULLUP);
  pinMode(  LEDPIN, OUTPUT);
  pinMode(  CTSPIN, OUTPUT);

  // step pin initialization
  for (int StepIdx = 0; StepIdx < 4; StepIdx++) {    // Loop over sequencer Steps
    pinMode(     StepPins[StepIdx], OUTPUT);                // config step pin
    digitalWrite(StepPins[StepIdx], (uint8_t) Config.Step[StepIdx].Polarity); // config as receive mode   
  }  
  digitalWrite(  LEDPIN, HIGH);
  digitalWrite(  CTSPIN, LOW);
}

// this loop is entered seveal seconds after setup()
// Tx timout is managed at the loop() level, outside of state machine
void loop() {
  bool Key;        // used by state machine, combined from hardware and timeout
  bool KeyTimeOut; // used by Tx timout management in loop()
  static long TxTimer_msec; 

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
  Key = (KeyInput || RTSInput) & !(Config.Timer.Enable & KeyTimeOut);  // either key in or RTs and not timeout

  StateMachine(Key, TimeLoop);

  delay(LOOPTIMEINTERVAL);
}

// Default configuration, executed from Setup(), if necessary
void initConfigStruct(void) {
  Config.Step[0].Polarity = OPEN; // open on Rx 
  Config.Step[0].Assert   = 250;  // msec relay assert time
  Config.Step[0].Release  = 200;  // msec relay release time
  Config.Step[1].Polarity = OPEN;
  Config.Step[1].Assert   = 251;
  Config.Step[1].Release  = 201;
  Config.Step[2].Polarity = OPEN;
  Config.Step[2].Assert   = 252;
  Config.Step[2].Release  = 202;
  Config.Step[3].Polarity = OPEN;
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
  Config.Check            = 0;    // CRC-16
}

// States are number 0 to 9 by an enum function
// State 0, Rx
// States 1:4, transition from Rx to Tx
// State 5, Tx
// State 9:6, transition from Tx to Rx
void StateMachine(bool Key, int TimeLoop) {
  // State Machine
  StatePrevious = State;
  State = StateNext; 
  switch (State) {
    case Rx:
      if (StatePrevious != State) {  // first time looping through Rx state
        StateEntryMsg(StatePrevious, State, 0); // state debug with no timer
      }
      // watch for Key asserted, and transition to first Tx state
      if (Key) { // Keyed
        Serial.println("State Rx, key asserted");  
        StateNext =  S1T;
      } // keyed
      break;

    // manage step 1 relay during Rx to Tx sequence
    case S1T:
      // if unkeyed, transition to corresponding Rx transition state
      if (!Key) {
        StateNext = S1R;
        break;
      }
      StateNext = StateTimer(S2T); // next state either current or parameter
      break;

    // manage step 2 relay during Rx to Tx sequence
    case S2T:
      if (!Key) {
        StateNext = S2R;
        break;
      }
      StateNext = StateTimer(S3T);
      break;

    // manage step 3 relay during Rx to Tx sequence
    case S3T:
      if (!Key) {
        StateNext =  S3R;
        break;
      }
      StateNext = StateTimer(S4T);
      break;

    // manage step 4 relay during Rx to Tx sequence
    case S4T:
      if (!Key) {
        StateNext =  S4R;
        break;
      }
      StateNext =  StateTimer(Tx);
      break;

    case Tx:
      if (StatePrevious != State) {
        StateEntryMsg(StatePrevious, State, 0);
        // TODO, look at Config.CTS.Enable
        digitalWrite(CTSPIN, HIGH);
      }

      if (!Key) {
        StateNext =  S4R;
        digitalWrite(CTSPIN, LOW);
      }
      break;

    // manage step 4 relay during Tx to Rx sequence
    case S4R: // step 4 release timing
      if (Key) {
        StateNext =  S4T;
        break;
      }
      StateNext = StateTimer(S3R);
      break;

    // manage step 3 relay during Tx to Rx sequence
    case S3R:
      if (Key) {
        StateNext =  S3T;
        break;
      }
      StateNext = StateTimer(S2R);
      break;

    // manage step 2 relay during Tx to Rx sequence
    case S2R:
      if (Key) {
        StateNext =  S2T;
        break;
      }
      StateNext = StateTimer(S1R);
      break;

    // manage step 1 relay during Tx to Rx sequence
    case S1R:
      if (Key) {
        StateNext =  S1T;
        break;
      }
      StateNext = StateTimer(Rx);
      break;

    default:
      Serial.println("State: default, Error");
      break;
  }
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
void printConfig(void) {
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
