// State Machine ASCII art, legend () for state name, [] for event name 
//
//                 |--->(    Rx      )--->|
//                 |    (test key, RST)    |
//                 |    (clr TxTimeout)    v
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

#include "SequencerStateMachine.h"
#include "HardwareConfig.h"         // pick up pin names
#include "Global.h"

// Private to StateMachine functions
// State Machine definitions
enum         State_t  { Rx,     S1T,     S2T,     S3T,     S4T,   Tx,     S4R,     S3R,     S2R,     S1R }; // numbered 0 to 9
String  StateName[] = {"Rx",   "S1T",   "S2T",   "S3T",   "S4T", "Tx",   "S4R",   "S3R",   "S2R",   "S1R"}; // for debug messages
uint8_t StepIndex[] = { 99,       0,       1,       2,       3,   99,       3,       2,       1,       0 }; // map states to step index

// private functions
void newStateMsg(sConfig_t Config, State_t prevState, State_t State, int StepTime, pin_size_t StepPin);
State_t StateTimer (sConfig_t Config, State_t prevState, State_t State, State_t nextState, int TimeLoop, int StepPin);

// Commom state timer and state transition function
// Called from each state of state machine every time through loop()
// On first call to state, set the timer and assert the step output
// Returns State for next pass, either same State or StateNew
// Leave State unchanged if timer still running
// nextState returned when time completes 
// Timer set from table of assert and release times for each step

// detect is this is the first call after state change
//   initialize the timer based on state
//   run the timer on the current state
// when timer completes, return the new state
// Parameters
//  Config: polarity of step pins
//  prevState, State: detect when a new state
//  nextState: for when timer timesout
//  TimeLoop: time between calls to statemachine, used to decrement timer
//  StepPin: pin controlled by next state
State_t StateTimer (sConfig_t Config, State_t prevState, State_t State, State_t nextState, int TimeLoop, int StepPin) {
  static int StepTime;
  if (prevState != State) {                           // State change event
    if ((State >= 1) & (State <=4)) {                 // States for transition from Rx to Tx 
    // write pin associated with state, Rx and Tx do not get written
      digitalWrite(StepPin, (uint8_t) !Config.Step[State - 1].Polarity);
      StepTime = Config.Step[State - 1].Release;          // initialize the timer
    }
    if ((State >= 6) & (State <= 9)) {                // States for transition from Tx to Rx
      digitalWrite(StepPin, (uint8_t) Config.Step[State - 6].Polarity);
      StepTime = Config.Step[State - 6].Release;          // initialize the timer
    }
    newStateMsg(Config, prevState, State, StepTime, StepPin);  // debug message
    return State;
  } else {                                            // timer running, check for timeout
    StepTime -= TimeLoop;
    //Serial.print("State S1T, StepTime ");
    //Serial.print(StepTime);
    //Serial.println(" ms");
    if (StepTime <= 0) {
      return nextState;
    } else {
      return State;
    } // if timeout
  } // if/else statechange
} // StateTimer()

// common message function for all states
void newStateMsg(sConfig_t Config, State_t prevState, State_t State, int StepTime, pin_size_t StepPin) {
  if (DEBUGLEVEL > 0) {
    Serial.print("Enter State ");
    Serial.print(StateName[State]);
    Serial.print(" from ");
    Serial.print(StateName[prevState]);
    if (StepTime == 0) {  // skip timer if not specified
      Serial.println();
    } else {
      Serial.print(", timer ");
      Serial.print(StepTime);
      Serial.print(" ms, Pin ");
      Serial.print(StepPin);
      Serial.print(", Polarity ");
      if ((State >= 1) & (State <= 4)) {  // Rx state
        Serial.print(!Config.Step[State - 1].Polarity); 
      }
      if ((State >= 6) & (State <= 9)) {  // Tx State
        Serial.print( Config.Step[State - 6].Polarity);
      }
      Serial.println();
    } // if timer
  } // if debuglevel
} // StateEntryMessage

// States are number 0 to 9 by an enum function
// State 0, Rx
// States 1:4, transition from Rx to Tx
// State 5, Tx
// State 9:6, transition from Tx to Rx
void StateMachine(sConfig_t Config, bool Key, int TimeLoop) {
// Global sequencer state machine variables
  static State_t State     = Rx;
  static State_t prevState = Tx;
  static State_t nextState;

  prevState = State;
  State = nextState; 
  switch (State) {
    case Rx: 
      if (prevState != State) {  // first time looping through Rx state
        newStateMsg(Config, prevState, State, 0, RX_PIN); // State debug with no timer
      }
      // watch for Key asserted, and transition to first Tx state
      if (Key) { // Keyed
        Serial.println("State Rx, key asserted");  
        nextState =  S1T;
      } // keyed
      break;
    // manage step 1 relay during Rx to Tx sequence
    case S1T:
      if (Key) {
        nextState = StateTimer(Config, prevState, State, S2T, TimeLoop, S2T_PIN); // next state either current or parameter
      } else { // if not keyed, transition to corresponding Rx transition state
        nextState = S1R;
      }
      break;

    // manage step 2 relay during Rx to Tx sequence
    case S2T:
      if (Key) {
        nextState = StateTimer(Config, prevState, State, S3T, TimeLoop, S3T_PIN);
      } else {
        nextState = S2R;
      }
      break;

    // manage step 3 relay during Rx to Tx sequence
    case S3T: 
      if (Key) {
        nextState = StateTimer(Config, prevState, State, S4T, TimeLoop, S4T_PIN);
      } else {
        nextState =  S3R;
      }
      break;

    // manage step 4 relay during Rx to Tx sequence
    case S4T: 
      if (Key) {
        nextState =  StateTimer(Config, prevState, State, Tx, TimeLoop, TX_PIN);
      } else {
        nextState =  S4R;
      }
      break;

    case Tx: 
      if (prevState != State) {
        newStateMsg(Config, prevState, State, 0, TX_PIN);
        // TODO, if Config.CTS.Enable...
        digitalWrite(CTSPIN, HIGH);
      }
      if (!Key) {
        nextState =  S4R;
        // TODO account for config.CTS.polarity
        digitalWrite(CTSPIN, LOW);
      }
      break;

    // manage step 4 relay during Tx to Rx sequence
    case S4R:  // step 4 release timing
      if (!Key) {
        nextState = StateTimer(Config, prevState, State, S3R, TimeLoop, S3R_PIN);
      } else {
        nextState =  S4T;
      }
      break;

    // manage step 3 relay during Tx to Rx sequence
    case S3R: 
      if (!Key) {
        nextState = StateTimer(Config, prevState, State, S2R, TimeLoop, S2R_PIN);
      } else {
        nextState =  S3T;
      }
      break;

    // manage step 2 relay during Tx to Rx sequence
    case S2R: 
      if (!Key) {
        nextState = StateTimer(Config, prevState, State, S1R, TimeLoop, S1R_PIN);
      } else {
        nextState =  S2T;
      }
      break;

    // manage step 1 relay during Tx to Rx sequence
    case S1R: 
      if (Key) {
        nextState = StateTimer(Config, prevState, State, Rx, TimeLoop, RX_PIN);
      } else {
        nextState =  S1T;
      }
      break;

    default:
      Serial.print("State: default, Error");
      Serial.print(" state ");
      Serial.print(State);
      Serial.println();
      break;
  } 
}
