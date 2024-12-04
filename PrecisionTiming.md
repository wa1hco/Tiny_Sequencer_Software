Precision Timing

Some testing with the state machine running on 10 msec timer interrupts shows
actual delay is about 10 to 15 msec greater than programmed delay.  Also, it
takes up to 10 msec to respond to a Key change event.

This branch intends to change the design to improve the timing
- Key input generates interrupt on any change
- RTS input generates interrupt on any change
- Sequencer delay implemented as a programmed timer interrupt

State processing on entry from the timer interrupt 
- update output
- set the next state to continue the transition
- programs the delay

State processing on entry from the Key interrupt
- check if Key change cancels the state transition
- if cancel
  - update output
  - set next state on opposite transition
  - program the delay

Input Events
- Key_input change or RTS change

Debouncing Key_input and RTS
- multiple interrupts at sub-msec intervals

Sequencer State Machine
- function that operates in interrupt context
- called from Key change, RTS change, Timer interrupt
- interrupts masked while in state machine
- race conditions
  - input event during Timer event processing
  - timer event during input event processing

Rx State
- Wait for key asserted

Tx State
- wait for Key released