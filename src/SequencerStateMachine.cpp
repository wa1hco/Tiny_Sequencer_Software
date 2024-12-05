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
enum  State_t                   {  Rx,   S1T,     S2T,     S3T,     S4T,    Tx,   S4R,     S3R,     S2R,     S1R    }; // numbered 0 to 9
const uint8_t StepIdx[]       = {   9,     0,       1,       2,       3,     9,     3,       2,       1,       0    }; // index into Config.Step[] array
const char*   StateName[][10] = {" Rx", "S1T",   "S2T",   "S3T",   "S4T", " Tx", "S4R",   "S3R",   "S2R",   "S1R"   }; // for debug messages
const uint8_t StepPin[]       = {   0,   S1T_PIN, S2T_PIN, S3T_PIN, S4T_PIN, 0,   S4R_PIN, S3R_PIN, S2R_PIN, S1R_PIN}; // map state to hardware pin

// Time uses unsigned long millis().  But steps use uint8_t TimeDelay.
// Avoid using unsigned long variable and calculation in most places.
// Single global state machine timer is required, accessed by all states
// unsigned long    TimeStart
// unsigned uint8_t TimeDelay
// unsigned int     TimeElapsed
// Set TimeStart on entry to a state
// Calculate TimeElapsed a single point in Sequencer()
// Pass TimeElapsed to each state for comparison with its delay

// Tx timeout variables
unsigned long Timeout_time_millis; 
         bool KeyTimeOut; // used by Tx timout management in loop()

// private functions
State_t StateTimer(State_t prevState, State_t State, State_t nextState);

// Called from loop()
void Sequencer() {
  digitalWrite(XTRA6PIN, HIGH);

  // read the three causes for state events KeyPin, RTSPin, millis()
  // Convert Key input to positive true logic
  uint8_t KeyPin = digitalRead(KEYPIN);  // low when opto led on
  uint8_t KeyPositive = !KeyPin; // MCU Pin state, Opto off, pin pulled up, Opto ON causes low
  bool    Key;

  // Convert RTS input to positive true logic
  bool RTSPin = digitalRead(RTSPIN);
  bool RTSPositive = !RTSPin;  // key if RTS UP, meaning RTS/ and MCU pin low
  
  bool KeyState = KeyPositive; // hardware Key interface, high = asserted
  bool RTSState = GlobalConf.RTSEnable & RTSPositive; // USB serial key interface, high = asserted
  
  // Update the state machine timer global variables
  TimeNow = millis();
  if (TimeDelay == 0) { // flag that no timer is setup
    TimeElapsed = 0;
  } else {
    TimeElapsed = TimeNow - TimeStart;
  }
  
  // set the timeout timer to the future if unkeyed
  // when keyed, the timeout timer is not updated
  if (!KeyState & !RTSState) {  // if unkeyed, reset the tx timeout timer
    Timeout_time_millis = TimeNow + (unsigned long) GlobalConf.Timeout * 1000; //sec to msec
    KeyTimeOut = false;
  }
  
  // if Tx timer timeout, set KeyTimeOut flag
  if (TimeNow > Timeout_time_millis) {
    KeyTimeOut = true;
  }
  bool isTimerDisabled = (GlobalConf.Timeout == 0);  // timeout = 0 means disable timeout

  // used by state machine, combined from hardware and timeout

  if (isTimerDisabled){
    Key = (KeyState | RTSState);
  } else {
    Key = (KeyState | RTSState) & !KeyTimeOut;  // key in OR RTS AND NOT timeout
  }
  StateMachine(Key);
  digitalWrite(XTRA6PIN, LOW);  
}

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
//  GlobalConf: polarity of step pins
//  prevState, State: detect when a new state
//  nextState: for when timer timesout
//  TimeLoop: time between calls to statemachine, used to decrement timer
//  StepPin: pin controlled by next state

State_t StateTimer (State_t prevState, State_t State, State_t nextState) {
  if (prevState != State) { // State change on this pass
    if ((State >= S1T) & (State <= S4T)) { // States for transition from Rx to Tx 
      digitalWrite(StepPin[State], !GlobalConf.Step[StepIdx[State]].RxPolarity);
      TimeDelay = GlobalConf.Step[StepIdx[State]].Tx_msec;
    }    
    if ((State >= S4R) & (State <= S1R)) {              // States for transition from Tx to Rx
      digitalWrite(StepPin[State],  GlobalConf.Step[StepIdx[State]].RxPolarity);
      TimeDelay = GlobalConf.Step[StepIdx[State]].Rx_msec;
    }
    // each call with a state change sets the start time   
    TimeStart = TimeNow;
    TimeElapsed = 1;
  } // if state change 

  if (TimeElapsed >= TimeDelay) {  // timer running, check for timeout
    return nextState;
  } else {
    return State;
  }
} // StateTimer()

