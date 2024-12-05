Loop State Machine

Running the sequencer state machine from 10 msec interval timed interrupts
results in latency of 5 msec average and 10 msec peak.  Whole intervals are
added when transitioning from one state to another.

Run the sequencer state machine in the loop rather than triggered by interrupts.
On entry branch on state.  Each state checks the Key_input, RTS, and time.

Time delay is implemented by adding delay to current millis() and then watching
for time to exceed it.

Timing is disrupted if UserInterface() is active, but that should not be the 
case when actually using sequencer
