
#include "UserInterface.h"
#include "SoftwareConfig.h"

// check for new state, if new, prompt
// pass in, previous userconfig state, currenet userconfig state, prompt string
void newUserState(uint8_t prevUCS, uint8_t UCS, String Prompt) {
  if (prevUCS != UCS) {
    Serial.print("                                                         \r");
    Serial.print(Prompt);
  }
}

// user configureation setting state machine
// called each pass througn loop()
// command tree
//   {s, k, r, c, t, ?}         // Step, Key, RTS, CTS, Timer, Config  
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
sConfig_t UserConfig(sConfig_t Config) {
  enum UserConfigState {
    first, 
    top, 
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
  static UserConfigState UCS = first;
  static UserConfigState pUCS;
  static UserConfigState nUCS;
  char inChar;
  String inStr;

  pUCS = UCS;
  UCS  = nUCS;
  switch (UCS) {
    case first: {
      Serial.println("UserConfig");
      while(Serial.available()) Serial.read(); // clear out input buffer
      nUCS = top;
      break;
    }
    case top: {
      newUserState(pUCS, UCS, "{Step, Key, RTS, CTS, Timeout, Dump: ");
      if (Serial.available() < 1) break; // stay in this state until character
      inChar = (char) Serial.read();
      Serial.print(inChar);
      switch (inChar) {
        case 's':
          nUCS = step;
          break;
        case 'k':
          nUCS = key;
          break;
        case 'r':
          nUCS = rts;
          break;
        case 'c':
          nUCS = cts;
          break;
        case 't':
          nUCS = timeout;
          break;
        case 'd':
          nUCS = dump;
          break;
        default:
          nUCS = err;
      }  // if Serial.available()
    }
    case step: {
      newUserState(pUCS, UCS, "Step {1, 2, 3, 4}: ");
      if (Serial.available() > 0) {
        inChar = (char) Serial.read();
        Serial.print(inChar);
        //StepIdx = atoi(inChar);
        switch (inChar) {
          case '1':
          case '2':
          case '3':
          case '4':
            Serial.print("                                                                \r");
            Serial.print("Step ");
            Serial.print(" {Assert msec, Release msec, Open, Closed: ");
           break;
          case 'a':  // Assert time
            nUCS = stepA;
            break;
          case 'r':  // Release time
            nUCS = stepR;
            break;
          case 'o':  // Open
            nUCS = stepO;
            break;
          case 'c':  // Closed
            nUCS = stepC;
            break;
            default:   // error
            break;
        } // switch on step char
      } // switch on step character
    }
    case stepA:  { // Step Assert, read time
      newUserState(pUCS, UCS, "Step Assert time (msec): ");
      String numStr = Serial.readString();
      Config.Timer.Time = numStr.toInt();
      nUCS = top;
    }
    case stepR: { // Step Release, read time
      newUserState(pUCS, UCS, "Step x Release (time)}: ");
    }
    case stepO: {
      newUserState(pUCS, UCS, "Step x Open: ");
      Config.Step[0].Polarity = 1;
      nUCS = top;
    }
    case stepC: {
      newUserState(pUCS, UCS, "Step x Closed: ");
      Config.Step[0].Polarity = 0;
      nUCS = top;
    }
    case key: {
      newUserState(pUCS, UCS, "Key {Enable, Polarity}: ");
    }
    case keyE: {
      newUserState(pUCS, UCS, "Key Enable {1, 0};");
    }
    case keyP: {
      newUserState(pUCS, UCS, "Key Enable {1, 0};");
    }
    case rts: {
      newUserState(pUCS, UCS, "RTS {Enable, Polarity}: ");
    }
    case rtsE: {
      newUserState(pUCS, UCS, "RTS Enable {y, n};");
    }
    case rtsP: {
      newUserState(pUCS, UCS, "RTS Polarity {1, 0}: ");
    }
    case cts: {
      newUserState(pUCS, UCS, "CTS {Enable, Polarity: ");
    }
    case ctsE: {
      newUserState(pUCS, UCS, "CTS Enable {y, n}: ");
    }
    case ctsP: {
      newUserState(pUCS, UCS, "CTS Polarity {1, 0}: ");
    }
    case timeout: {
      newUserState(pUCS, UCS, "Timeout {seconds}: ");
    }
    case dump: {
      newUserState(pUCS, UCS, "Dump of Configuration\n");
      PrintConfig(Config);
    }
    case err: {
      newUserState(pUCS, UCS, "Step {Assert time, Release time, Open, Closed}: ");
      nUCS = top;
    }
  } // switch (UCS)
  return Config;
} // userConfig()

