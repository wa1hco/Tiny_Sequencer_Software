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

enum UserConfigState {
  top,          // display top info
   cmd,         // display command info
    step,       // s {1 2 3 4}
      stepT,    // tx {0 to 255 msec}
      stepR,    // rx {0 to 255 msec} 
      stepC,    // contacts closed on tx
      stepO,    // contacts open on tx
    rts, 
      rtsE,     // RTS enabled, Tx on RTS UP
      rtsD,     // RTS disabled
    cts,  
      ctsE,     // CTS enabled, UP if ready to modulate
      ctsD,     // CTS disabled
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
  if (prevUCS != UCS) { // global variables, if first pass, read next token 
    Token = strtok(NULL, " ");  
    if (Token != NULL) {  // if token, return it
      char Msg[40];
      snprintf(Msg, 40, "GetNextToken: token from buffer, len -%d- -%s-", strlen(Token), Token[0]);
      Serial.println(Msg);
      return Token;
    } else {             // no token, prompt
      Serial.println("GetNextToken: strtok(NULL) returns NULL");
      Serial.println(Prompt);
      return NULL; // Caller resposible for reading new line
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

void UserConfig(sConfig_t Config) {

  #define OPEN   0
  #define CLOSED 1

  prevUCS = UCS;
  UCS = nextUCS;  

  // these variable get passed from state call to state call
  static char    FirstChar;
  static uint8_t StepNum;
  static int     Step_msec;
  static uint8_t StepForm;
  static int     Len;

  if (UCS == top) {
    Serial.println("UserConfig: top");
    PrintConfig(Config);
    nextUCS = cmd;
    return;      // skip the rest of the UCS tests
  }
  // read user command as a String, convert to null terminated C string
  if (UCS == cmd) {
    if (prevUCS != UCS) { // global variables, if first pass, read next token 
      Serial.println("Which setting? Step, RTS, CTS, Timeout, Dump");
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

    Serial.print("UserConfig: User entered -");
    Serial.print(Line);
    Serial.println("-");

    char * Token = strtok(Line, " ");
    FirstChar = tolower(Token[0]);

    Serial.print("UserConfig: first token, len ");
    Serial.print(strlen(Token));
    Serial.print("first char -");
    Serial.print(Token[0]);
    Serial.println("-");

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

  // on entering this state Line currently in strtok may contain a 2nd token
  // or may have to prompt for a new line 
  // on first entry, read the next token from strtok()
  // if no token on user line, or the wrong token, emit prompt for step number
  if (UCS == step) {
    char * Token = GetNextToken("Step # {Tx msec, Rx msec, Tx {Open, Closed}");
    if (Token == NULL) {  // no token yet, return and come back thru GetNextToken
        return;
    }
    {
      char Msg[40];
      snprintf(Msg, 40, "UserInterface: UCS == step, Token len -%d- -%s-", strlen(Token), Token[0]);
      Serial.println(Msg);
    }
    StepNum = (uint8_t) Token[0] - (uint8_t) '0';
    if ((StepNum == 1) | 
        (StepNum == 2) |
        (StepNum == 3) |
        (StepNum == 4) ) {
      char Msg[40];
      snprintf(Msg, 40, "UserInterface: Valid StepNum -%d-", StepNum);
      Serial.println(Msg);
    } else {
        char Msg[40];
        snprintf(Msg, 40, "UserInterface, Invalid Seq #: -%s- -%d-", Token, StepNum);
        Serial.println(Msg);
        nextUCS = cmd;  // error bailout
        return;
    } 
    
    char Prompt[60];
    snprintf(Prompt, 60, "Step %d: enter Tx msec, Rx msec, Tx {Open, Closed}", StepNum);
    Token = GetNextToken(Prompt);

    switch (tolower(Token[0])) {
      case 't':  // Tx delay time
        nextUCS = stepT;
        break;
      case 'r':  // Rx delay time
        Token = GetNextToken("msec");
        Step_msec = atoi(Token);
        nextUCS = cmd;
        break;
      case 'o':  // Open
        nextUCS = stepO;
        StepForm = OPEN;
        break;
      case 'c':  // Closed
        StepForm = CLOSED;
        nextUCS = stepC;
        break;
      default:   // error
        nextUCS = top;
        break;
    } // switch on step character
  } // if step

  if (UCS == stepT) { // tx delay time
    char Msg[40];
    snprintf(Msg, 40, "Step %d Tx delay: enter time (msec): ", StepNum);
    char * Token = GetNextToken(Msg);
    if (Token == NULL) {
      return;
    } else {
      Step_msec = atoi(Token);
      Config.Step[StepNum].Tx_msec = (uint8_t) atoi(Token);
      nextUCS = cmd;
    }
  }
  if (UCS == stepR) {  // Rx delay time
    char Msg[40];
    snprintf(Msg, 40, "Step %d Rx delay: enter time (msec): ", StepNum);
    char * Token = GetNextToken(Msg);
    if (Token == NULL) {
      return;
    } else {
      Step_msec = atoi(Token);
      Config.Step[StepNum].Rx_msec = (uint8_t) atoi(Token);
      nextUCS = cmd;
    }
  }
  if (UCS == stepO) {
    char Msg[40];
    snprintf(Msg, 40, "Step %d Open", StepNum);
    Config.Step[StepNum].TxPolarity = OPEN;
    nextUCS = cmd;
  }
  if (UCS == stepC) { 
    char Msg[40];
    snprintf(Msg, 40, "Step %d Closed", StepNum);
    Config.Step[StepNum].TxPolarity = CLOSED;
    nextUCS = cmd;
  }

  if (UCS == rts) {;
    char * Token = GetNextToken("RTS {Enable, Disable");
    if (Token == NULL) {
      return;
    } else {
      char FirstChar = tolower(Token[0]);
      if (FirstChar == 'e'){
        Serial.println("RTS enabled");
        Config.RTS.Enable = true;
      } 
      if (FirstChar == 'd') {
        Serial.println("RTS Disabled");
        Config.RTS.Enable = false;
      }
      nextUCS = cmd;
    }
  }

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
  }
  if (UCS == timeout) {
    char * Token = GetNextToken("Enter Timeout in seconds");
    if (Token == NULL) {
      return;
    } else {
      int Timeout = atoi(Token);
      Config.Timer.Time = Timeout;
    }
    nextUCS = cmd;
  }
  if (UCS == dump) {
    Serial.println("UserInterface: UCS = dump");
    PrintConfig(Config);
    nextUCS = cmd;
  }

  if (UCS == initialize) {
    Serial.println("UserInterface: UCS = initialize");
    sConfig_t Config = InitDefaultConfig();
    PutConfig(0, Config);
    nextUCS = cmd;
  }

  if (UCS == help) {
    Serial.println("UserInterface: UCS = help");
    nextUCS = cmd;
  }

  if (UCS == err) {
    Serial.println("UserInterface: User input error");
    nextUCS = cmd;
  }

  // The user may have made changes to the config
  PutConfig(0, Config);
  return;
} // UserConfig() 