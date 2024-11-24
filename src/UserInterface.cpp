// User Interface
// Called from loop(), nothing else needs loop()
// Sequencer function runs from interrupt
// user enters a line of characters terminated by \r
// build line character array
// line contain command and variable number of parameters
// Extract the command token, first token from line
// Set next state based on command first character {s, k, r, c, t, d}
// each command state processes next token or prompts and waits for more serial input

#include "UserInterface.h"
#include "HardwareConfig.h"
#include "SoftwareConfig.h"
#include <SerialReadLine.h>
#include <stdlib.h>
#include <EEPROM.h>
#include <CRC.h>

enum UserConfigState {
  top,          // display top info
   cmd,         // display command info
    step,       // s {1 2 3 4}
      stepT,    // tx {0 to 255 msec}
      stepR,    // rx {0 to 255 msec} 
      stepC,    // contacts closed on tx
      stepO,    // contacts open on tx
    rts,        // {enable, disable}
    cts,        // {enable, disable}
    timeout,    // Time, seconds 0 means disabled
    dump,       // PrintConfig()
    initialize, // InitDefaultConfig()
    help,       // display user interface instructions
    err         // if user input not understood
};

// private variable for UserConfig
static UserConfigState UCS = top;
static UserConfigState prevUCS;
static UserConfigState nextUCS;

#define LINELEN 40
char Line[LINELEN + 1];   // Line of user text needs to be available to multiple functions

void newUserState(uint8_t prevUCS, uint8_t UCS, const char * Prompt) {
  if (prevUCS != UCS) {
    Serial.println(Prompt);
  }
}

// Called after each case statement for user state machine
// token exists in strtok() buffer, return point to it
// if strtok() retuns NULL, and first pass emit prompt, 
//    wait for line from serial-readline
char * GetNextToken(const char * Prompt) {
  char * Token;
  // on the first pass, try to get token from strtok() buffer
  if (prevUCS != UCS) { // UserInterface variables, if first pass, read next token 
    Token = strtok(NULL, " ");  
    if (Token == NULL) {  // if token, return it
      Serial.println(Prompt);
      return NULL; // Caller resposible for reading new line
    } else {             // no token, prompt
      return Token;
    } // if on token from strtok() buffer
  } // if first pass

  // on 2nd and following entries,  check for user entry
  reader.poll();
  if (!reader.available()) {
    return NULL;
  }
  if (reader.len() < LINELEN) { 
    reader.read(Line);  // copy into local buffer
  } else { // too long for Line
    char LineFlush[reader.len()];
    reader.read(LineFlush);
    Serial.println("UserInterface: error, user input too long for Line[40]");
    return NULL;
  }
  Token = strtok(Line, " ");
  {
    char Msg[40];
    snprintf(Msg, 40, "GetNextToken: token from Line len -%d- -%s-", strlen(Token), Token[0]);
    Serial.println(Msg);
  }
  return Token;  
} // GetNextToken

// user configureation setting state machine
// called each pass througn loop()
// command tree
//   {s, k, r, c, t, d}              // Step, RTS, CTS, Timer, Dump  
//     s {1, 2, 3, 4} {t, r, {o, c}} // Step # Tx delay, Rx delay, Rx {Open, Closed}
//       t msec                      //        Tx delay 0 to 255 msec
//       r msec                      //        Rx delay 0 to 255 msec
//       p {o, c}                    //        Polarity on Tx {Open, Closed}
//     r {e, d}                      // RTS,   Up on Tx
//       e                           //        Enable
//       d                           //        Disaable
//     c {e, d}                      // CTS,   Up on Tx
//       e                           //        Enable
//       d                           //        Disable
//     t {time}                      // Time   seconds, 0 means no timeout
//     d                             // Dump   PrintConfig()
//     i                             // Init   InitDefaultConfig()
//     h                             // help   display instructions

// UserConfg will update the EEPROM after each user input

