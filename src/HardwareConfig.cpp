
#include "HardwareConfig.h"

void InitPins(void) {
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
}