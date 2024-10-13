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
//----------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
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
#define RCS_PIN   PIN_PA1
#define RCS_PIN   PIN_PA2
#define LED_PIN   PIN_PA3
#define XTRA1PIN  PIN_PA4
#define XTRA2PIN  PIN_PA5
#define XTRA3Pin  PIN_PA6
#define XTRA4Pin  PIN_PA7
#define XTRA5Pin  PIN_PB5
#define XTRA6Pin  PIN_PB4

// Sequencer board design drives high to light the LED in the optoisolator, which causes the contacts to close,
// define sequencer contacts in terms of MCU output pin
#define CLOSED HIGH  // drive MCU pin high to assert
#define OPEN LOW

// User defines the StepTiming relative to key change
// delays specifiec in msec after key asserted
#define STEP1TIME   0   // msec, usually no delay on first stage of switching
#define STEP2TIME 300   // msec
#define STEP3TIME 600   // msec
#define STEP4TIME 900   // msec

// define input pin polarity
#define KEYASSERTED LOW

// User defines contact closure state when not keyed
#define STEP1ASSERTED CLOSED  // Normally Open, close when keyed
#define STEP2ASSERTED CLOSED
#define STEP3ASSERTED CLOSED
#define STEP4ASSERTED CLOSED

#define LOOPTIMEINTERVAL 1

void setup() {
  // pin configuration
  pinMode(KEYPIN, INPUT_PULLUP);
  pinMode(Step1Pin, OUTPUT);
  pinMode(Step2Pin, OUTPUT);
  pinMode(Step3Pin, OUTPUT);
  pinMode(Step4Pin, OUTPUT);

  pinMode(LED_Pin, OUTPUT);

  // pin state initialization
  digitalWrite(Step1Pin, (uint8_t)!STEP1ASSERTED);
  digitalWrite(Step2Pin, (uint8_t)!STEP2ASSERTED);
  digitalWrite(Step3Pin, (uint8_t)!STEP3ASSERTED);
  digitalWrite(Step4Pin, (uint8_t)!STEP4ASSERTED);

  digitalWrite(LED_Pin, HIGH);
}

// this loop is entered seveal seconds after power on
// cases to consider
// - Key asserted all during power up

void loop() {
  // loop internal variables, need static because loop() exits
  static int KeyState;
  static int SequenceTime = 0;
  static unsigned long TimeNow;
  static unsigned int TimeLoop;
  static unsigned long TimePrevious = millis(); // initial on first entry to loop

  // time calculation for this pass through the loop
  // On entry to loop(), millis() is at about 5 seconds !?
  TimeNow = millis();

  // Calculate time increment for this pass through loop()
  TimeLoop = (unsigned int)(TimeNow - TimePrevious);

  TimePrevious = TimeNow;  // always, either first or later pass

  // Count up if key asserted, count down if not asserted
  KeyState = digitalRead(KEYPIN);

  // SequenceTime increases if key asserted and decreases if key not asserted
  if (KeyState == KEYASSERTED) {
    SequenceTime += (int)TimeLoop;  // ramp up time value
  } else {
    SequenceTime -= (int)TimeLoop;  // ramp down timer value
  }
  // SequenceTime is clamped to between 0 and maximum sequence step value
  if (SequenceTime < 0) SequenceTime = 0;
  if (SequenceTime > STEP4TIME) SequenceTime = STEP4TIME;

  // Write the state of the sequencer to each step output
  // This form avoids the use of if statements, handles assert and de-assert, and handles both NO and NC config
  // Time   NO/NC     Time  XOR NOT  NO/NC  digitalWRITE
  //   <    CLOSED    0     ^   !    1      = 0
  //   >    CLOSED    1     ^   !    1      = 1
  //   <    OPEN      0     ^   !    0      = 1
  //   >    OPEN      1     ^   !    0      = 0
  digitalWrite(Step1Pin, (uint8_t)((SequenceTime >  STEP1TIME) ^ !(STEP1ASSERTED == CLOSED)));
  digitalWrite(Step2Pin, (uint8_t)((SequenceTime >= STEP2TIME) ^ !(STEP2ASSERTED == CLOSED)));
  digitalWrite(Step3Pin, (uint8_t)((SequenceTime >= STEP3TIME) ^ !(STEP3ASSERTED == CLOSED)));
  digitalWrite(Step4Pin, (uint8_t)((SequenceTime >= STEP4TIME) ^ !(STEP4ASSERTED == CLOSED)));

  delay(LOOPTIMEINTERVAL);
}
