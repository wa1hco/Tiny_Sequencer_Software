// Tiny Sequencer
// Licensed under GPL V3, copyright 2024 by jeff millar, wa1hco

// define pin numbers
#define KEYPIN   0
#define STEP1PIN 1
#define STEP2PIN 2
#define STEP3PIN 3
#define STEP4PIN 4

// Sequencer board design drives high to light the LED in the optoisolator, which causes the contacts to close, 
// define sequencer contacts in terms of MCU output pin
#define CLOSED HIGH  // drive MCU pin high to assert
#define OPEN   LOW

// User defines the StepTiming relative to key change
#define STEP1TIME 300 // msec, delay from key asserted to output contacts asserted
#define STEP2TIME 600 // msec
#define STEP3TIME 900 // msec
#define STEP4TIME 1200 // msec

// define input pin polarity
#define KEYASSERTED LOW

// User defines contact closure state when not keyed
#define STEP1ASSERTED CLOSED // Normally Open, close when keyed
#define STEP2ASSERTED CLOSED
#define STEP3ASSERTED CLOSED
#define STEP4ASSERTED CLOSED

#define LOOPTIMEINTERVAL 10

void setup() {
  // pin configuration
  pinMode(KEYPIN, INPUT_PULLUP);
  pinMode(STEP1PIN, OUTPUT);
  pinMode(STEP2PIN, OUTPUT);
  pinMode(STEP3PIN, OUTPUT);
  pinMode(STEP4PIN, OUTPUT);

  // pin state initialization
  digitalWrite(STEP1PIN, (uint8_t) !STEP1ASSERTED);
  digitalWrite(STEP2PIN, (uint8_t) !STEP2ASSERTED);
  digitalWrite(STEP3PIN, (uint8_t) !STEP3ASSERTED);
  digitalWrite(STEP4PIN, (uint8_t) !STEP4ASSERTED);
}

// this loop is entered seveal seconds after power on
// cases to consider
// - Key asserted all during power up

void loop() {
  // loop internal variables, don't need static because loop() never exits
  int KeyState;
  int SequenceTime = 0;
  unsigned long TimeNow;
  unsigned int TimeLoop = LOOPTIMEINTERVAL;
  unsigned long TimePrevious = millis();

  while (true) {
    // time calculation for this pass through the loop
    // On entry to loop(), millis() is at about 5 seconds !?
    TimeNow = millis();

    // Calculate time increment for this pass through loop()
    TimeLoop = (unsigned int) (TimeNow - TimePrevious);

    TimePrevious = TimeNow;  // always, either first or later pass

    // Count up if key asserted, count down if not asserted
    KeyState = digitalRead(KEYPIN);

    // manage the sequence timer depending on key input
    // SequenceTime ramps up if key asserted and ramps down if key not asserted
    if (KeyState == KEYASSERTED) {
      SequenceTime += (int) TimeLoop; // ramp up time value
    } else {
      SequenceTime -= (int) TimeLoop; // ramp down timer value
    }
    // SequenceTime is clamped to between 0 and maximum sequence step value
    if (SequenceTime < 0) SequenceTime = 0;
    if (SequenceTime > STEP4TIME) SequenceTime = STEP4TIME;

    // Write the state of the sequencer to each
    // This form avoids the use of if statements, handles assert and de-assert, and handles both NO and NC config
    // Time   NO/NC     Time  XOR NOT  NO/NC  digitalWRITE
    //   <    CLOSED    0     ^   !    1      = 0
    //   >    CLOSED    1     ^   !    1      = 1
    //   <    OPEN      0     ^   !    0      = 1   
    //   >    OPEN      1     ^   !    0      = 0

    digitalWrite(STEP1PIN, (uint8_t) ((SequenceTime > STEP1TIME) ^ !(STEP1ASSERTED == CLOSED)));
    digitalWrite(STEP2PIN, (uint8_t) ((SequenceTime > STEP2TIME) ^ !(STEP2ASSERTED == CLOSED)));
    digitalWrite(STEP3PIN, (uint8_t) ((SequenceTime > STEP3TIME) ^ !(STEP3ASSERTED == CLOSED)));
    digitalWrite(STEP4PIN, (uint8_t) ((SequenceTime > STEP4TIME) ^ !(STEP4ASSERTED == CLOSED)));

    delay(LOOPTIMEINTERVAL);
  }
}
  
