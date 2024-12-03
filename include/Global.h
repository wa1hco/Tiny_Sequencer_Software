#ifndef GLOBAL_H
#define GLOBAL_H

// declare the global variables
extern sConfig_t GlobalConf;
extern char Msg[80];

#ifdef DEBUG
extern unsigned long ISR_Time;
extern unsigned long Min_ISR_Time;
extern unsigned long Max_ISR_Time;
#endif

#endif