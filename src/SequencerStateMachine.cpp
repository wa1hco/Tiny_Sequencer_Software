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
//                 |                     |
//                 \                     /
//                   <--(    Tx     )<--        
//                     ( test Tx time)
//                     ( set TxTimeOut)

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
// Set TimeStart on entry to a state
// Calculate TimeElapsed a single point in Sequencer()

// Sequencer masks key with Timeout flag
// State machine timing variables
unsigned long  TimeNow;     // msec, read from millis()
unsigned long  TimeStart;   // msec, time at start of state
unsigned long  TimeElapsed; // msec, time in state up to 255,000 msec
unsigned long  TimeDelay;   // msec, time setting for state, up to 255 seconds

// Tx timeout design
// Avoid timeout calculation during sequencing to improve accuracy
// Tx state watches time elapsed seconds for timeout
// on timeout, set timeout flag to mask isKey, go to timeout state
// Rx state waits for unkey, then clears timeout flag
// Tx timeout variables  
unsigned  int  Timeout;    // sec,  time setting for Tx timeout
bool           isTimeOut;        

// Sequencer state variables
static State_t State;
static State_t prevState;
static State_t nextState;

// isKey management variables
bool isKeyPin;
bool isRTSPin;
bool isKey;

void SequencerInit() {
  // Global sequencer state machine variables
  State = Rx;
  prevState = Rx;
  nextState = Rx;
  isTimeOut = false;
  TimeNow = millis();
  TimeStart = TimeNow;
  SequencerSetOutputs();
}

// set the sequencer outputs per the configuration
void SequencerSetOutputs() {
  digitalWrite(S1R_PIN, GlobalConf.Step[0].RxPolarity);
  digitalWrite(S2R_PIN, GlobalConf.Step[1].RxPolarity);
  digitalWrite(S3R_PIN, GlobalConf.Step[2].RxPolarity);
  digitalWrite(S4R_PIN, GlobalConf.Step[3].RxPolarity);
}

// private functions
State_t StateTimer(State_t prevState, State_t State, State_t nextState);
void StateMachine();

// Called from loop()
// Common stuff for state machine
// read the keying inputs, convert to postive logic, mask with Tx timeout
// read the time
void Sequencer() {
  //digitalWrite(XTRA6PIN, HIGH); // external timing monitor

  TimeNow = millis();
  TimeElapsed = TimeNow - TimeStart;
  
  // isKey processing
  // Convert isKey input to positive true logic
  isKeyPin = !digitalRead(KEYPIN); // MCU Pin state, Opto off, pin pulled up, Opto ON causes low
  // Convert RTS input to positive true logic and gate with RTS enable
  isRTSPin = GlobalConf.RTSEnable & !digitalRead(RTSPIN); // USB serial key interface, high = asserted
  // combined hardware and serial RTS positive logic signal
  isKey = (isKeyPin | isRTSPin) & !isTimeOut;  // key in OR RTS AND NOT timeout

  StateMachine();

  //digitalWrite(XTRA6PIN, LOW);  // external timing monitor
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
  // processing for a state change
  if (prevState != State) { // State change on this pass
    // New state, start time   
    TimeStart = TimeNow;
    TimeElapsed = 0;

    // New state in transition from Rx to Tx
    if ((State >= S1T) & (State <= S4T)) {  
      digitalWrite(StepPin[State], !GlobalConf.Step[StepIdx[State]].RxPolarity);
      TimeDelay = (unsigned long) GlobalConf.Step[StepIdx[State]].Tx_msec;
    }  
    // New state in transition from Tx to Rx  
    if ((State >= S4R) & (State <= S1R)) {              
      digitalWrite(StepPin[State],  GlobalConf.Step[StepIdx[State]].RxPolarity);
      TimeDelay = (unsigned long) GlobalConf.Step[StepIdx[State]].Rx_msec;
    }
    return State; // done for this pass
  } // if state change 

  // for every pass, check if delay reached
  if (TimeElapsed >= TimeDelay) {  // timer running, check for timeout
    return nextState;
  } else {
    return State;
  }
} // StateTimer()

