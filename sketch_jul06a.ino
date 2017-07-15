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
const byte GRID_WIDTH = 3;
const byte GRID_HEIGHT = 2;
const byte SCREEN_LIMIT = 12;     // Number of characters which can be displayed in one row of the LED
int selectionState = 0;           // O if choosing x, 1 if choosing y
int gridSelectX = -1;
int gridSelectY = -1;

bool items[][GRID_WIDTH] = {
  {false, false, false},
  {false, false, false},
  {false, false, false},
}; // Item matrix



// Arduino ports
const byte BUTTON_INPUT = A0;
const byte INTERRUPT_INPUT = 0;

const byte OUTPUT_LOCKED = 10;



// Input
enum STATE {NONE, INPUT_ADMIN, INPUT_GUEST, RESET_PASSCODE, ADD_ITEM};
volatile byte lastInputVoltage = 0; // Voltage recorded from last button pressed
volatile long lastInputTime = 0;    // Time recorded at last button press 


// Timer
const int TIMEOUT = 30; // in seconds

volatile long timerOverflow = 0;

const int SCALING_FACTOR = 256;
const int TIMER_FREQUENCY = 16000 / SCALING_FACTOR;  // Timer clock frequency in cycles per millisecond
const long TIMER_RESET_INTERRUPT = 65536 / 2;


/*  --------------------------------------------
    -------   Safe state variables
    -------------------------------------------- */

STATE state;  // Current state safe is in (e.g.: guest mode, reset passcode, etc.)
bool locked;  // Is safe currently locked
bool validPasscode = false; // Whether passcode entered was correct

bool shouldPrint = false;



/*  --------------------------------------------
    -------   LCD Display setup
    -------------------------------------------- */

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int pointer = 0;  // 0 is "A"
String userInput = "";



void setup() {
  Serial.begin(9600);
  
  locked = true;
  state = ADD_ITEM;

  cli();  // Disables interrupt

  for(int i = 1; i <= 5; i++) {   pinMode(i, OUTPUT);   }

  // Reset all Timer1 registers
  TCNT1 = 0;
  TCCR1A = 0;
  TCCR1B = 0;

  // Trigger timer reset at 
  OCR1A = TIMER_RESET_INTERRUPT;
  TCCR1B |= (4 >> 0);   // Default setting on Timer1 is 4 (100)

  TIMSK1 |= (7 >> 0);

  sei();  // Enables interrupts

  // attachInterrupt(INTERRUPT_INPUT, buttonPushed, HIGH);
  lcd.begin(16,2);

  Serial.println("BEGIN");
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

  lastInputVoltage = analogRead(BUTTON_INPUT);
  
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

  updateLCDOutput();
  updateLCDInput();
}

void updateLCDOutput() {
  String alphabet = "";

  // Grid uses alphabet horizontally, numbers vertically
  if (selectionState == 0) {
    for (int i = 0; i < GRID_WIDTH; i++) {
      if (pointer == i)   { alphabet += char(i + int('A')); }
      else                { alphabet += char(i + int('a')); }
    }
  }
  else {
    for (int i = 0; i < GRID_WIDTH; i++) {
      if (pointer == i)   { alphabet += char(i + int('1')); }
    }
  }
  
  lcd.setCursor(0, 0);
  lcd.print(F("Add Item: "));
  
  if (gridSelectX != -1) {                        lcd.print(char(gridSelectX + int('A'))); }
  if (gridSelectY != -1) { lcd.setCursor(11, 0);  lcd.print(char(gridSelectY + int('1'))); }
  
  lcd.setCursor(0, 1);
  lcd.print(alphabet);
}

void updateLCDInput() {
  int x = analogRead(0);
  
  if (x < 100) {
    pointer++;    // Go right
  }
  else if (x < 200) {
    if (selectionState == 0) { 
      gridSelectX = pointer;
      selectionState++;
      lcd.clear();
    }
    
    else if (selectionState == 1) { 
      gridSelectY = pointer; 
      toggleItemGrid(gridSelectX, gridSelectY);
      
      updateLCDOutput(); // So user can see which grid spot they've selected
      delay(500);
      
      gridSelectX = -1;
      gridSelectY = -1;

      selectionState = 0;

      showItemGrid();
      lcd.clear();
    }
    
    pointer = 0;
  }
  
  else if (x < 600) {
    pointer--;    // Go left
  }

  if (pointer < 0) {  pointer = 0; }
  if (pointer >= GRID_WIDTH - 1 && selectionState == 0) { pointer = GRID_WIDTH - 1; }
  if (pointer >= GRID_HEIGHT - 1 && selectionState == 1) { pointer = GRID_HEIGHT - 1; }

  if (x < 1023) {
    delay(200);
  }
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

void toggleItemGrid(int x, int y) {
  items[x][y] = !items[x][y];
}

void showItemGrid() {
  for(int y = 0; y < GRID_HEIGHT; y++) {
    for(int x = 0; x < GRID_WIDTH; x++) {
      Serial.print(items[x][y]);
    }
    Serial.println();
  }
}

// Called every loop() cycle. Updates the LEDs showing which item slots are occupied
void updateItemLEDs() {
  
  // Note that output ports 3-5 are for rows, 6-7 are for columns 
  // Updates from top to bottom, left to right
  for(int y = 3; y < 5; y++) {
    digitalWrite(y, HIGH);   // Prepares row to be written to
    
    for(int x = 6; x < 7; x++) {
      if (items[x][y]) {
        digitalWrite(x, HIGH);
        delay(30);
        digitalWrite(x, LOW);
      }
    }
    
    digitalWrite(y, LOW);   // Disables row
  }
  
}

/*  --------------------------------------------
    -------   Button handlers
    -------------------------------------------- */

void buttonPushed() {
  lastInputVoltage = analogRead(BUTTON_INPUT);
  lastInputTime = timeSinceReset();
}

// Returns the ID of the last button that was pressed,
// based on the last read voltage
KEY getLastButton() {
  KEY lastButton = KEY_NONE;
  
  if (lastInputVoltage > 0)       {   lastButton = KEY_1; }
  else if (lastInputVoltage < 20) {   lastButton = KEY_1; }
  else if (lastInputVoltage < 30) {   lastButton = KEY_1; }
  // TODO: measure voltages, extend to all keys

  lastInputVoltage = 0;
  
  return lastButton;
}

/*  --------------------------------------------
    -------   Timer handlers
    -------------------------------------------- */

// Returns time since last button was pushed
long timeSinceLastButton() {
  return timeSinceReset() - lastInputTime;
}

long timeSinceReset() {
  // 250 clock cycles per millisecond with default scaling factor
  return (timerOverflow * TIMER_RESET_INTERRUPT + TCNT1) / TIMER_FREQUENCY;
}

void resetTimer() {
  TCNT1 = 0;
  timerOverflow = 0;
}

// Called when the timer reaches TIMER_RESET_INTERRUPT
ISR (TIMER1_COMPA_vect) {
 timerOverflow++;
 TCNT1 = 0;
}

