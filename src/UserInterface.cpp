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

#define LINELEN 40
char Line[LINELEN + 1];   // Line of user text needs to be available to multiple functions

// parse functions, return false on error, true on Config change
bool StepCmd();             
bool RTScmd();
bool CTScmd();
bool TimeoutCmd();
bool InitCmd(char * Token);
int Get_msec(char * Token); // Supports StepCmd, returns 0:255 msec or -1 for error

// other commands
bool BootCmd();
void UpdateConfig();        // write Config when parse function true
void FlushLines();
void PrintHelp();           // flush anything in the input buffer

#define OPEN   LOW    // map Digital pin to Sequencer contact closure 
#define CLOSED HIGH
#define MAX_TOKENS_PER_LINE 4
static char * Tokens[MAX_TOKENS_PER_LINE];
static bool needCmdPrompt = true;

void UserConfig() {
  static char * Token;
  uint8_t TokenCnt;
  static bool isConfUpdate = false;

  // these variable get passed from state call to state call
  static uint8_t StepIdx;  // step command index number, {0 to 3}
  static char    StepArg;  // step command argument {Tx, Rx, Open, Closed}
  static char    CmdChar;  // used for token processing

  if (needCmdPrompt) {
    PrintConfig(Config);
    FlushLines();
    needCmdPrompt = false;
    snprintf(Msg, 80, "%s", "Command list: Step, RTS, CTS, Timeout, Display, Init, Boot, Help");
    Serial.println(Msg);
  }

  reader.poll();  // update the serial reader status
  if(!reader.available()) { // wait for user to enter a line
    needCmdPrompt = false;
    return;
  }

  if (reader.len() > LINELEN) {
    FlushLines();
    return;
  }
  reader.read(Line);
  if (strlen(Line) == 0) {
    return;
  }

  // try to read all the tokens that might be on line
  // no need to check for Token = NULL at this point
  snprintf(Msg, 80, "User line '%s' len %d, ", Line, strlen(Line));
  Serial.print(Msg);

  Tokens[0] = strtok(Line, " "); // first Token is handled differently
  for(TokenCnt = 1; TokenCnt < MAX_TOKENS_PER_LINE; TokenCnt++) {
    Token = strtok(NULL, " ");
    Tokens[TokenCnt] = Token;
  } 

  Serial.print("Tokens ");
  for (TokenCnt = 0; TokenCnt < MAX_TOKENS_PER_LINE; TokenCnt++) {
    Serial.print("'");
    Serial.print(Tokens[TokenCnt]);
    Serial.print("' ");
  }
  Serial.println();

  Token = Tokens[0];  // first token on user cmd line
  CmdChar = (char) tolower(Token[0]); // first char of first token
  if (CmdChar == 's') {
    if (StepCmd()) UpdateConfig();
  }
  else if (CmdChar == 'r') {
    if (RTScmd()) UpdateConfig();
  }
  else if (CmdChar == 'c') {
    if (CTScmd()) UpdateConfig();
  }
  else if (CmdChar == 't') {
    if (TimeoutCmd()) UpdateConfig();
  }
  else if (CmdChar == 'd') {
    PrintConfig(Config);
  }
  else if (CmdChar == 'i') {
    if (InitCmd(Tokens[0])) UpdateConfig;    // require whole token
  }
  else if (CmdChar == 'b') {
    BootCmd();
  }
  else if (CmdChar == 'h') {
    PrintHelp();
  }
  else {
    snprintf(Msg, 80, "invalid command, Token '%s', hex %x %x %x, len %d", Token, Token[0], Token[1], Token[2], strlen(Token));
    Serial.println(Msg);
  }
} // UserConfig() 

// called when line available starting with Tokens[0] is step command
// Get StepIdx, StepArg, and msec if necessary
bool StepCmd() {
  char * endptr;
  long lmsec;
  int msec;
  uint8_t StepIdx;
  char * Token;

  Token = Tokens[1];
  if (Token == NULL) {
    Serial.println("Token NULL, incomplete command");
    return false;
  }
  // get the step index from 0 to 3
  long lStepIdx = strtol(Token, &endptr, 10);
  if (errno == ERANGE) {
    Serial.println("GetStepIdx: strtol() ERANGE");
    return false;
  } 
  if (endptr == Token) {
    Serial.println("GetStepIdx: (endptr == Token), no conversion");
    return false;
  }
  if (*endptr != '\0') {
    snprintf(Msg, 80, "GetStepIdx: Token %x, '%s', endptr %x, '%s', ", Token, endptr, endptr);
    Serial.println(Msg);
    return false;
  } 
  if ((lStepIdx < 0) | (lStepIdx > 3)) {
    Serial.println("step: StepIdx out of range 0:3");
    return false;
  }
  StepIdx = (uint8_t) lStepIdx;

  // get step argument
  Token = Tokens[2];
  if (Token == NULL) { // if missing argument
    Serial.println("step: missing step argument {r, t, o, c}");
    return false;
  } 
  char StepArg = (char) tolower(Token[0]);
  if (StepArg == 't') {
    msec = Get_msec(Tokens[3]);
    if (msec < 0) {
      return false;
    } else {
      Config.Step[StepIdx].Tx_msec = msec;
      return true;
    }    
  }
  if (StepArg == 'r') {
    msec = Get_msec(Tokens[3]);
    if (msec < 0) {
      return false;
    } else {
      Config.Step[StepIdx].Rx_msec = msec;
      return true;
    }
  }  
  if (StepArg == 'o') {
    Config.Step[StepIdx].RxPolarity = OPEN;
    return true;
  } 
  if (StepArg == 'c') {
    Config.Step[StepIdx].RxPolarity = CLOSED;
    return true;
  } 
  Serial.println("no valid argument");
  return false; // no valid argument
} // StepCmd()

