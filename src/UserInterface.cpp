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
#include "SequencerStateMachine.h"
#include "Global.h"
#include <SerialReadLine.h>
#include <stdlib.h>
#include <errno.h>
#include <EEPROM.h>
#include <CRC.h>

// User command states
enum UserConfigState {
  top,            // display top info, go to cmd
   cmd,           // wait for command {s, r, c, t, d i, h}
    stepIdx,      // wait for number {1, 2, 3, 4}
      stepArg,    // wait for step command {t, r, o, c}
        msec,     // wait for msec
    rts,          // wait for {enable, disable}
    cts,          // wait for {enable, disable}
    timeout,      // wait for Time, seconds 0 means disabled
    display,      // PrintConfig(), go to cmd
    Init,         // InitDefaultConfig(), needs whole token
    Boot ,        // call software reset, need whole token
    help,         // PrintHelp()
    err           // user input not understood, go to top
};

// convienent char arrays for debug messages
const char *userStateName[][15] = {"top", 
                                     "cmd", 
                                       "stepIdx", 
                                         "stepArg", 
                                           "msec", 
                                       "rts", 
                                       "cts", 
                                       "timeout", 
                                       "display", 
                                       "Init", 
                                       "Boot",
                                       "help", 
                                       "err"};

// private variable for UserConfig
static UserConfigState prevUCS = err;
static UserConfigState UCS = err;
static UserConfigState nextUCS = top;

#define LINELEN 40
char Line[LINELEN + 1];   // Line of user text needs to be available to multiple functions

void PrintHelp() {
  Serial.println("This is help for the user interface.");
  Serial.println("The user enters command and parameters, MCU echos after end of line");
  Serial.println("Input can be one token at a time or all the tokens for a command");
  Serial.println("Top level: 'S'tep, 'R'TS, 'C'TS, 'T'imeout, 'D'isplay, 'P'rom, 'I'nitialize, 'H'elp");
  Serial.println("Step {Step number 0 to 3} {'T'x delay, 'R'x delay, 'O'pen on rx, 'C'losed on rx}");
  Serial.println("RTS {'E'nable, 'D'isable}");
  Serial.println("CTS {'E'nable, 'D'isable}");
  Serial.println("Timeout 0 to 255 seconds, Tx timeout, 0 means disable");
  Serial.println("Display, print working configuration");
  Serial.println("'Init', spelled out, initialize configuration to programmed defaults");
  Serial.println("Help, print this text");
  Serial.println("Changes are automatically written to EEPROM");
  Serial.println("'Boot' command, spelled out, simulates power cycle");
  Serial.println("Examples...");
  Serial.println("   's 0 t 100' step 0 tx delay 100 msec");
  Serial.println("   'step 0 tx 100' step 0 tx delay 100 msec, long form");
  Serial.println("   's 3 o, step 3 Open on Rx");
  Serial.println("   'r e', RTS enable");
  Serial.println("   't 120', tx timeout 120 seconds");
  Serial.println("   't 1', tx timeout disabled");
  Serial.println("   'd', display configuration");
  Serial.println("   'Init', initialize to programmed defaults, needs whole command");
  Serial.println("   'Boot', reboot using software reset, needs whole command");
}

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
    // first pass, Token not nulls
    return Token;
  }
  // after first pass
  reader.poll(); // Since no token, wait for line
  if (!reader.available()) {
    return NULL;
  }

  uint8_t LineLen = reader.len();
  if (LineLen > LINELEN) { 
    char LineFlush[reader.len()];
    reader.read(LineFlush);
    Serial.println("GetNextToken: error, user input too long for Line[40]");
    return NULL;
  }

  reader.read(Line);  // copy into local buffer
  // TODO handle cr and lf in user input
  snprintf(Msg, 80, "User entered '%s'", Line);
  Serial.println(Msg);

  Token = strtok(Line, " ");  // update strtok buffer with new line
  // Line available, may have a token
  //snprintf(Msg, 80, "GetNextToken: token from Line len -%d- -%s-", strlen(Token), Token);
  //Serial.println(Msg);
  return Token;  
} // GetNextToken

