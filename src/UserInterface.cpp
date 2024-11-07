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
#include "SoftwareConfig.h"

enum UserConfigState {
  first,
  read,
    step,
      stepA,
      stepR,
      stepO,
      stepC,  
    key, 
      keyE, 
      keyP,
    rts, 
      rtsE,
      rtsP,
    cts, 
      ctsE,
      ctsP,
    timeout,
    dump, 
    err
};

// global 
static UserConfigState UCS = first;
static UserConfigState prevUCS;
static UserConfigState nextUCS;
String sLine;
char Line[] = "                                             ";
char * pLine = &Line[0];
char * pToken;
char Delimiter = ' ';
char * pDelimiter = &Delimiter;

void newUserState(uint8_t prevUCS, uint8_t UCS, String Prompt) {
  if (prevUCS != UCS) {
    Serial.print("                                                         \r");
    Serial.print(Prompt);
  }
}

// Called after each case statement for user state machine
// tries to read the next token on the String sLine
// if not, prompts for next input for state, then reads user input
// returns NULL if read times out
char * GetNextToken(char * pPrompt) {
  if (prevUCS != UCS) { // global variables, if first pass, read next token 
    pToken = strtok(0, pDelimiter);
    // if no token, prompt
    if (*pToken == NULL) {
      Serial.println(pPrompt);
    }
  } // if first pass

  // follow on passes, wait for serial entry
  // on 2nd and following entries, use token or start read
  if ((*pToken == NULL) && (Serial.available() == 0)) {
    return NULL;
  }

  // may have Token or may have user input
  if (*pToken == NULL) {
    sLine = Serial.readStringUntil('\r');
    if (sLine == NULL) {
      return NULL;
    }
    // TODO check is sLine filled to end
    // at least some input in token
      pToken = strtok(NULL, pDelimiter);  // get next token
      Serial.println();
      Serial.print("UserInterface: 2nd token ");
      Serial.print(*pToken);
      Serial.println();   
  }
} // GetNextToken

// user configureation setting state machine
// called each pass througn loop()
// command tree
//   {s, k, r, c, t, d}         // Step, Key, RTS, CTS, Timer, Dump  
//     s {1, 2, 3, 4} {a, r, p} // Step Number Assert, Release, Polarity
//       s a (msec)             // Step Assert 0 to 255 msec
//       s r (msec)             // Step Release 0 to 255 msec
//     k {e, p}                 // Key
//       k e {y, n}             // Key Enable yes/no
//       k p {o, c}             // Key Polarity, High/Low
//     r {e, p}                 // RTS
//       r e {y, n}             // RTS Enable yes/no
//       r p {h, l}             // RTS Polarity High/Low
//     c {e, p}                 // CTS 
//       c e {y, n}             // CTS Enable yes/no
//       c p {h, l}             // CTS Polarity High/Low
//     t {time}                 // seconds, 0 means no timeout
//     d                        // Dump, print configuration

sConfig_t UserConfig(sConfig_t Config) {

  prevUCS = UCS;
  UCS = nextUCS;  
  
  switch (UCS) {
     case first:
      while(Serial.available()) Serial.read(); // clear out input buffer
      nextUCS = read;
      break;

    // read user command as a String, convert to null terminated C string
    case read:
      char Prompt[] = "Step, Key, RTS, CTS, Timeout, Dump: ";
      char * pPrompt = Prompt;
      if (prevUCS != UCS) { // global variables, if first pass, read next token 
        Serial.println(pPrompt);
      if(Serial.available() == 0) {
        nextUCS = read;
        break;
      }
      // some charcters available
      sLine = Serial.readStringUntil('\r');
      if (sLine == NULL) {
        nextUCS = read;
        break;
      }
      strcpy(pLine, sLine.c_str());      
      pToken = strtok(pLine, pDelimiter);

      Serial.println();
      Serial.print("UserInterface: 1st token ");
      Serial.print(*pToken);
      Serial.println();   

      // switch on first character of first token
      Serial.print("UserInterface: switch on first character ");
      Serial.print(pToken[0]);
      Serial.println();

      switch (pToken[0]) {
        case 's':
          nextUCS = step;
          break;
        case 'k':
          nextUCS = key;
          break;
        case 'r':
          nextUCS = rts;
          break;
        case 'c':
          nextUCS = cts;
          break;
        case 't':
          nextUCS = timeout;
          break;
        case 'd':
          nextUCS = dump;
          break;
        default:
          nextUCS = err;
      }  // if Serial.available()
      break;

    // on first entry, read the next token,
    // if no token on user line, emit prompt for step number
    case step: 
      pToken = GetNextToken(pPrompt);
      switch ((uint8_t) pToken) {
        case '1':
        case '2':
        case '3':
        case '4':
          Serial.print("                                                                \r");
          Serial.print("Step ");
          Serial.print(" {Assert msec, Release msec, Open, Closed: ");
          break;
        case 'a':  // Assert time
          nextUCS = stepA;
          break;
        case 'r':  // Release time
          nextUCS = stepR;
          break;
        case 'o':  // Open
          nextUCS = stepO;
          break;
        case 'c':  // Closed
          nextUCS = stepC;
          break;
          default:   // error
          break;
      } // switch on step character
      break;
    case stepA:  // Step Assert, read time
      newUserState(prevUCS, UCS, "Step Assert time (msec): ");
      String numStr = Serial.readString();
      Config.Timer.Time = numStr.toInt();
      nextUCS = read;
      break;
    case stepR:  // Step Release, read time
      newUserState(prevUCS, UCS, "Step x Release (time)}: ");
      nextUCS = read;
      break;
    case stepO: 
      newUserState(prevUCS, UCS, "Step x Open: ");
      Config.Step[0].Polarity = 1;
      nextUCS = read;
      break;
    case stepC: 
      newUserState(prevUCS, UCS, "Step x Closed: ");
      Config.Step[0].Polarity = 0;
      nextUCS = read;
      break;
    case key: 
      newUserState(prevUCS, UCS, "Key {Enable, Polarity}: ");
      nextUCS = read;
      break;
    case keyE: 
      newUserState(prevUCS, UCS, "Key Enable {1, 0};");
      nextUCS = read;
      break;
    case keyP: 
      newUserState(prevUCS, UCS, "Key Enable {1, 0};");
      nextUCS = read;
      break;
    case rts:
      newUserState(prevUCS, UCS, "RTS {Enable, Polarity}: ");
      nextUCS = read;
      break;
    case rtsE: 
      newUserState(prevUCS, UCS, "RTS Enable {y, n};");
      nextUCS = read;
      break;
    case rtsP:
      newUserState(prevUCS, UCS, "RTS Polarity {1, 0}: ");
      nextUCS = read;
      break;
    case cts: 
      newUserState(prevUCS, UCS, "CTS {Enable, Polarity: ");
      nextUCS = read;
      break;
    case ctsE: 
      newUserState(prevUCS, UCS, "CTS Enable {y, n}: ");
      nextUCS = read;
      break;
    case ctsP: 
      newUserState(prevUCS, UCS, "CTS Polarity {1, 0}: ");
      nextUCS = read;
      break;
    case timeout: 
      newUserState(prevUCS, UCS, "Timeout {seconds}: ");
      nextUCS = read;
      break;
    case dump: 
      newUserState(prevUCS, UCS, "Dump of Configuration\n");
      PrintConfig(Config);
      nextUCS = read;
      break;
    case err: 
      newUserState(prevUCS, UCS, "Step {Assert time, Release time, Open, Closed}: ");
      nextUCS = read;
      break;
  } // switch (UCS)
  return Config;
} // userConfig()

