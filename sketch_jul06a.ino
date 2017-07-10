#include <LiquidCrystal.h>
#include <Arduino.h>

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


// Safe combination
const byte CODE_LENGTH = 6;
String adminPasscode = "123456";
String guestPasscodes[] = {"836204", "273027", "284027", "194629", "957368", "325492"};

//char adminPasscode[] = {1, 2, 3, 4, 5, 6};	// this is alternative to array of strings
//char* guestPasscodes[] = {
  //(char[]){1, 2, 3, 4, 5, 6},
  //(char[]){1, 2, 3, 4, 5, 6},
//};

KEY codeEntry[CODE_LENGTH];
byte codeNumsEntered = 0;



// Adding item 
const byte ALPHABET_LENGTH = 26;
const byte SCREEN_LIMIT = 12;     // Number of characters which can be displayed in one row of the LED



// Arduino ports
const byte BUTTON_INPUT = 13;
const byte INTERRUPT_INPUT = 0;

const byte OUTPUT_LOCKED = 10;



// Input
enum STATE {NONE, INPUT_ADMIN, INPUT_GUEST, RESET_PASSCODE, ADD_ITEM};
volatile byte lastInputVoltage = 0; // Voltage recorded from last button pressed
volatile long lastInputTime = 0;    // Time recorded at last button press 


// Timeout
const int TIMEOUT = 30; // in seconds



/*  --------------------------------------------
    -------   Safe state variables
    -------------------------------------------- */

STATE state;  // Current state safe is in (e.g.: guest mode, reset passcode, etc.)
bool locked;  // Is safe currently locked
bool validPasscode = false; // Whether passcode entered was correct



/*  --------------------------------------------
    -------   LCD Display setup
    -------------------------------------------- */

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int pointer = 0;  // 0 is "A"
int scrollPos = 0;
String userInput = "";



void setup() {
  locked = true;
  state = NONE;

  attachInterrupt(INTERRUPT_INPUT, buttonPushed, RISING);
  lcd.begin(16,2);
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
    if (state == INPUT_ADMIN || state == INPUT_GUEST) {
      updateInputCombination();
    }

    else if (state == ADD_ITEM) {
      updateAddItem();
    }
  }

  
}

void updateInputCombination() {
  
  // If the last button pushed was a number
  if (getLastButton() >= KEY_1 && getLastButton() <= KEY_9) {

    if (state == INPUT_ADMIN) {
       if (getLastButton() != codeEntry[codeNumsEntered]) validPasscode = false;
    }
    else if (state == INPUT_GUEST) {
       if (getLastButton() != adminPasscode[codeNumsEntered]) validPasscode = false;
    }
    
    codeNumsEntered++;

    // If all numbers have been entered, check if passcode is correct
    if (codeNumsEntered == CODE_LENGTH && validPasscode) unlock();
    
  }
  else {
    // Reset state
  }
}

// Updates a user adding an item to the safe
void updateAddItem() {
  int x = analogRead(0);
  
  if (x < 100)
      pointer++;    // Go right
  else if (x < 400)
      userInput += char(pointer + int('a'));  // Adds selected character onto item name
  else if (x < 600)
      pointer--;    // Go left
  else if (x < 800)
      boolean nothing = false;
      // Do something

  String alphabet = "";

  if (pointer < 0)                 pointer = ALPHABET_LENGTH - 1;
  if (pointer >= ALPHABET_LENGTH)  pointer = 0;
  
  while (pointer > scrollPos + SCREEN_LIMIT) { scrollPos++; }
  while (pointer < scrollPos)                { scrollPos--; }
  
  for (int i = scrollPos; i < SCREEN_LIMIT + scrollPos; i++) {
    if (pointer == i)   alphabet += ' ' + char(i + int('A')) + ' ';
    else                alphabet += char(i + int('a'));
  }
  
  lcd.setCursor(0, 0);
  lcd.print("Add item:" + userInput);
  lcd.setCursor(0, 1);
  lcd.print(alphabet);
}

/*  --------------------------------------------
    -------   Safe handlers
    -------------------------------------------- */

void unlock() {
  locked = false;
  digitalWrite(OUTPUT_LOCKED, LOW);
}

void lock() {
  locked = true;
  digitalWrite(OUTPUT_LOCKED, HIGH);
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
