This sequencer software is an arduino sketch for use with the ATTiny814 microcontroller.
The software is copyright 2024 by jeff millar, wa1hco.  The software is licensed under the GPL V3

The software supports one Key input and 4 contact closure outputs.
The input is configurable for either High or Low for key asserted.
Each output is contact closure configurable for normally CLOSED or OPEN state when not keyed. But when powered off, the contacts are OPEN.
When Key is asserted, a time value start incrementing, as time thresholds are met, the output contacts are asserted.
when Key is released, the time value starts decrementing, as the thresholds are met, the output contacts are de-asserted.
The sequence time counter usually starts at zero and is limited to the maximum time the user configured.
If a key sequence is ended early, the timer counts down from whatever value was reached.
Other features of the software
* option USB serial communication with a computer
* optional ability to key using RTS
* optional ability to read state of key using CTS