// States are numbered 0 to 9 by an enum function
// State 0, Rx
// States 1:4, transition from Rx to Tx
// State 5, Tx
// State 9:6, transition from Tx to Rx
void StateMachine(bool Key) {
// Global sequencer state machine variables
  static State_t State     = Rx;
  static State_t prevState = Tx;
  static State_t nextState;

  prevState = State;
  State = nextState; 
  switch (State) {
    case Rx: 
      // lock in the receive state on every pass
      digitalWrite(S1T_PIN, (uint8_t) GlobalConf.Step[0].RxPolarity); // config as receive mode 
      digitalWrite(S2T_PIN, (uint8_t) GlobalConf.Step[1].RxPolarity); // config as receive mode 
      digitalWrite(S3T_PIN, (uint8_t) GlobalConf.Step[2].RxPolarity); // config as receive mode 
      digitalWrite(S4T_PIN, (uint8_t) GlobalConf.Step[3].RxPolarity); // config as receive mode 

      // watch for Key asserted, and transition to first Tx state
      if (Key) { // Keyed
        //Serial.println("State Rx, key asserted");  
        nextState =  S1T;
      } // keyed
      break;
    // manage step 1 relay during Rx to Tx sequence
    case S1T:
      if (Key) { // if keyed, check timer and need for next state
        nextState = StateTimer(prevState, State, S2T); 
        digitalWrite(XTRA1PIN, !GlobalConf.Step[S1T- S1T].RxPolarity);
      } else { // if not keyed, transition to corresponding Rx transition state
        nextState = S1R;
      }
      break;

    // manage step 2 relay during Rx to Tx sequence
    case S2T:
      if (Key) {
        digitalWrite(XTRA2PIN, !GlobalConf.Step[S2T- S1T].RxPolarity);
        nextState = StateTimer(prevState, State, S3T);
      } else {
        nextState = S2R;
      }
      break;

    // manage step 3 relay during Rx to Tx sequence
    case S3T: 
      if (Key) {
        digitalWrite(XTRA3PIN, !GlobalConf.Step[S3T - S1T].RxPolarity);
        nextState = StateTimer(prevState, State, S4T);
      } else {
        nextState =  S3R;
      }
      break;

    // manage step 4 relay during Rx to Tx sequence
    case S4T: 
      if (Key) {
        digitalWrite(XTRA4PIN, !GlobalConf.Step[S4T - S1T].RxPolarity);
        nextState =  StateTimer(prevState, State, Tx);
      } else {
        nextState =  S4R;
      }
      break;

    case Tx: 
      if (prevState != State) {
        // TODO, if GlobalConf.CTSEnable...
        //digitalWrite(CTSPIN, CTS_UP);
      }
      if (!Key) { // watch for release of key
        nextState =  S4R;
        digitalWrite(CTSPIN, CTS_DOWN);
      }
      break;

    // manage step 4 relay during Tx to Rx sequence
    case S4R:  // step 4 release timing
      if (!Key) {
        nextState = StateTimer(prevState, State, S3R);
        digitalWrite(XTRA4PIN, GlobalConf.Step[S1R - S4R].RxPolarity);

      } else {
        nextState =  S4T;
      }
      break;

    // manage step 3 relay during Tx to Rx sequence
    case S3R: 
      if (!Key) {
        nextState = StateTimer(prevState, State, S2R);
        digitalWrite(XTRA3PIN, GlobalConf.Step[S1R - S3R].RxPolarity);
      } else {
        nextState =  S3T;
      }
      break;

    // manage step 2 relay during Tx to Rx sequence
    case S2R: 
      if (!Key) {
        nextState = StateTimer(prevState, State, S1R);
        digitalWrite(XTRA2PIN, GlobalConf.Step[S1R - S2R].RxPolarity);
      } else {
        nextState =  S2T;
      }
      break;

    // manage step 1 relay during Tx to Rx sequence
    case S1R: 
      if (!Key) {
        nextState = StateTimer(prevState, State, Rx);
        digitalWrite(XTRA1PIN, GlobalConf.Step[S1R - S1R].RxPolarity);
      } else {
        nextState =  S1T;
      }
      break;
  } // switch(State)
} // StateMachine()
