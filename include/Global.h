#ifndef GLOBAL_H
#define GLOBAL_H

// declare the global variables
extern sConfig_t GlobalConf;
extern char Msg[80];

// State machine timing variables
extern unsigned long TimeNow;
extern unsigned long TimeStart;
extern unsigned int  TimeElapsed;
extern      uint8_t  TimeDelay;

#endif