// get the step index from 0 to 3
// returns 0 to 3 or -1 if error
int8_t GetStepIdx(char * Token) {
  char * endptr;
  long lStepIdx = strtol(Token, &endptr, 10);

  if (errno == ERANGE) {
    Serial.println("GetStepIdx: strtol() ERANGE");
    return -1;
  } else if (endptr == Token) {
    Serial.println("GetStepIdx: (endptr == Token), no conversion");
    return -1;
  } else if (*endptr != '\0') {
    snprintf(Msg, 80, "GetStepIdx: Token %x, '%s', endptr %x, '%s', ", Token, endptr, endptr);
    Serial.println(Msg);
    return -1;
  } else if ((lStepIdx < 0) | (lStepIdx > 3)) {
    return -1;
  } else {
    return static_cast <int8_t> (lStepIdx);
  }
}

#define OPEN   LOW    // map Digital pin to Sequencer contact closure 
#define CLOSED HIGH

void UserConfig(sConfig_t *pConfig) {
  sConfig_t Config = *pConfig;  // Make a local copy of the config
  static char * Token;
  char * endptr;
  long lmsec;

  // these variable get passed from state call to state call
  static uint8_t StepIdx;  // step command index number, {0 to 3}
  static char    StepArg;  // step command argument {Tx, Rx, Open, Closed}
  static char    CmdChar;  // used for token processing

  prevUCS = UCS;
  UCS = nextUCS;  

  switch (UCS) {

  case top: // nextUCS initialized to top, then shifted to UCS
    Serial.println("UserIntf: switch(UCS) top");
    PrintConfig(Config);
    nextUCS = cmd;
    break;

  case cmd: // wait for user command, next state based on first letter
    Token = GetNextToken("Command list: Step, RTS, CTS, Timeout, Display, Init, Boot, Help");
    if (Token == NULL) {
      break;
    }
    CmdChar = (char) tolower(Token[0]);
    switch (CmdChar) { // switch on first character of first token
      case 's':
        nextUCS = stepIdx; // wait for number {1, 2, 3, 4}
        break;
      case 'r':
        nextUCS = rts;     // wait for {enable, disable}
        break;
      case 'c':
        nextUCS = cts;     // wait for {enable, disable}
        break;
      case 't':
        nextUCS = timeout; // wait for seconds
        break;
      case 'd': 
        nextUCS = display;
        break;
      case 'i':            // reinitialize config
        nextUCS = Init;    // require whole token
        break;
      case 'b':            // reboot as if from power cycle
        nextUCS = Boot;    // require whole token
        break;
      case 'h': 
        nextUCS = help;
        break;
      default:
        Serial.println("UserInterface: switch(cmd) default, invalid command");
        nextUCS = top;
    } // switch (CmdChar)
    break; // switch(UCS) case cmd:

  case rts: // wait for enable or disable
    Token = GetNextToken("RTS, enter {Enable, Disable");
    if (Token == NULL) {
      break;
    } 
    switch (tolower(Token[0])) {
    case 'e':
      //Serial.println("RTS enabled");
      Config.RTSEnable = true;
      nextUCS = cmd;
      break;
    case 'd':
      //Serial.println("RTS Disabled");
      Config.RTSEnable = false;
      nextUCS = cmd;
      break;
    default:
      Serial.println("UserInterface: rts {Enable, Disable} not found");
    } // switch(tolower(Token[0]))
    break; // case rts:
    
  case cts: // wait for enable or disable
    Token = GetNextToken("CTS {Enable, Disable}");
    if (Token == NULL) {
      break;
    }
    switch (tolower(Token[0])) {
    case 'e':
      Serial.println("CTS enabled");
      Config.CTSEnable = true;
      nextUCS = cmd;
      break;
    case 'd':
      Serial.println("CTS Disabled");
      Config.CTSEnable = false;
      nextUCS = cmd;
      break;
    default:
      Serial.println("UserInterface: rts {Enable, Disable} not found");
      nextUCS = cmd;
    }
    #ifdef DEBUG
    snprintf(Msg, 80, "UserInterface: case cts: after switch(Token[0])");
    Serial.println(Msg);
    #endif
    break; // case cts:

  case timeout: // wait for timeout in seconds
    Token = GetNextToken("Enter Timeout in seconds");
    if (Token == NULL) {
      break;  // case timeout, stay in this state
    } else {
      unsigned long ulTimeout = strtoul(Token, &endptr, 10);
      if (endptr == Token) {
        #ifdef DEBUG
        snprintf(Msg, 80, "UserInterface: user entry -%s- not an (unsigned long)", Token);
        Serial.println(Msg);
        #endif
        nextUCS = cmd;
        break; // case timeout, start command over
      } else {
        Config.Timeout = (uint16_t) ulTimeout;
        nextUCS = cmd;
        break;
      }
    }
    Serial.println("UserInterface: case timeout; after if(Token), should not get here;");
    nextUCS = cmd;
    break; // switch(UCS) case timeout:

  case display:
    PrintConfig(Config);
    nextUCS = cmd;
    break;

  case Init: // initialize config from EEPROM
    if (strcmp(Token, "Init")== 0) { // require whole token
      Config = InitDefaultConfig();
      nextUCS = cmd; 
      break;
    }  
    Serial.print("UserInterface: 'Init' command entry error -");
    Serial.print(Token);
    Serial.println("-");
    nextUCS = cmd;
    break;

  case Boot: // reboot as if from power up
    if (strcmp(Token, "Boot") == 0) { // require whole token
      _PROTECTED_WRITE(RSTCTRL.SWRR,1); 
      nextUCS = cmd; // should not get here
      break;
    }  
    Serial.print("UserInterface: 'Boot' command entry error -");
    Serial.print(Token);
    Serial.println("-");
    nextUCS = cmd;
    break;

  case help:
    PrintHelp();
    nextUCS = cmd;
    break; // switch(UCS) case help:

  // ******** states that wait for arguments to user commands *********
  case stepIdx: // wait for sequencer step number
    // read step number
    Token = GetNextToken("GetStepState: enter sequence step number");
    if (Token == NULL) {
      break;
    }  
    { // enclosed because new variable declared in this case:
      int8_t StepIdxTmp = GetStepIdx(Token);
      if (StepIdxTmp < 0) { // error detected, not value in range 0:3
        Serial.println("UserInterface: invalid StepIdx");
        nextUCS = cmd;
        break;
      }
      StepIdx = (uint8_t) StepIdxTmp;
      nextUCS = stepArg;
      break; // StepIdx ok
    }

  case stepArg: // wait for arguments to step command
    Token = GetNextToken("stepArg: {Tx msec, Rx msec, Open or Closed on rx}");
    if (Token == NULL) {
      break;
    } 
    { // enclosed because variable declared
      StepArg = (char) tolower(Token[0]);
      switch (StepArg) {
        case 't':
          nextUCS = msec;
          break;
        case 'r':
          nextUCS = msec;
          break;
        case 'o':
          //Serial.println("UserInterface: StepState Open on RX");
          Config.Step[StepIdx].RxPolarity = OPEN;
          nextUCS = cmd; // step # open, command complete
          break;
        case 'c':
          //Serial.println("UserInterface: StepState Closed on RX");
          Config.Step[StepIdx].RxPolarity = CLOSED;
          nextUCS = cmd; // step # closed, command complete
          break;
        default:
          Serial.println("UserInterface: invalid input to step command");
          break;
      } // switch (ArgChar)
    } 

  case msec: // wait for Tx or Rx delay msec, 0 to 255
    Token = GetNextToken("enter msec, 0 to 255");
    if (Token == NULL) {
      break;
    }  
    // Token should contain msec
    lmsec = strtol(Token, &endptr, 10); 
    if (endptr == Token) {
      Serial.print("UserInterface: strtol(msec) failed, try again");
      break;
    }
    if ((lmsec > 255) | (lmsec < 0)) {
      Serial.println("UserInterface: Sequence delay msec out of range, try again");
      break;
    }
    // valid lmsec, configure based on StepArg
    switch (StepArg){
      case 't':
        Config.Step[StepIdx].Tx_msec = (uint8_t) lmsec;
        nextUCS = cmd;
        break;
      case 'r':
        Config.Step[StepIdx].Rx_msec = (uint8_t) lmsec;
        nextUCS = cmd;
        break;
      default:
        Serial.println("UserInterface: switch(StepArg), invalid StepArg");
    } // switch(StepArg)
    break; // case msec:

  default:
    Serial.println("UserInterface: switch(UCS) reached default:");
    nextUCS = top;

  } // switch(UCS)

  // The user may have made changes to the config
  Config.CRC16 = CalcCRC(Config);  // update CRC16
  PutConfig(0, Config);  // write changed bytes of config to EEPROM
  *pConfig = Config;  // Copy the new config to global config

  return;
} // UserConfig() 