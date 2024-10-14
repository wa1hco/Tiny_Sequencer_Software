// Tiny Sequencer
// Licensed under GPL V3, copyright 2024 by jeff millar, wa1hco

// on Key asserted, transition to transmit, assumes key remain asserted
//   state 1, assert relay 1, wait relay 1 assert time; usually antenna transfer relay
//   state 2, assert relay 2, wait relay 2 assert time; usually transverter relays
//   state 3, assert relay 3, wait relay 3 assert time; usually (something)
//   state 4, assert relay 4, wait relay 4 assert time; transmitter
//   state 5, transmit ready
//   
// on key released, transition to receive, assumes key remains released
//   release relay 4, 
//   wait relay 4 release time, release relay 3
//   wait relay 3 release time, release relay 2
//   wait relay 2 release time, release relay 1
//   wait relay 1 release time, receive ready

// on key released during transition to transmit
//   transition out of current state to release

// on key asserted during transition to receive
//   complete transition to receive
//   enter transition to transmit

// Each sequence step, called from state machine
//   enter as part or loop() at regular intervals
//   montior KEY asserted or not asserted
//   one timer for either assert or release
//   monitor timer for state

// State Receive
//   If KEY asserted, transtion to State 1
// State Transmit
//   If KEY released, transition to State 4

// State N, manage sequence N output
//   Parameters:State changed flag, KEY value, 
//   Internal: Timer value, assert and release time
//   Returns: In Process, Assert complete, Released complete
//   if state changed (first pass thru State after a transition)
//     if KEY asserted
//       initialize timer using assert time
//       assert output
//     if KEY released
//       intialize timer using release time
//       release output
//     previousKEY = KEY
//     return InProcess
//  
//   if state did not change and KEY == previousKEY
//     update timer
//     if timer in progress
//       return InProcess
//     if timer complete
//       return Complete  (prevoiusKEY contains info about tx or rx transition)
//
//   if KEY != previousKEY
//     if KEY asserted
//       initialize timer using asert time
//       assert output
//   if KEY released
//     initialize timer using release time
//     release output
//   previousKEY = KEY
//   return InProcess
//     
//       

//  ATTinyX14 Hardware V1-rc1 Pin Definitions
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
// Pin      |1  |2  |3  |4  |5  |6  |7  |8  |9  |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |20 |
// Port     |   |PA4|PA5|PA6|PA7|PB5|PB4|PB3|PB2|PB1|PB0|PC0|PC1|PC2|PC3|PA0|PA1|PA2|PA3|   |
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
// Power    |PWR|   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |GND|
// PWM      |   |   |   |   |   |   |   |8  |9  |   |   |12 |13 |14 |15 |   |   |   |   |   |
// UPDI     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |PDI|   |   |   |   |
// Analog   |A0~|A1~|A2 |A3 |A4 |A5 |   |   |   |A8~|A9~|A2 |A3 |   |   |A17|A14|A15|A16|   |
// I2C      |   |   |   |   |   |   |   |   |   |SDA|SCL|   |   |   |   |   |   |   |   |   |
// Serial1  |   |   |   |   |   |   |   |RXD|TXD|   |   |   |   |   |   |   |   |   |   |   |
// DAC      |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
// CLOCK    |   |   |   |   |   |   |   |OSC|OSC|   |   |   |   |   |   |   |   |   |EXT|   |
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|   |   |   |   |   |   |

// Sequencer board use of pins
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
// Pins     |1  |2  |3  |4  |5  |6  |7  |8  |9  |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |20 |  
// Port     |   |PA4|PA5|PA6|PA7|PB5|PB4|PB3|PB2|PB1|PB0|PC0|PC1|PC2|PC3|PA0|PA1|PA2|PA3|   |
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|  // pin state initialization

// SeqOut   |   |   |   |   |   |   |   |   |   |   |S4 |S3 |S2 |S1 |   |   |   |   |   |
// Serial   |   |   |   |   |   |   |   |RXD|TXD|   |   |   |   |   |   |   |RTS|CTS|   |   |
// Key      |   |   |   |   |   |   |   |   |   |   |   |   |   |   |KEY|   |   |   |   |   |
// LED      |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |LED|   |
// UPDI     |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |PDI|   |   |   |   |
// EXTRA    |   |1  |2  |3  |4  |5  |6  |   |   |   |   |   |   |   |   |   |   |   |   |   |
//--------- |---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
// Unused   |   |   |   |   |   |   |   |   |   |10 |   |   |   |   |   |   |   |   |   |   |
//--------- |---|---|---|---|---|---|---|---|---|---|---|---|---|---|   |   |   |   |   |   |

//The following code is still for the ATTinyX16

// define MCU pins
#define KEYPIN    PIN_PC3 // MCU 15, Breakout JP4 8
#define STEP1PIN  PIN_PC2 // MCU 14, Breakout JP4 9
#define STEP2PIN  PIN_PC1 // MCU 13, Breakout JP4 10
#define STEP3PIN  PIN_PB0 // MCU 11, Breakout JP2 10
#define STEP4PIN  PIN_PA3 // MCU 19, Breakout JP4 4
#define RTSPIN    PIN_PA1
#define CTSPIN    PIN_PA2
#define LEDPIN    PIN_PA3
#define XTRA1PIN  PIN_PA4
#define XTRA2PIN  PIN_PA5
#define XTRA3PIN  PIN_PA6
#define XTRA4PIN  PIN_PA7
#define XTRA5PIN  PIN_PB5
#define XTRA6PIN  PIN_PB4

