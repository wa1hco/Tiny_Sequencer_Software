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

//  ATTinyX16 Hardware V1-rc1 Pin Definitions
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

// Serial Command Menu
// Code Name and Version
// List current parameters
//   Step timing assert and release
//   Step polarity NO or NC
//   Key Polarity High or Low
//   RTS used for Key  true or false
//   CTS used for Tx ready
//   Timeout on Tx True or False
//   Timeout Value

#include <SerialMenu.hpp>
SerialMenu& menu = SerialMenu::get();

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

// define states and related variables
// Sequencer board design drives high to light the LED in the optoisolator, which causes the contacts to close,
// define sequencer contacts in terms of MCU output pin
#define CLOSED HIGH  // drive MCU pin high to assert
#define OPEN    LOW
// User defines contact closure state when not keyed
// User defines the step timing per the data sheet for the relay
// delays specifiec in msec after key asserted
enum           States { Rx,     S1T,     S2T,     S3T,     S4T,   Tx,     S4R,     S3R,     S2R,     S1R};  // numbered 0 to 9
String StateNames[] = {"Rx",   "S1T",   "S2T",   "S3T",   "S4T", "Tx",   "S4R",   "S3R",   "S2R",   "S1R"}; // for debug messages
int StepPins[]      = {   0, PIN_PC2, PIN_PC1, PIN_PB0, PIN_PA3,    0, PIN_PA3, PIN_PB0, PIN_PC1, PIN_PC2}; // Hardware config
//  SOIC-20 Pin                   14       13       11      19              19       11       13       14   // hardware config info
//  Tiny1616 Breakout         JP4-12   JP4-10   JP2-10   JP4-4           JP4-4   JP2-10   JP4-10   JP4-12   // hardware config info
int StepForms[]     = {   0,  CLOSED,  CLOSED,  CLOSED,  CLOSED,    0,    OPEN,    OPEN,    OPEN,    OPEN}; // user config
int StepTimes[]     = {   0,     300,     300,     300,    300,     0,     200,     200,     200,     200}; // user config

// define input pin polarity
#define KEYASSERTED   LOW  
#define RTSASSERTED   LOW

// define keying characteristics
#define KEYENABLE     true
#define RTSENABLE     true
#define TIMEOUTENABLE true
#define TXTIMER       10000    // msec

#define DEBUGLEVEL  1     // 

// Global state machine variables
static int           StateNext = Rx;
static int           State     = Rx;
static int           StatePrevious;
int                  TimeLoop;
unsigned long        TimeNow;
static unsigned long TimePrevious = millis(); // initialize

void setup() {  
  Serial.begin(19200);

  Serial.println();
  while (!Serial){};
  Serial.println("Sequencer startup");

  menu.load(nullptr, 0);
  menu.show();

  // pin configuration
  pinMode(  KEYPIN, INPUT_PULLUP); 
  pinMode(  RTSPIN, INPUT_PULLUP);
  pinMode(  LEDPIN, OUTPUT);
  pinMode(  CTSPIN, OUTPUT);

  // step pin initialization
  for (int StateIdx = 0; StateIdx < 10; StateIdx++) {    // Loop over all states
    if (StepPins[StateIdx] == 0)  break;                 // skip undefined states
    if (StateNames[StateIdx].indexOf('R') != -1) break;  // skip if not receive transition
    pinMode( StepPins[StateIdx], OUTPUT);                // config step pin
    digitalWrite(StepPins[StateIdx], (uint8_t) StepForms[StateIdx]); // config as receive mode   
  }
  digitalWrite(  LEDPIN, HIGH);
  digitalWrite(  CTSPIN, LOW);
}

// this loop is entered seveal seconds after setup()
void loop() {

  menu.run(1);

  bool Key;        // used by state machine, combined from hardware and timeout
  bool KeyTimeOut; // used by tx timout management in loop()
  static int TxTimer; 

  // time calculation for this pass through the loop
  TimeNow = millis();
  // Calculate time increment for this pass through loop()
  TimeLoop = (unsigned int)(TimeNow - TimePrevious);
  TimePrevious = TimeNow;  // always, either first or later pass

  // two methods of keying 
  bool KeyInput = KEYENABLE & !( (bool) digitalRead(KEYPIN) ^ (bool) KEYASSERTED); // hardware Key interface, high = asserted
  bool RTSInput = RTSENABLE & !( (bool) digitalRead(RTSPIN) ^ (bool) KEYASSERTED); // USB serial key interface, high = asserted

  if (!KeyInput & !RTSInput) {  // if unkeyed, reset the tx timeout timer
    TxTimer = TXTIMER;
    KeyTimeOut = false;
  } else {
    TxTimer -= TimeLoop;
  }
  
  if (TxTimer <= 0) {
    TxTimer = 0;               // keep timer from underflowing
    KeyTimeOut = true;
  }
  Key = (KeyInput || RTSInput) & !(TIMEOUTENABLE & KeyTimeOut);  // either key in or RTs and not timeout

  StateMachine(Key, TimeLoop);

  delay(LOOPTIMEINTERVAL);
}
// Commom state timer and state transition function
// Called with new State if timer times out
// On first call to state, set the timer and assert the step output
// Returns the next state for the state machine if timer still running
// Timer set from table of assert and release times for each step
// State and StatePrevious are global and maintained at high level
int StateTimer(int StateNew) { 
  static int           StepTimer;

  if (StatePrevious != State) {  // initialize timer on state change event
    StepTimer = StepTimes[State];
    StateEntryMsg(StatePrevious, State, StepTimer);
    digitalWrite(StepPins[State], (uint8_t) StepForms[State]);
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
      Serial.println(" ms");
    } // if timer
  } // if debuglevel
} // StateEntryMessage

void StateMachine(bool Key, int TimeLoop) {
  // State Machine
  StatePrevious = State;
  State = StateNext; 
  switch (State) {
    case Rx:
      if (StatePrevious != State) {
        StateEntryMsg(StatePrevious, State, 0); // state debug with no timer
      }

      if (Key) { // Keyed
        Serial.println("State Rx, key asserted");  
        StateNext =  S1T;
      } // keyed
      break;

    // manage step 1 relay during Rx to Tx sequence
    case S1T:
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
