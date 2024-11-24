
#include "HardwareConfig.h"
#include "Config.h"

void InitPins(sConfig_t Config) {
  // Hardware configuration
  pinMode(KEYPIN, INPUT_PULLUP); 
  pinMode(RTSPIN, INPUT_PULLUP);

  pinMode(LEDPIN, OUTPUT);
  pinMode(CTSPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  digitalWrite(CTSPIN, LOW);

  pinMode(S1T_PIN, OUTPUT);
  pinMode(S2T_PIN, OUTPUT);
  pinMode(S3T_PIN, OUTPUT);
  pinMode(S4T_PIN, OUTPUT);

  pinMode(XTRA1PIN, OUTPUT);
  pinMode(XTRA2PIN, OUTPUT);
  pinMode(XTRA3PIN, OUTPUT);
  pinMode(XTRA4PIN, OUTPUT);
  pinMode(XTRA5PIN, OUTPUT);
  pinMode(XTRA6PIN, OUTPUT);

  // Define the form (NO/NC) of step relays wired into the board
  digitalWrite(S1T_PIN, (uint8_t) Config.Step[0].RxPolarity); // config as receive mode 
  digitalWrite(S2T_PIN, (uint8_t) Config.Step[1].RxPolarity); // config as receive mode 
  digitalWrite(S3T_PIN, (uint8_t) Config.Step[2].RxPolarity); // config as receive mode 
  digitalWrite(S4T_PIN, (uint8_t) Config.Step[3].RxPolarity); // config as receive mode 


}