// get msec Tx or Rx delay msec, 0 to 255
int Get_msec(char * Token) {  
  char * endptr;
  if (Token == NULL) {
    Serial.println("msec: missing delay value ");
    return -1;
  } 
  // Token available, check msec
  long lmsec = strtol(Token, &endptr, 10); 
  if (endptr == Token) {
    Serial.print("msec: strtol(msec) failed");
    return -1;
  } 
  if ((lmsec < 0) | (lmsec > 255)) {
    Serial.println("msec: out of range");
    return -1;
  }
  return (int) lmsec;
} // Get_msec()

// wait for enable or disable
bool RTScmd() {
  char * Token = Tokens[1];
  if (Token == NULL) {  // check mission command argument
    Serial.println("rts: incomplete command");
    return false;
  } 
  // Token not NULL
  char ArgChar = (tolower(Token[0])); // on first character...
  if (ArgChar == 'e') {
    Config.RTSEnable = true;
    return true;
  }
  if (ArgChar == 'd') {
    Config.RTSEnable = false;
    return true;
  }
  return false; // didn't find 'd' or 'e' argument  
}

bool CTScmd() {
  char * Token = Tokens[1];
  if (Token == NULL) {  // check mission command argument
    Serial.println("cts: incomplete command");
    return false;
  } 
  // Token not NULL
  char ArgChar = (tolower(Token[0]));  // on first character...
  if (ArgChar == 'e') {
    Config.RTSEnable = true;
    return true;
  }
  if (ArgChar == 'd') {
    Config.RTSEnable = false;
    return true;
  }
  return false;  
}

bool TimeoutCmd() {
  char * endptr;
  char * Token = Tokens[1];  // 
  if (Token == NULL) { // no time value waiting
    Serial.println("timeout: incomplete command");
    return false;
  } 
  // token not NULL, check time value
  unsigned long ulTimeout = strtoul(Token, &endptr, 10); // try to get timeout value
  if (endptr == Token) {  // check if not interpreted
    snprintf(Msg, 80, "UserInterface: user entry -%s- not an (unsigned long)", Token);
    Serial.println(Msg);
    return false;
  } 
  if (ulTimeout > 255) { // check if too large
    snprintf(Msg, 80, "case timeout: specified time %lu > 255", ulTimeout);
    Serial.println(Msg);
    return false;
  }
  Config.Timeout = (uint16_t) ulTimeout;
  needCmdPrompt = true;
}

bool BootCmd() {
  if (strcmp(Tokens[0], "Boot") == 0) { // require whole token
    _PROTECTED_WRITE(RSTCTRL.SWRR,1); // magic boot command
    Serial.println("reboot failed");
    return false;
  } else {
    Serial.println("UserInterface: 'Boot' type full command, case sensitive");
    return false;
  }
}  

// flush the user input, used at startup and after input error
void FlushLines() {
  while (reader.available()) {
    char Line[reader.len()];
    reader.read(Line);
    snprintf(Msg, 80, "FlushLines: Line %s, Len %d", Line, strlen(Line));
    Serial.println(Msg);
  }
} // FlushLines()

// The user may have made changes to the config
void UpdateConfig() {
    Config.CRC16 = CalcCRC(Config);  // update CRC16
    PutConfig(0, Config);  // write changed bytes of config to EEPROM
    SequencerSetOutputs(); // update the output immediately
    needCmdPrompt = true;
} // UpdateConfig()

bool InitCmd(char * Token) {
  // initialize config from EEPROM
  if (strcmp(Token, "Init")== 0) { // require whole token
    Config = InitDefaultConfig();
    return true;
  }  
  Serial.print("UserInterface: 'Init' command entry error -");
  Serial.print(Token);
  Serial.println("-");
  return false;
}

void PrintHelp() {
  Serial.println("This is help for the user interface.");
  Serial.println("The user enters command and parameters, MCU echos after end of line");
  Serial.println("Input is one line at a time");
  Serial.println("Entry is case insensitive, with two exceptions");
  Serial.println("Commands and alpha arguments check only the first letter, see examples");  
  Serial.println("Top level configuration commands: S(tep), R(TS), C(TS), T(imeout)"); 
  Serial.println("  Step {Step number 0 to 3} {T(x) delay, R(x) delay, O(pen) on rx, C(losed) on rx}");
  Serial.println("  RTS {E(nable), D(isable)}");
  Serial.println("  CTS {E(nable), D(isable)}");
  Serial.println("  Timeout 0 to 255 seconds, Tx timeout, 0 means disable");
  Serial.println("Top level Info commands: D(isplay), Boot, Init, H(elp)");
  Serial.println("  Display, print working configuration");
  Serial.println("  Init, spelled out, initialize configuration to programmed defaults");
  Serial.println("  Boot, spelled out, simulates power cycle");
  Serial.println("  Help, print this text");
  Serial.println("Changes are automatically written to EEPROM");
  Serial.println("Examples...");
  Serial.println("  's 0 t 100' step 0 tx delay 100 msec");
  Serial.println("  'step 0 tx 100' step 0 tx delay 100 msec, long form");
  Serial.println("  's 3 o, step 3 Open on Rx");
  Serial.println("  'r e', RTS enable");
  Serial.println("  't 120', tx timeout 120 seconds");
  Serial.println("  't 0', tx timeout disabled");
  Serial.println("  'd', display configuration");
  Serial.println("  'Init', case sensitive, spelled out, initialize to programmed defaults");
  Serial.println("  'Boot', case Sensitive, spelled out, reboot using software reset");
} // PrintHelp