// State Machine
// Rx is not timed, Tx is timed for timeout, sequence state have delay times
// States are numbered 0 to 9 by an enum function
// State 0, Rx
// States 1:4, transition from Rx to Tx
// State 5, Tx
// State 9:6, transition from Tx to Rx
// Tx timeout is timed and asserts isTimeout, Rx watches key inputs and releases isTimeout
void StateMachine() {
  prevState = State;
  State = nextState; 
  switch (State) {
    case Rx: 
      // Note Rx start, for consistancy
      if (State != prevState) {
        TimeStart = TimeNow;
        TimeElapsed = 0;
      }
      // if timed out, wait for both unkeyed and clear timeout flag
      if (isTimeOut) { // if timed out
        if (isKeyPin | isRTSPin) { // and either still keys
          break; // skip the rest continue waiting
        } else { 
          isTimeOut = false; // reset the Tx timeout timer, fall through to normal logic
        }
      } // if isTimeOut

      // watch for isKey asserted, and transition to first Tx state
      if (isKey) { // test combined key signal
        //Serial.println("State Rx, key asserted");  
        nextState = S1T;
      } // keyed
      break;
    
    case S1T:  // manage step 1 relay during Rx to Tx sequence
      if (isKey) { // if keyed, check timer and need for next state
        nextState = StateTimer(prevState, State, S2T); 
        digitalWrite(XTRA1PIN, !GlobalConf.Step[S1T- S1T].RxPolarity);
      } else { // if not keyed, transition to corresponding Rx transition state
        nextState = S1R;
      }
      break;

    
    case S2T:  // manage step 2 relay during Rx to Tx sequence
      if (isKey) {
        digitalWrite(XTRA2PIN, !GlobalConf.Step[S2T- S1T].RxPolarity);
        nextState = StateTimer(prevState, State, S3T);
      } else {
        nextState = S2R;
      }
      break;

    
    case S3T:  // manage step 3 relay during Rx to Tx sequence
      if (isKey) {
        digitalWrite(XTRA3PIN, !GlobalConf.Step[S3T - S1T].RxPolarity);
        nextState = StateTimer(prevState, State, S4T);
      } else {
        nextState = S3R;
      }
      break;

    
    case S4T:  // manage step 4 relay during Rx to Tx sequence
      if (isKey) {
        digitalWrite(XTRA4PIN, !GlobalConf.Step[S4T - S1T].RxPolarity);
        nextState =  StateTimer(prevState, State, Tx);
      } else {
        nextState = S4R;
      }
      break;

    case Tx: // manage Tx state timeout
    {
      if (prevState != State) {
        TimeStart = TimeNow;
        TimeElapsed = 0;
        digitalWrite(CTSPIN, GlobalConf.CTSEnable & CTS_UP);
      }
      Timeout = GlobalConf.Timeout; // sec, Tx time limit, but 0 means no timeout
      // watch for timeout
      if ((Timeout > 0) & (TimeElapsed > (((unsigned long) Timeout) * 1000UL))) {
        isTimeOut = true; // block isKey on next pass, accurate enough    
        // fall through to release of key
      }
      // watch for release of key
      if (!isKey) { 
        nextState =  S4R;
        digitalWrite(CTSPIN, CTS_DOWN);
      }
      break;
    }

    // manage step 4 relay during Tx to Rx sequence
    case S4R:  // step 4 release timing
      if (!isKey) {
        nextState = StateTimer(prevState, State, S3R);
        digitalWrite(XTRA4PIN, GlobalConf.Step[S1R - S4R].RxPolarity);

      } else {
        nextState =  S4T;
      }
      break;

    // manage step 3 relay during Tx to Rx sequence
    case S3R: 
      if (!isKey) {
        nextState = StateTimer(prevState, State, S2R);
        digitalWrite(XTRA3PIN, GlobalConf.Step[S1R - S3R].RxPolarity);
      } else {
        nextState =  S3T;
      }
      break;

    // manage step 2 relay during Tx to Rx sequence
    case S2R: 
      if (!isKey) {
        nextState = StateTimer(prevState, State, S1R);
        digitalWrite(XTRA2PIN, GlobalConf.Step[S1R - S2R].RxPolarity);
      } else {
        nextState =  S2T;
      }
      break;

    // manage step 1 relay during Tx to Rx sequence
    case S1R: 
      if (!isKey) {
        nextState = StateTimer(prevState, State, Rx);
        digitalWrite(XTRA1PIN, GlobalConf.Step[S1R - S1R].RxPolarity);
      } else {
        nextState =  S1T;
      }
      break;
  } // switch(State)
} // StateMachine()
