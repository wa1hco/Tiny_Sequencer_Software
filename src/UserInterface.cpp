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
#include <errno.h>
#include <EEPROM.h>
#include <CRC.h>

// User command states
enum UserConfigState {
  top,            // display top info, go to userCmd
   userCmd,       // wait for command {s, r, c, t, d i, h}
    stepNum,      // wait for number {1, 2, 3, 4}
      stepArg,    // wait for step command {t, r, o, c}
        Tx_msec,  // wait for msec
        Rx_msec,  // wait for msec
        stepC,    // contacts closed on rx
        stepO,    // contacts open on rx
    rts,          // wait for {enable, disable}
    cts,          // wait for {enable, disable}
    timeout,      // wait for Time, seconds 0 means disabled
    dump,         // PrintConfig(), go to userCmd
    initialize,   // InitDefaultConfig()
    help,         // PrintHelp()
    err           // user input not understood, go to top
};

const char *userStateName[][15] = {"top", 
                                     "userCmd", 
                                       "stepNum", 
                                         "stepArg", 
                                           "Tx_msec", 
                                           "Rx_msec", 
                                           "stepC", 
                                           "stepO", 
                                       "rts", 
                                       "cts", 
                                       "timeout", 
                                       "dump", 
                                       "init", 
                                       "help", 
                                       "err"};

// private variable for UserConfig
static UserConfigState prevUCS = err;
static UserConfigState UCS = err;
static UserConfigState nextUCS = top;
char Msg[80];

#define LINELEN 40
char Line[LINELEN + 1];   // Line of user text needs to be available to multiple functions

// Called after each case statement for user state machine
// token exists in strtok() buffer, return point to it
// if strtok() retuns NULL, and first pass emit prompt, 
//    wait for line from serial-readline
// cases
//  first pass, token null:  prompt, return null
//  first pass, token valid: return Token
//  nth pass, token null:    return null
//  nth pass, Token valid:   return Token
char * GetNextToken(const char * Prompt) {
  char * Token;
  if (prevUCS != UCS) {         // on first pass, try to get next token on current line
    Token = strtok(NULL, " ");  // returns a pointer to internal char array
    if (Token == NULL) {
      Serial.println(Prompt);   // Prompt on first pass
      return NULL;
    }
    // first pass, Token not null
    return Token;
  }
  // after first pass
  reader.poll(); // Since no token, wait for line
  if (!reader.available()) {
    return NULL;
  }

  if (reader.len() > LINELEN) { 
    char LineFlush[reader.len()];
    reader.read(LineFlush);
    Serial.println("GetNextToken: error, user input too long for Line[40]");
    return NULL;
  }
  reader.read(Line);  // copy into local buffer
  Token = strtok(Line, " ");  // update strtok buffer with new line
  // Line available, may have a token
  //snprintf(Msg, 80, "GetNextToken: token from Line len -%d- -%s-", strlen(Token), Token);
  //Serial.println(Msg);
  return Token;  
} // GetNextToken

// get the step number from 1 to 4
// TODO figure out 0 to 3 vs 1 to 4
int8_t GetStepNum(char * Token) {
  char * endptr;
  snprintf(Msg, 80, "GetStepNum: (char * Token) -%s-, Token %x, endptr %x", Token, Token, endptr);
  Serial.println(Msg);
  long lStepNum = strtol(Token, &endptr, 10);
  if (errno == ERANGE) {
    Serial.println("GetStepNum: value out of range");
  } else if (endptr == Token) {
    Serial.println("GetStepNum: (endptr == Token), no conversion performed");
  } else if (*endptr != '\0') {
    Serial.println("GetStepNum: *endptr != NULL, partial conversion");
  } else {
    Serial.print("GetStepNum: converted value ");
    Serial.println(lStepNum);
  }

  if (endptr == Token) {
    Serial.print("GetStepNum: strtol(strStepNum) failed, start over");
    nextUCS = userCmd;
    return -1;
  }
  if ((lStepNum > 4) | (lStepNum < 0)) {
    Serial.println("GetStepNum: Sequence number out of range, start over");
    nextUCS = userCmd;
    return -1;
  }
  uint8_t StepNum = static_cast <int8_t> (lStepNum);
  snprintf(Msg, 80, "GetStepNum: strtol(%s), lStepNum %d, hex %x, StepNum %d, hex %x", Token, lStepNum, lStepNum, StepNum, StepNum);
  Serial.println(Msg);
  snprintf(Msg, 80, "GetStepNum: Token %x, endptr %x", Token, endptr);
  Serial.println(Msg);
  return StepNum;
}

