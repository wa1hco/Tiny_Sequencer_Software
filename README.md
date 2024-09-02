This sequencer software is an arduino sketch for use with the ATTiny85 microcontroller in the Digispark configuration.
The software is copyright 2024 by jeff millar, wa1hco.  The software is licensed under the GPL V3

The softare supports one Key input and 4 contact closure outputs
The input is configurable for either High or Low for key asserted
Each output is configurable to use when not keyed either normally CLOSED or OPEN state.  But when powered off, the contacts are OPEN
When Key is asserted, a time value start incrementing, as time thresholds are met, the output contacts are asserted
when Key is released, the time value starts decrementing, as the thresholds are met, the output contacts are de-asserted
The sequence time counter usually starts at zero and is limited to the maximum time the user configured
If a key sequence is aborted the timer counts down from whatever value was reached