// User defines the step timing per the data sheet for the relay
// delays specifiec in msec after key asserted
int  AssertTimes[] = { 300, 300, 300, 300};
int ReleaseTimes[] = { 200, 200, 200, 200};

// define input pin polarity
#define KEYASSERTED LOW

// Sequencer board design drives high to light the LED in the optoisolator, which causes the contacts to close,
// define sequencer contacts in terms of MCU output pin
#define CLOSED HIGH  // drive MCU pin high to assert
#define OPEN    LOW
// User defines contact closure state when not keyed
#define STEP1ASSERTED CLOSED  // Normally Open, close when keyed
#define STEP2ASSERTED CLOSED
#define STEP3ASSERTED CLOSED
#define STEP4ASSERTED CLOSED

#define LOOPTIMEINTERVAL 10 // msec

enum StateNames {Rx, S1T, S2T, S3T, S4T, Tx, S4R, S3R, S2R, S1R};

void setup() {
  // pin configuration
  pinMode(  KEYPIN, INPUT_PULLUP);
  pinMode(STEP1PIN, OUTPUT);
  pinMode(STEP2PIN, OUTPUT);
  pinMode(STEP3PIN, OUTPUT);
  pinMode(STEP4PIN, OUTPUT);
  pinMode(  LEDPIN, OUTPUT);
  pinMode(  RTSPIN, INPUT_PULLUP);
  pinMode(  CTSPIN, OUTPUT);

  // pin state initialization
  digitalWrite(STEP1PIN, (uint8_t) !STEP1ASSERTED);
  digitalWrite(STEP2PIN, (uint8_t) !STEP2ASSERTED);
  digitalWrite(STEP3PIN, (uint8_t) !STEP3ASSERTED);
  digitalWrite(STEP4PIN, (uint8_t) !STEP4ASSERTED);
  digitalWrite(  LEDPIN, HIGH);
  digitalWrite(  CTSPIN, LOW);
  Serial.begin(19200);

  delay(1000);
  Serial.println("Sequencer startup");
}

// this loop is entered seveal seconds after power on

