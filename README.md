This sequencer software is a program for use with the ATTiny1616 
microcontroller, copyright 2024 by jeff millar, wa1hco.  The software is 
licensed under the Creative Commons Share and Attribute license.  The development 
environment is VS Code...it just ended up too big for the Arduino IDE.

The software supports one Key input and 4 contact closure outputs.
Key input is an optoisolator diode with voltage and current limiting.
The Key input 
    asserted when 5 to 15 volts is present on the terminal.
    about 2.2K input impedance.
The user can wire the two isolated Key contacts for Volts to Key 
or Ground to key.
Each output is contact closure configurable for a receive state of OPEN or CLOSED. 
When powered off, the contacts are OPEN.

The sequencer uses an ATTiny1616 microcontroller which uses a single pin UPDI
programming interface.

Other features of the software
* optional USB serial communication with a computer
* USB interface for programming or comms based on jumper
* ability to key using RTS, RTS UP requests Tx
* ability to read state of key using CTS, CTS UP indicates ready to modulate
* user configurable tx delay and rx delay times for each step
* user configurable transmit timeout

Internal Key variable is the OR of external Key and CTS, ANDed with optional Key timeout

Here are more details from the software comments
// on Key asserted, transition thru states to Tx, assuming key remain asserted
//   state S1T, assert relay 1, wait relay 1 assert time; usually antenna transfer relay
//   state S2T, assert relay 2, wait relay 2 assert time; usually transverter relays
//   state S3T, assert relay 3, wait relay 3 assert time; usually (something)
//   state S4T, assert relay 4, wait relay 4 assert time; transmitter
//   state Tx, transmit ready, assert CTS
//   
// on key released, transition thru states to Rx, assuming key remains released
//   State S4R, release relay 4, release CTS, wait relay 4 release time
//   State S3R, release relay 3, wait relay 3 release time
//   State S2R, release relay 2, wait relay 2 release time
//   State S1R, release relay 1, wait relay 1 release time 
//   State Rx

// on key released during transition to transmit
//   release relay immediately
//   wait relay release time
//   transition to next release step

// on key asserted during transition to receive
//   assert relay immediately
//   wait relay asert time
//   enter transition to next transmit step

// state machine has 
//   Rx state holds as long as key released
//   Tx state holds as long as key asserted
//   Key optionally has a Timeout
//   4 Tx timed states for relays asserted
//   4 Rx timed states for relays released
//   Tx states exit to corresponding Rx state if key released
//   Rx states ex it to corresponding Tx State if key asserted
//   Timed states set timers on transition into that state
//   Timer values set based on relay close and open times, may be different
//   Timeout causes transition to next state in Rx to Tx or Rx to Tx sequence

// Keying
//   Key inputs are external opto diode or RTS from USB serial port
//   Key inputs are converted High true logic and OR'd together
//   Key is AND'd with TxTimout
//   RX state will reset TxTimeout if key signal and RTS both released