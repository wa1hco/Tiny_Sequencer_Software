

#ifndef SEQUENCERSTATEMACHINE_H
#define SEQUENCERSTATEMACHINE_H

#include "Config.h"

// Public functions
void SequencerInit();        //called from setup()
void SequencerSetOutputs();  // called after config change to outputs
void Sequencer();            // called from loop()

#endif


