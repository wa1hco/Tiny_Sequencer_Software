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
    rts,          // wait for {enable, disable}
    cts,          // wait for {enable, disable}
    timeout,      // wait for Time, seconds 0 means disabled
    display,      // PrintConfig(), go to userCmd
    initialize,   // InitDefaultConfig()
    prom,         // get the configuration from EEPROM, check CRC
    help,         // PrintHelp()
    err           // user input not understood, go to top
};

// convienent char arrays for debug messages
const char *userStateName[][15] = {"top", 
                                     "userCmd", 
                                       "stepNum", 
                                         "stepArg", 
                                           "Tx_msec", 
                                           "Rx_msec", 
                                       "rts", 
                                       "cts", 
                                       "timeout", 
                                       "display", 
                                       "init", 
                                       "prom",
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
    // first pass, Token not nulls
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
  snprintf(Msg, 80, "User entered '%s', %d char", Line, reader.len());
  Serial.println(Msg);

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
  long lStepNum = strtol(Token, &endptr, 10);

  if (errno == ERANGE) {
    Serial.println("GetStepNum: value out of range");
    return -1;
  } else if (endptr == Token) {
    Serial.println("GetStepNum: (endptr == Token), no conversion");
    return -1;
  } else if (*endptr != '\0') {
    snprintf(Msg, 80, "GetStepNum: Token %x, '%s', endptr %x, '%s', ", Token, endptr, endptr);
    Serial.println(Msg);
    return -1;
  } else {
    return static_cast <int8_t> (lStepNum);
  }
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

  switch (UCS) {

  case top: // nextUCS initialized to top, then shifted to UCS
    Serial.println("UserIntf: switch(UCS) top");
    PrintConfig(Config);
    nextUCS = userCmd;
    break;

  case userCmd: // wait for user command, next state based on first letter
    Token = GetNextToken("Command list: Step, RTS, CTS, Timeout, Display, Prom, Init, Help");
    if (Token == NULL) {
      break;
    }
    userCmdChar = (char) tolower(Token[0]);

    switch (userCmdChar) { // switch on first character of first token
    case 's':
      nextUCS = stepNum; // wait for number {1, 2, 3, 4}
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
      Config = InitDefaultConfig();
      nextUCS = top;
      break;
    case 'p':            // read from eeprom and display
      {  // needed because creating variable
        Serial.println("Get config from EEPROM, check CRC, dispaly");
        Config = GetConfig(0);
        uint16_t CRC = CalcCRC(Config);
        if (Config.CRC16 != CRC) {
          snprintf(Msg, 80, "UserInterface: bad CRC, EEPROM %x, Calc %x", Config.CRC16, CRC);
          Serial.println(Msg);
        }
        PrintConfig(Config);
        nextUCS = userCmd;
        break;
      }
    case 'h': 
      nextUCS = help;
      break;
    default:
      Serial.println("UserInterface: switch(userCmd) default, invalid command");
      nextUCS = top;

    } // switch (userCmdChar)

    // nextUCS has been set from user command character
    break; // switch(UCS) case userCmd:

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
      nextUCS = userCmd;
    }
    snprintf(Msg, 80, "UserInterface: case cts: after switch(Token[0])");
    Serial.println(Msg);
    break; // case cts:

  case timeout: // wait for timeout in seconds
    Token = GetNextToken("Enter Timeout in seconds");
    if (Token == NULL) {
      break;  // case timeout, stay in this state
    } else {
      unsigned long ulTimeout = strtoul(Token, &endptr, 10);
      if (endptr == Token) {
        snprintf(Msg, 80, "UserInterface: user entry -%s- not an (unsigned long)", Token);
        Serial.println(Msg);
        nextUCS = userCmd;
        break; // case timeout, start command over
      } else {
        Config.Timer.Time = (unsigned int) ulTimeout;
        nextUCS = userCmd;
        break;
      }
    }
    Serial.println("UserInterface: case timeout; after if(Token), should not get here;");
    nextUCS = userCmd;
    break; // switch(UCS) case timeout:

  case display:
    Serial.println("Working Config");
    PrintConfig(Config);
    nextUCS = userCmd;
    break;

  case initialize:
    nextUCS = top;
    break; // switch(UCS) case initialize:

  case help:
    Serial.println("This is help of the user interface.");
    Serial.println("The user enters characters and numbers, MCU does not echo them");
    Serial.println("Input can be one token at a time or all the tokens for a command");
    Serial.println("Top level: Step, RTS, CTS, Timeout, Display, Initialize, Help");
    Serial.println("Step {Step number 0 to 3} {Tx delay, Rx delay, Open on rx, Closed on rx}");
    Serial.println("   's 0 t 100' step 0 tx delay 100 msec");
    Serial.println("   's 3 o, step 3 Open on Rx");
    Serial.println("RTS {enable, disable}");
    Serial.println("   'r e', RTS enable");
    Serial.println("CTS {enable, disable}");
    Serial.println("Timeout 0 to 255 seconds, Tx timeout in seconds, 0 means disable");
    Serial.println("Display, print current configuration");
    Serial.println("Initialize, reset configuration to software defaults");
    Serial.println("Changes are automatically written to EEPROM");
    Serial.println("Help, print this text");
    nextUCS = userCmd;
    break; // switch(UCS) case help:

  // ******** states that wait for arguments to user commands *********
  case stepNum: // wait for sequencer step number
    // called after 's' entered by user
    // read step number
    Token = GetNextToken("GetStepState: enter sequence step number");
    if (Token == NULL) {
      break;
    }  else { // enclosed because new variable declared
      int8_t StepNumTmp = GetStepNum(Token);
      if (StepNumTmp < 0) {
        Serial.println("UserInterface: StepNumTmp < 0, try again");
        nextUCS = userCmd;
        break;
      }
      StepNum = (uint8_t) StepNumTmp;
      if ((StepNum >= 0) & (StepNum <= 3)) {  //stepnum ranges from 0 to 3
        nextUCS = stepArg;
        break; // StepNum ok
      } else {
        snprintf(Msg, 80, "UserInterface: StepNum %d, out of range", StepNum);
        nextUCS = userCmd;
        break;
      }
    } // if (Token)

  case stepArg: // wait for arguments to step command
    {
      Token = GetNextToken("stepArg: {Tx msec, Rx msec, Open or Closed on rx}");
      if (Token == NULL) {
        break;
      } else { // enclosed because variable declared
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
            break;

        } // switch (ArgChar)
      } // if (Token)
    }

  case Rx_msec: // wait for Tx or Rx delay msec, 0 to 255
    Token = GetNextToken("enter Rx_msec, 0 to 255");
    if (Token == NULL) {
      break;
    } else { // enclosed because variable declared
      lmsec = strtol(Token, &endptr, 10);
      if (endptr == Token) {
        Serial.print("UserInterface: strtol(Tx_msec) failed , try again");
        break;
      }
      if ((lmsec > 255) | (lmsec < 0)) {
        Serial.println("UserInterface: Sequence delay msec out of range, try again");
        break;
      }
      snprintf(Msg, 80, "UserInterface: case Rx_msec StepNum %d", StepNum);
      Serial.println(Msg);
      Config.Step[StepNum].Rx_msec = (uint8_t) lmsec;
      nextUCS = userCmd;
      break; // case Rx_msec:
    } // if (Token)
    
  case Tx_msec: // wait for Tx or Rx delay msec, 0 to 255
    Token = GetNextToken("enter Tx msec, 0 to 255}");
    if (Token == NULL) {
      break;
    } else { // enclosed because variable declared
      lmsec = strtol(Token, &endptr, 10);
      if (endptr == Token) {
        Serial.print("UserInterface: strtol(Tx_msec) failed , try again");
        break;
      }
      if ((lmsec > 255) | (lmsec < 0)) {
        Serial.println("UserInterface: Sequence delay msec out of range, try again");
        break;
      }
      snprintf(Msg, 80, "UserInterface: case Rx_msec StepNum %d", StepNum);
      Serial.println(Msg);
      Config.Step[StepNum].Tx_msec = (uint8_t) lmsec;
      nextUCS = userCmd;
      break; // case Tx_msec:
    } // if (Token)

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