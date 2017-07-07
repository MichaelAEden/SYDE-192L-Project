/*  --------------------------------------------
    -------   Declaring constants
    -------------------------------------------- */

// Keypad
const char FIRST_VAL = 'a';
const char LAST_VAL = '_';
enum KEY {KEY_NONE, 
          KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
          KEY_ADD_ITEM, KEY_GUEST_MODE, KEY_ADMIN_MODE
};



// Code
const byte CODE_LENGTH = 6;
char adminPasscode[] = {1, 2, 3, 4, 5, 6};
char* guestPasscodes[] = {
  (char[]){1, 2, 3, 4, 5, 6},
  (char[]){1, 2, 3, 4, 5, 6},
};
KEY codeEntry[CODE_LENGTH];
byte codeNumsEntered = 0;



// Arduino input ports
const byte BUTTON_INPUT = 13;
const byte INTERRUPT_INPUT = 0;



// Input
enum STATE {NONE, INPUT_ADMIN, INPUT_GUEST, RESET_PASSCODE, ADD_ITEM};
volatile byte lastInputVoltage = 0; // Voltage recorded from last button pressed
volatile long lastInputTime = 0;    // Time recorded at last button press 


// Timeout
const int TIMEOUT = 30; // in seconds


/*  --------------------------------------------
    -------   Global variables
    -------------------------------------------- */

STATE state;  // Current state safe is in (e.g.: guest mode, reset passcode, etc.)
bool locked;  // Is safe currently locked
bool validPasscode = false; // Whether passcode entered was correct

void setup() {
  locked = true;
  state = NONE;

  attachInterrupt(INTERRUPT_INPUT, buttonPushed, RISING);
}

/*  --------------------------------------------
    -------   Main loop
    -------------------------------------------- */
    
void loop() {
  // pressing guest button on safe's display starts guestMode function
  // pressing admin button on safe's display starts adminMode function

  // automatically locks after X seconds once safe door is closed
  
  if (state != NONE) {

    // If we are checking input for a key combination
    if (state == INPUT_ADMIN || INPUT_GUEST) {
      
      // If the last button pushed was a number
      if (getLastButton() >= KEY_1 && getLastButton() >= KEY_9) {

        // If too many numbers are entered, passcode is invalid
        if (codeNumsEntered >= CODE_LENGTH) {
          validPasscode = false;
        }

        // Otherwise, add the last number pushed to an array
        else {
          codeEntry[codeNumsEntered] = getLastButton();
          codeNumsEntered++;

          // If all numbers have been entered, check if passcode is correct
          if (codeNumsEntered == CODE_LENGTH) {

            validPasscode = true;
            for (byte i = 0; i < CODE_LENGTH; i++) {
              if (state == INPUT_ADMIN) {
                 if (codeEntry[i] != adminPasscode[i]) validPasscode = false;
              }
              else if (state == INPUT_GUEST) {
                 if (codeEntry[i] != adminPasscode[i]) validPasscode = false;
              }
            }
          }
        }

        
      }
      else {
        // Reset state
      }
    }
  }
}

/*  --------------------------------------------
    -------   Button handlers
    -------------------------------------------- */

void buttonPushed() {
  lastInputVoltage = analogRead(BUTTON_INPUT);
  lastInputTime = millis();
}

// Returns the ID of the last button that was pressed,
// based on the last read voltage
KEY getLastButton() {
  KEY lastButton = KEY_NONE;
  
  if (lastInputVoltage < 10) {        lastButton = KEY_1; }
  else if (lastInputVoltage < 20) {   lastButton = KEY_1; }
  else if (lastInputVoltage < 30) {   lastButton = KEY_1; }
  // TODO: measure voltages, extend to all keys
  
  return lastButton;
}

/*  --------------------------------------------
    -------   Timer handlers
    -------------------------------------------- */

// Returns time since last button was pushed
long timeSinceLastButton() {
  return millis() - lastInputTime;
}

void guestMode(){    // mode where only temp. code can unlock the safe
  
}

void adminMode(){   // mode where regular admin. code can unlock the safe

}