void UserConfig(sConfig_t *pConfig) {

  // define the MCU pin output level for Open on Rx or Closed on Rx
  sConfig_t Config = *pConfig;  // Make a copy of the config
  prevUCS = UCS;
  UCS = nextUCS;  

  // these variable get passed from state call to state call
  static char    FirstChar;
  static uint8_t StepNum;
  static int     Step_msec;
  static uint8_t StepForm;
  static int     Len;

  // UCS initialized to top when declared
  if (UCS == top) {
    Serial.println("UserConfig: top");
  
    PrintConfig(Config);
    nextUCS = cmd;
    return;      // skip the rest of the UCS tests
  }
  // read user command as a String, convert to null terminated C string
  if (UCS == cmd) {
    if (prevUCS != UCS) { // global variables, if first pass, read next token 
      Serial.println("Select command: Step, RTS, CTS, Timeout, Dump, Init, Help");
    }
    // check for a command line from user, return to loop() if no line, will come back
    reader.poll();
    if (!reader.available()) {
      return;
    }
    if (reader.len() < LINELEN) {
      reader.read(Line);
    } else {
      char LineFlush[reader.len()];
      reader.read(LineFlush);
      return;
    }

    char * Token = strtok(Line, " ");
    FirstChar = tolower(Token[0]);

    // branch on first character of first token
    if (FirstChar == 's') nextUCS = step;
    if (FirstChar == 'r') nextUCS = rts;
    if (FirstChar == 'c') nextUCS = cts;
    if (FirstChar == 't') nextUCS = timeout;
    if (FirstChar == 'd') nextUCS = dump;
    if (FirstChar == 'i') nextUCS = initialize;
    if (FirstChar == 'h') nextUCS = help;
    return;  // skip rest of UCS tests
  } // if UCS == cmd

  // user pressed 's' at the first character
  // run a second level state machine for processing the rest of the command
  if (UCS == step) {
    #define OPEN   0
    #define CLOSED 1
    enum StepState_t {S, StepCmd, Tx, Rx, Open, Closed};
    static StepState_t StepState;
    char * Token;

    if (prevUCS != UCS) {
      StepState = S;
    }

    // S was entered, 2nd level state machine
    // Number 1-4, Tx_msec, Rx_msec, open or closed on Tx
    // UCS remains 's' until command complete
    switch (StepState) {
      case S:
        Token = GetNextToken("Step # {Tx msec, Rx msec, Tx {Open, Closed}");
        if (Token == NULL) {  // no token yet, return and come back thru GetNextToken
            return;
        }
        // TODO check for errors
        StepNum = atoi(Token);
        // if returns 0 maybe user entered 0, maybe input invalid
        if ((StepNum == 0) & (Token[0] != '0')) {
          // user input not a number format
          Serial.println("invalid step number");
          nextUCS = cmd;
          return;
        }
        StepState = StepCmd;
        break;

      case StepCmd:
        Token = GetNextToken("{Tx msec, Rx msec, {Open, Closed} on Rx");
        if (Token == NULL) {  // no token yet, return and come back thru GetNextToken
            return;
        }
        char CmdChar = tolower(Token[0]);
        switch (CmdChar) {
          case 't':
            Token = GetNextToken("Tx msec");
            if (Token == NULL) {  // no token yet, return and come back thru GetNextToken
                return;
            }
            StepNum = atoi(Token);
            if ((StepNum == 0) & (Token[0] != '0')) {
              // user input not a number format
              Serial.println("invalid step number");
              nextUCS = cmd;
              return;
            }
            break;
          case 'r':
            Token = GetNextToken("Rx msec");
            if (Token == NULL) {  // no token yet, return and come back thru GetNextToken
                return;
            }
            break;
          case 'o':
            Serial.println("UserInterface: StepState Open on RX");
            Config.Step[StepNum].RxPolarity = OPEN;
            UCS = cmd; // step # open, command complete
            break;
          case 'c':
            Serial.println("UserInterface: StepState Closed on RX");
            Config.Step[StepNum].RxPolarity = Closed;
            UCS = cmd; // step # closed, command complete
            break;
          default:
            Serial.println("UserInterface: invalid input to step command");
            break;
        }  
      default: 
        Serial.println("UserConfig: StepState default");
        break;
    } // switch StepState
    return;
  } // if step

  if (UCS == rts) {
    char * Token = GetNextToken("RTS {Enable, Disable");
    if (Token == NULL) {
      return;
    } else {
      char FirstChar = tolower(Token[0]);
      if (FirstChar == 'e'){
        //Serial.println("RTS enabled");
        Config.RTS.Enable = true;
      } 
      if (FirstChar == 'd') {
        //Serial.println("RTS Disabled");
        Config.RTS.Enable = false;
      }
      nextUCS = cmd;
    }
  } // if rts

  if (UCS == cts) {;
    char * Token = GetNextToken("CTS {Enable, Disable}");
    if (Token == NULL) {
      return;
    } else {
      char FirstChar = tolower(Token[0]);
      if (FirstChar == 'e'){
        Serial.println("CTS enabled");
        Config.CTS.Enable = true;
      } 
      if (FirstChar == 'd') {
        Serial.println("CTS Disabled");
        Config.CTS.Enable = false;
      }
      nextUCS = cmd;
    }
  } // if cts

  if (UCS == timeout) {
    char * Token = GetNextToken("Enter Timeout in seconds");
    if (Token == NULL) {
      return;
    } else {
      int Timeout = atoi(Token);
      Config.Timer.Time = Timeout;
    }
    nextUCS = cmd;
  } // if timeout

  if (UCS == dump) {
    PrintConfig(Config);
    nextUCS = cmd;
  } // if dump

  if (UCS == initialize) {
    Serial.println("UserInterface: UCS = initialize");
    // reinitialize config
    sConfig_t Config = InitDefaultConfig();
    nextUCS = cmd;
  } // if initialize

  if (UCS == help) {
    Serial.println("UserInterface: UCS = help");
    nextUCS = cmd;
  }  // if help

  if (UCS == err) {
    Serial.println("UserInterface: User input error");
    nextUCS = cmd;
  } // if err

  // The user may have made changes to the config
  Config.CRC16 = CalcCRC(Config);  // update CRC16
  PutConfig(0, Config);  // write changed bytes of config to EEPROM
  *pConfig = Config;  // Copy the new config to global config
  return;
} // UserConfig() 