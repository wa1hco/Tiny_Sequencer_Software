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

// Private to StateMachine functions
// State Machine definitions
enum         State_t           {  Rx,     S1T,     S2T,     S3T,     S4T,    Tx,     S4R,     S3R,     S2R,     S1R }; // numbered 0 to 9
const char*  StateName[][10] = {" Rx",   "S1T",   "S2T",   "S3T",   "S4T", " Tx",   "S4R",   "S3R",   "S2R",   "S1R"}; // for debug messages
uint8_t   StepPin[] =          {    0, S1T_PIN, S2T_PIN, S3T_PIN, S4T_PIN,     0, S4R_PIN, S3R_PIN, S2R_PIN, S1R_PIN}; // map state to hardware pin

// private functions
void   newStateMsg(sConfig_t Config, State_t prevState, State_t State, int StepTime);
State_t StateTimer(sConfig_t Config, State_t prevState, State_t State, State_t nextState, int TimeLoop);

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
State_t StateTimer (sConfig_t Config, State_t prevState, State_t State, State_t nextState, int TimeLoop) {
  static int StepTime;
  // State change on this pass
  if (prevState != State) { 
    if ((State >= S1T) & (State <= S4T)) {                    // States for transition from Rx to Tx 
      digitalWrite(StepPin[State], (uint8_t) Config.Step[S1T - 1].TxPolarity);
      StepTime = Config.Step[State - 1].Tx_msec;             // initialize the timer
    }    
    if ((State >= S4R) & (State <= S1R)) {                   // States for transition from Tx to Rx
      digitalWrite(StepPin[State], (uint8_t) !Config.Step[S1R - 6].TxPolarity);
      StepTime = Config.Step[State - 6].Rx_msec;             // initialize the timer
    }
    newStateMsg(Config, prevState, State, StepTime);         // debug message
  } // if state change 

  StepTime -= TimeLoop;

  if (StepTime <= 0) {  // timer running, check for timeout
    State = nextState;
  }
  return State;
} // StateTimer()

// common message function for all states
void newStateMsg(sConfig_t Config, State_t prevState, State_t State, int StepTime) {
  if ((State == Rx) | State == Tx) {
    char Msg[80];
    snprintf(Msg, 80, "State from %s to %s", StateName[0][prevState], StateName[0][State]);
    Serial.println(Msg); 
  } else {
    char Msg[80];
    snprintf(Msg, 80, "State from %s to %s, timer %d msec", StateName[0][prevState], StateName[0][State], StepTime);
    Serial.println(Msg);
  }
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
        newStateMsg(Config, prevState, State, 0); // State debug with no timer
      }
      // watch for Key asserted, and transition to first Tx state
      if (Key) { // Keyed
        //Serial.println("State Rx, key asserted");  
        nextState =  S1T;
      } // keyed
      break;
    // manage step 1 relay during Rx to Tx sequence
    case S1T:
      if (Key) {
        nextState = StateTimer(Config, prevState, State, S2T, TimeLoop); // next state either current or parameter
      } else { // if not keyed, transition to corresponding Rx transition state
        nextState = S1R;
      }
      break;

    // manage step 2 relay during Rx to Tx sequence
    case S2T:
      if (Key) {
        nextState = StateTimer(Config, prevState, State, S3T, TimeLoop);
      } else {
        nextState = S2R;
      }
      break;

    // manage step 3 relay during Rx to Tx sequence
    case S3T: 
      if (Key) {
        nextState = StateTimer(Config, prevState, State, S4T, TimeLoop);
      } else {
        nextState =  S3R;
      }
      break;

    // manage step 4 relay during Rx to Tx sequence
    case S4T: 
      if (Key) {
        nextState =  StateTimer(Config, prevState, State, Tx, TimeLoop);
      } else {
        nextState =  S4R;
      }
      break;

    case Tx: 
      if (prevState != State) {
        newStateMsg(Config, prevState, State, 0);
        // TODO, if Config.CTS.Enable...
        digitalWrite(CTSPIN, CTS_UP);
      }
      if (!Key) {
        nextState =  S4R;
        digitalWrite(CTSPIN, CTS_DOWN);
      }
      break;

    // manage step 4 relay during Tx to Rx sequence
    case S4R:  // step 4 release timing
      if (!Key) {
        nextState = StateTimer(Config, prevState, State, S3R, TimeLoop);
      } else {
        nextState =  S4T;
      }
      break;

    // manage step 3 relay during Tx to Rx sequence
    case S3R: 
      if (!Key) {
        nextState = StateTimer(Config, prevState, State, S2R, TimeLoop);
      } else {
        nextState =  S3T;
      }
      break;

    // manage step 2 relay during Tx to Rx sequence
    case S2R: 
      if (!Key) {
        nextState = StateTimer(Config, prevState, State, S1R, TimeLoop);
      } else {
        nextState =  S2T;
      }
      break;

    // manage step 1 relay during Tx to Rx sequence
    case S1R: 
      if (!Key) {
        nextState = StateTimer(Config, prevState, State, Rx, TimeLoop);
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