void loop() {
  // loop internal variables, need static because loop() exits
  static int State = Rx;
  static int StatePrevious = Rx;
  int Key;
  //int RTS;
  //static int RTSPrevious;
  static int Timer;
  int TimeLoop;
  unsigned long TimeNow;
  static unsigned long TimePrevious = millis(); // initial on first entry to loop

  // time calculation for this pass through the loop
  // On entry to loop(), millis() is at about 5 seconds !?
  TimeNow = millis();

  // Calculate time increment for this pass through loop()
  TimeLoop = (unsigned int)(TimeNow - TimePrevious);

  TimePrevious = TimeNow;  // always, either first or later pass

  // Count up if key asserted, count down if not asserted
  Key = digitalRead(KEYPIN);
  //RTS = digitalRead(RTSPIN);

  switch (State) {
    case Rx:
      if (StatePrevious != Rx) {
        Serial.print("State Rx: prev ");
        Serial.print(StatePrevious);
        Serial.print(", curr ");
        Serial.print(State);
        Serial.println();
        digitalWrite(CTSPIN, LOW);
      }
      if (Key == KEYASSERTED) { // Keyed
        Serial.println("State Rx, key asserted");  
        State = S1T;
      } // keyed
      StatePrevious = Rx;
      break;

    case S1T:
      //Serial.print("State S1T, Prev ");
      //Serial.print(StatePrevious);
      //Serial.print(" Curr ");
      //Serial.print(State);
      //Serial.println();
      if (Key != KEYASSERTED) {
        State = S1R;
        StatePrevious = S1T;  
        break;
      }
      if (StatePrevious == S1T) { // timer running
        Timer -= TimeLoop;
        //Serial.print("State S1T, Timer ");
        //Serial.print(Timer);
        //Serial.println(" ms");
        if (Timer <= 0) {
          State = S2T;
        }
      } else { // state change event
        digitalWrite(STEP1PIN, (uint8_t) STEP1ASSERTED);
        Timer = AssertTimes[0];
        Serial.print("S1T state entry, timer ");
        Serial.print(Timer);
        Serial.println(" ms");
      }
      StatePrevious = S1T;  
      break;

    case S2T:
      if (Key != KEYASSERTED) {
        StatePrevious = S2T;  
        State = S2R;
        break;
      }
      if (StatePrevious == State) { // timer running
        Timer -= TimeLoop;
        //Serial.print("State S2T, timer ");
        //Serial.print(Timer);
        //Serial.println(" ms");
        if (Timer <= 0) {
          State = S3T;
        }
      } else { // key event
        digitalWrite(STEP2PIN, (uint8_t) STEP2ASSERTED);
        Timer = AssertTimes[1];
        Serial.print("S2T state entry, timer ");
        Serial.print(Timer);
        Serial.println(" ms");
      }
      StatePrevious = S2T;  
      break;

    case S3T:
      if (Key != KEYASSERTED) {
        StatePrevious = S3T;
        State = S3R;
        break;
      }
      if (StatePrevious == State) { // timer running
        Timer -= TimeLoop;
        //Serial.print("State S3T, timer ");
        //Serial.print(Timer);
        //Serial.println(" ms");
        if (Timer <= 0) {
          State = S4T;
        }
      } else { // key event
        digitalWrite(STEP3PIN, (uint8_t) STEP3ASSERTED);
        Timer = AssertTimes[2];
        Serial.print("S3T state entry, timer ");
        Serial.print(Timer);
        Serial.println(" ms");
      }
      StatePrevious = S3T;  
      break;

    case S4T:
      if (Key != KEYASSERTED) {
        StatePrevious = S4T;  
        State = S4R;
        break;
      }
      if (StatePrevious == State) { // timer running
        Timer -= TimeLoop;
        //Serial.print("State S4T, timer");
        //Serial.print(Timer);
        //Serial.println(" ms");
        if (Timer <= 0) {
          State = Tx;
        }
      } else { // key event
        digitalWrite(STEP4PIN, (uint8_t) STEP4ASSERTED);
        Timer = AssertTimes[3];
        Serial.print("S4T state entry, timer ");
        Serial.print(Timer);
        Serial.println(" ms");
      }
      StatePrevious = S4T;  
      break;

    case Tx:
      if (StatePrevious != State) {
        Serial.print("State: Tx prev ");
        Serial.print(StatePrevious);
        Serial.print(", curr ");
        Serial.print(State);
        Serial.println();
        digitalWrite(CTSPIN, HIGH);
      }
      if (Key != KEYASSERTED) {
        State = S4R;
      }
      StatePrevious = Tx;  
      break;

    case S4R: // step 4 release timing
      if (Key == KEYASSERTED) {
        StatePrevious = S4R;  
        State = S4T;
        break;
      }
      if (StatePrevious == State) { // timer running, next step on timout
        Timer -= TimeLoop;
        //Serial.print("State S4R, Timer ");
        //Serial.print(Timer);
        //Serial.println(" ms");
        if (Timer <= 0) {  
          State = S3R;
        }
      } else { // key event, release the relay, set timer
        digitalWrite(STEP4PIN, (uint8_t) !STEP4ASSERTED);
        Timer = ReleaseTimes[3];
        Serial.print("S4R state entry, timer ");
        Serial.print(Timer);
        Serial.println(" ms");
      }
      StatePrevious = S4R;
      break;

    case S3R:
      if (Key == KEYASSERTED) {
        StatePrevious = State;  
        State = S3T;
        break;
      }
      if (StatePrevious == State) { // timer running, next step on timout
        Timer -= TimeLoop;
        //Serial.print("State S3R, Timer: ");
        //Serial.print(Timer);
        //Serial.println(" ms");
        if (Timer <= 0) {
          StatePrevious = State;  
          State = S2R;
        }
      } else { // key event, release the relay, set timer
        digitalWrite(STEP3PIN, (uint8_t) !STEP3ASSERTED);
        Timer = ReleaseTimes[2];
        Serial.print("S3R state entry, timer ");
        Serial.print(Timer);
        Serial.println(" ms");
      }
      StatePrevious = S3R;
      break;

    case S2R:
      if (Key == KEYASSERTED) {
        StatePrevious = State;  
        State = S2T;
        break;
      }
      if (StatePrevious == State) { // timer running, next step on timout
        Timer -= TimeLoop;
        //Serial.print("State S2R, Timer: ");
        //Serial.print(Timer);
        //Serial.println(" ms");
        if (Timer <= 0) {
          StatePrevious = State;  
          State = S1R;
        }
      } else { // key event, release the relay, set timer
        digitalWrite(STEP2PIN, (uint8_t) !(STEP2ASSERTED == CLOSED));
        Timer = ReleaseTimes[1];
        Serial.print("S2R state entry, timer ");
        Serial.print(Timer);
        Serial.println(" ms");
      }
      StatePrevious = S2R;
      break;

    case S1R:
      if (Key == KEYASSERTED) {
        StatePrevious = State;  
        State = S1T;
        break;
      }
      if (StatePrevious == State) { // timer running, next step on timout
        Timer -= TimeLoop;
        //Serial.print("State S1R, Timer: ");
        //Serial.print(Timer);
        //Serial.println(" ms");
        if (Timer <= 0) {
          StatePrevious = State;  
          State = Rx;
        }
      } else { // key event, release the relay, set timer
        digitalWrite(STEP1PIN, (uint8_t) !STEP1ASSERTED);
        Timer = ReleaseTimes[0];
        Serial.print("S1R state entry, timer ");
        Serial.print(Timer);
        Serial.println(" ms");
      }
      StatePrevious = S1R;
      break;

    default:
      Serial.println("State: default, Error");
      break;
  }
  delay(LOOPTIMEINTERVAL);
}