#define OPEN   LOW    // map Digital pin to Sequencer contact closure 
#define CLOSED HIGH

void UserConfig(sConfig_t *pConfig) {
  // define the MCU pin output level for Open on Rx or Closed on Rx
  sConfig_t Config = *pConfig;  // Make a local copy of the config
  char * Token;
  char * endptr;
  long lmsec;
  
  // these variable get passed from state call to state call
  static uint8_t StepNum;
  static char    userCmdChar;  // used for token processing

  prevUCS = UCS;
  UCS = nextUCS;  

  if (prevUCS != UCS) {
      snprintf(Msg, 80, "UserIntf: before switch(UCS), prevUCS %s, UCS %s, nextUCS %s", 
               userStateName[0][prevUCS], userStateName[0][UCS], userStateName[0][nextUCS]);
      Serial.println(Msg);
  }

  switch (UCS) {
   case top: // nextUCS initialized to top, then shifted to UCS
    Serial.println("UserIntf: switch(UCS) top");
    PrintConfig(Config);
    nextUCS = userCmd;
    break;

  case userCmd: // wait for user command, next state based on first letter
    Token = GetNextToken("Select command: Step, RTS, CTS, Timeout, Dump, Init, Help");
    if (Token == NULL) {
      break;
    }
    userCmdChar = (char) tolower(Token[0]);

    snprintf(Msg, 80, "UserIntf: before switch(userCmdChar) -%s-, nextUCS %s", userCmdChar, userStateName[0][nextUCS]);
    Serial.println(Msg);
    
    switch (userCmdChar) { // switch on first character of first token
    case 's':
      Serial.println("UserInterface: command 's'");
      nextUCS = stepNum; // wait for number {1, 2, 3, 4}
      break;
    case 'r':
      Serial.println("UserInterface: command 'r'");
      nextUCS = rts;     // wait for {enable, disable}
      break;
    case 'c':
      Serial.println("UserInterface: command 'c'");
      nextUCS = cts;     // wait for {enable, disable}
      break;
    case 't':
      nextUCS = timeout; // wait for seconds
      break;
    case 'd': 
      Serial.println("UserInterface: command 'd'");
      PrintConfig(Config);
      nextUCS = userCmd;
      break;
    case 'i': 
      Serial.println("UserInterface: UCS = initialize");
      // reinitialize config
      sConfig_t Config = InitDefaultConfig();
      nextUCS = top;
      break;
    case 'h': 
      //PrintHelp();
      nextUCS = userCmd;
      break;
    default:
      Serial.println("UserInterface: userCmdChar not valid");
      nextUCS = top;

    } // switch (userCmdChar)
    // nextUCS has been set from user command character
    snprintf(Msg, 80, "UserIntf: after switch(usrCmdChar) -%s-, UCS %s, nextUCS %s", userCmdChar, userStateName[0][UCS], userStateName[0][nextUCS]);
    Serial.println(Msg);
    break; // case userCmd:

  case rts: // wait for enable or disable
    Token = GetNextToken("RTS, enter {Enable, Disable");
    if (Token == NULL) {
      break;
    } 
    switch (tolower(Token[0])) {
    case 'e':
      //Serial.println("RTS enabled");
      Config.RTS.Enable = true;
      nextUCS = userCmd;
      break;
    case 'd':
      //Serial.println("RTS Disabled");
      Config.RTS.Enable = false;
      nextUCS = userCmd;
      break;
    default:
      Serial.println("UserInterface: rts {Enable, Disable} not found");
    } // switch(tolower(Token[0]))
    break; // case rts:
    
  case stepNum: // wait for sequencer step number
    // called after 's' entered by user
    // read step number
    Token = GetNextToken("GetStepState: enter sequence step number");
    if (Token == NULL) {
      break;
    } 
    int8_t StepNumTmp = GetStepNum(Token);
    if (StepNumTmp < 0) {
      Serial.println("UserInterface: StepNumTmp < 0, try again");
      break;
    }
    uint8_t StepNum = (uint8_t) StepNumTmp;
    snprintf(Msg, 80, "UserInterface: case StepNumState, StepNumTmp %d, StepNum %d", StepNumTmp, StepNum);
    Serial.println(Msg);
    nextUCS = stepArg;
    break; // case stepNum:

  case stepArg: // wait for arguments to step command
    Token = GetNextToken("stepArg: {Tx msec, Rx msec, Open or Closed on rx}");
    if (Token == NULL) {
      break;
    }
    char ArgChar = tolower(Token[0]);
    switch (ArgChar) {
      case 't':
        nextUCS = Tx_msec;
        break;

      case 'r':
        nextUCS = Rx_msec;
        break;

      case 'o':
        Serial.println("UserInterface: StepState Open on RX");
        Config.Step[StepNum].RxPolarity = OPEN;
        nextUCS = userCmd; // step # open, command complete
        break;

      case 'c':
        Serial.println("UserInterface: StepState Closed on RX");
        Config.Step[StepNum].RxPolarity = CLOSED;
        nextUCS = userCmd; // step # closed, command complete
        break;

      default:
        Serial.println("UserInterface: invalid input to step command");

    } // switch (ArgChar)
    Serial.println("UserInterface: after switch(ArgChar)");
    break; // case stepArg

  case Rx_msec: // wait for Tx or Rx delay msec, 0 to 255
    Token = GetNextToken("enter Rx_msec");
    if (Token == NULL) {
      break;
    }
    lmsec = strtol(Token, &endptr, 10);
    if (endptr == Token) {
      Serial.print("UserInterface: strtol(Tx_msec) failed , try again");
      break;
    }
    if ((lmsec > 255) | (lmsec < 0)) {
      Serial.println("UserInterface: Sequence delay msec out of range, try again");
      break;
    }
    Config.Step[StepNum].Rx_msec = (uint8_t) lmsec;
    nextUCS = userCmd;
    break; // case Rx_msec:
    
  case Tx_msec: // wait for Tx or Rx delay msec, 0 to 255
    Token = GetNextToken("enter Tx msec}");
    if (Token == NULL) {
      break;
    }
    lmsec = strtol(Token, &endptr, 10);
    if (endptr == Token) {
      Serial.print("UserInterface: strtol(Tx_msec) failed , try again");
      break;
    }
    if ((lmsec > 255) | (lmsec < 0)) {
      Serial.println("UserInterface: Sequence delay msec out of range, try again");
      break;
    }
    Config.Step[StepNum].Tx_msec = (uint8_t) lmsec;
    nextUCS = userCmd;
    break; // case Tx_msec:
    
  case cts: // wait for enable or disable
    Token = GetNextToken("CTS {Enable, Disable}");
    if (Token == NULL) {
      break;
    }
    switch (tolower(Token[0])) {
    case 'e':
      Serial.println("CTS enabled");
      Config.CTS.Enable = true;
      nextUCS = userCmd;
      break;
    case 'd':
      Serial.println("CTS Disabled");
      Config.CTS.Enable = false;
      nextUCS = userCmd;
      break;
    default:
      Serial.println("UserInterface: rts {Enable, Disable} not found");

    }
    break; // case cts:

  case timeout: // wait for timeout in seconds
    Token = GetNextToken("Enter Timeout in seconds");
    if (Token == NULL) {
      break;
    } else {
      int Timeout = atoi(Token);
      Config.Timer.Time = Timeout;
    }
    nextUCS = userCmd;
    break; // case timeout:

  case help:
    Serial.println("UserInterface: UCS = help");
    nextUCS = userCmd;
    break; // case help:

  case err:
    Serial.println("UserInterface: User input error");
    nextUCS = userCmd;
    break; // case err:

  default:
    Serial.println("UserInterface: switch(UCS) reached default:");
    nextUCS = top;

  } // switch(UCS)

  if (prevUCS != UCS) {
      snprintf(Msg, 80, "UserInterface: end of switch(UCS), prevUCS %s, UCS %s, nextUCS %s", 
                        userStateName[0][prevUCS], userStateName[0][UCS], userStateName[0][nextUCS]);
      Serial.println(Msg);
  }
  // The user may have made changes to the config
  Config.CRC16 = CalcCRC(Config);  // update CRC16
  PutConfig(0, Config);  // write changed bytes of config to EEPROM
  *pConfig = Config;  // Copy the new config to global config
  return;
} // UserConfig() 