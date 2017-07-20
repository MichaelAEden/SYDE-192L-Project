#include <Arduino.h>
#include <Servo.h>

/*  --------------------------------------------
    -------   Declaring constants
    -------------------------------------------- */

// Botton keypad
enum KEY {KEY_NONE,                     // NONE: if nothing is going on, last button has been processed
          KEY_1, KEY_2, KEY_3, KEY_4,   // For buttons that were pressed
          KEY_RESET, KEY_LOCK           // reset, lock
};
bool isKeyPushed = false;               // Whether button is currently pressed / held down


// Safe combination
const byte CODE_LENGTH = 5;
const byte GUEST_PASSCODES = 3;
char guestPasscodes[][CODE_LENGTH] = {  // Array of guest passcodes with length of 5 numbers
  {1, 3, 3, 4, 2},
  {2, 4, 3, 4, 2},
  {1, 1, 2, 4, 3},
};
byte currentGuestPasscode = 0;          // Index of current code being used
byte codeNumsEntered = 0;               // How many passcode buttons pressed
int keyPressTimes[CODE_LENGTH] = {0, 0, 0, 0, 0};


// Accessing item - LED display
const int GRID_WIDTH = 3;
const int GRID_HEIGHT = 3;
byte gridSelectX;
byte gridSelectY;

bool items[][GRID_WIDTH] = {
  {true, true, true},
  {true, false, true},
  {true, true, true},
};



// Arduino ports
const byte INPUT_ANALOG = A0;       // Buttons
const byte INPUT_INTERRUPT = 2;     // Buttons

const byte OUTPUT_ACTIVE_CODE = 9;  // LED if currently entering code
const byte OUTPUT_SERVO = 12;       // Servo



// Servo
Servo servo;



// Input
enum STATE {NONE, INPUT_PASSCODE, ACCESS_ITEM};
volatile long lastInputTime = 0;     // Time recorded at last key press
KEY lastKeyPressed = KEY_NONE;       // Last key pressed



// Timer
const byte TIMEOUT_INPUT = 10;          // in seconds
const byte TIMEOUT_CYCLE_PASSCODE = 20; // in seconds

volatile long timerOverflow0 = 0;
const int SCALING_FACTOR_0 = 1024;                      // TCCR0B |= 5, scaling factor # of cycles before TCNT0 increments
const int TIMER0_FREQUENCY = 16000 / SCALING_FACTOR_0;  // Timer clock frequency in cycles per millisecond
const unsigned int TIMER0_RESET_INTERRUPT = 200;        // Triggers interrupt to increment overflow count, Actual max is 255

volatile long timerOverflow2 = 0;
const int SCALING_FACTOR_2 = 256;                       // TCCR2B |= 6
const int TIMER2_FREQUENCY = 16000 / SCALING_FACTOR_2;  // Timer clock frequency in cycles per millisecond
const unsigned int TIMER2_RESET_INTERRUPT = 200;        // Actual max is 255


/*  --------------------------------------------
    -------   Safe state variables
    -------------------------------------------- */

STATE state = NONE;  // Current state safe is in (e.g.: guest mode, reset passcode, etc.)
bool locked = true;         // Is safe currently locked
bool validPasscode = true; // Whether passcode entered was correct



void setup() {
  Serial.begin(9600);
  
  cli();  // Disables interrupt

  // Configure LED display
  pinMode(3, OUTPUT); // Row 1
  pinMode(4, OUTPUT); // Row 2
  pinMode(5, OUTPUT); // Row 3
  pinMode(6, OUTPUT); // Col 1
  pinMode(7, OUTPUT); // Col 2
  pinMode(8, OUTPUT); // Col 3
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);
  digitalWrite(8, HIGH);
  digitalWrite(3, LOW);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
    
  servo.attach(OUTPUT_SERVO);         // Servo now uses inputted pin
  pinMode(INPUT_ANALOG, INPUT);       // buttons
  pinMode(INPUT_INTERRUPT, INPUT);    // buttons

  // Reset all Timer0 and Timer2 registers
  TCNT0 = 0;
  TCCR0A = 0;
  TCCR0B = 0;

  TCNT2 = 0;
  TCCR2A = 0;
  TCCR2B = 0;

  // Timer0
  OCR0A = TIMER0_RESET_INTERRUPT; // output compare register - triggers interrupt before overflow
  TCCR0B |= (5 >> 0);   // Default setting on Timer0 is 5 (101)
  TIMSK0 |= (7 >> 0);   // interrupt flag (if on, enable interrupt)

  // Timer2
  OCR2A = TIMER2_RESET_INTERRUPT;
  TCCR2B |= (6 >> 0);   // Default setting on Timer2 is 6 (110)
  TIMSK2 |= (7 >> 0);

  sei();  // Enables interrupts

  attachInterrupt(INPUT_INTERRUPT - 2, buttonPushed, FALLING);
  Serial.println(F("BEGIN"));

  lock();
}

/*  --------------------------------------------
    -------   Main loop
    -------------------------------------------- */
    
void loop() {

  updateItemLEDs();

  if (!isKeyPushed && analogRead(INPUT_ANALOG) < 500) {
    //keyPressTimes[codeNumsEntered] = timeSinceReset();
    
    lastKeyPressed = getButton(analogRead(INPUT_ANALOG));
    isKeyPushed = true;
    Serial.print(lastKeyPressed);
  }
  else if (isKeyPushed && analogRead(INPUT_ANALOG) > 500) {
    isKeyPushed = false;
  }
  
  if (state != NONE) {

    // If checking input for a key combination
    if (state == INPUT_PASSCODE) {
      updateInputCombination();
    }

    // If prompting user to enter slot of item being accessed
    else if (state == ACCESS_ITEM) {
      updateAccessItem();
      showItemGrid();

      setState(NONE);
    }
  }

  // If there is no passcode being inputted, check for changing guest passcode
  else {
    if (guestCodeResetTime() > TIMEOUT_CYCLE_PASSCODE * 1000) {  
      currentGuestPasscode++; 
      currentGuestPasscode %= GUEST_PASSCODES; // cycle back to 1st passcode

      Serial.println();
      Serial.print("CHANGING PASSCODES: ");
      for (byte i = 0; i < CODE_LENGTH; i++)  Serial.print(char(guestPasscodes[currentGuestPasscode][i] + int('0')));
      Serial.println();

      resetGuestCodeCount();
    }
  }  
}

void updateInputCombination() {

  if (timeSinceLastButton() > TIMEOUT_INPUT * 1000) setState(NONE);
  if (lastKeyPressed == KEY_NONE) return;

  // If the last button pushed was a number
  if (lastKeyPressed >= KEY_1 && lastKeyPressed <= KEY_4) {
    digitalWrite(OUTPUT_ACTIVE_CODE, HIGH);           // if currently entering passcode, LED is on
    
    if (lastKeyPressed != guestPasscodes[currentGuestPasscode][codeNumsEntered]) validPasscode = false;
    
    codeNumsEntered++;

    // If all numbers have been entered, check if passcode is correct
    if (codeNumsEntered == CODE_LENGTH && validPasscode) {
      unlock();
      state = ACCESS_ITEM;
    }
  }
  
  else if (lastKeyPressed == KEY_LOCK) {
    lock();
    setState(NONE);
  }
  else if (lastKeyPressed == KEY_RESET) setState(NONE);

  lastKeyPressed = KEY_NONE; // Resets key so it does not get read twice  
}

void setState(STATE newState) {
  state = newState;
  
  if (state == NONE) {
    digitalWrite(OUTPUT_ACTIVE_CODE, LOW);
    state = NONE;
    codeNumsEntered = 0;
    validPasscode = true;
    TCCR2B |= (6 >> 0);   // continue counting again for passcode change (do not want to change while entering) (TCCR2B = 0 stops)
    lastKeyPressed = KEY_NONE;
  }
}

// Prompts user to enter slot ID being accessed
void updateAccessItem() {
  // Loops until valid input
  while (true) {

    Serial.println();

    Serial.println(F("Enter Grid Row."));
    waitForInput();
    gridSelectX = int(Serial.read()) - int('A');   // Converts string into grid square e.g.: (A2 -> 0, 1)

    Serial.println(F("Enter Grid Column."));
    waitForInput();
    gridSelectY = int(Serial.read()) - int('1');
    

    if ((gridSelectX >= GRID_WIDTH || gridSelectX < 0) || (gridSelectY >= GRID_HEIGHT || gridSelectY < 0))
      Serial.println(F("INVALID SQUARE"));
    else {
      toggleItemGrid(gridSelectX, gridSelectY);
      return;
    }
    
  }
}

void waitForInput() {
  while (!Serial.available()) {
    updateItemLEDs();
  }
}

/*  --------------------------------------------
    -------   Safe handlers
    -------------------------------------------- */

void unlock() {
  lightShow();
  
  locked = false;
  servo.write(0); // Rotates lock so box is no longer closed
  setState(ACCESS_ITEM);
}

void lock() {
  lightShow();
  
  locked = true;
  servo.write(90); // Rotates lock so box is closed
}

void toggleItemGrid(int x, int y) { // toggle LED
  items[x][y] = !items[x][y];
}

void showItemGrid() { // feedback to serial monitor
  for(int y = 0; y < GRID_HEIGHT; y++) {
    for(int x = 0; x < GRID_WIDTH; x++) {
      Serial.print(items[y][x]);
    }
    Serial.println();
  }
}

// Called every loop() cycle. Updates the LEDs showing which item slots are occupied
void updateItemLEDs() {
  
  // Note that output ports 3-5 are for rows, 6-8 are for columns 
  // Updates from top to bottom, left to right
  for(int y = 0; y < 3; y++) {
    digitalWrite(y + 6, LOW);   // Prepares row to be written to
    
    for(int x = 0; x < 3; x++) {
      if (items[y][x]) {
        digitalWrite(x + 3, HIGH);
        digitalWrite(x + 3, LOW);
      }
    }
    
    digitalWrite(y + 6, HIGH);   // Disables row
  }
}

/*  --------------------------------------------
    -------   Button handlers, interrupt
    -------------------------------------------- */

void buttonPushed() {       // button interrupt, called on falling signal
  if (state == NONE) setState(INPUT_PASSCODE);
  
  lastInputTime = timeSinceReset(); // Timer0 - buttons
  TCCR2B = 0;     // Stops counting guest password change
}

// Returns the ID of the last button that was pressed, based on the last read voltage
KEY getButton(int inputVoltage) {
  KEY lastButton = KEY_NONE;
  
  if (inputVoltage < 40)        {   lastButton = KEY_LOCK; }
  else if (inputVoltage < 70)   {   lastButton = KEY_RESET; }
  
  else if (inputVoltage < 170)    {   lastButton = KEY_4; }
  else if (inputVoltage < 260)    {   lastButton = KEY_3; }
  else if (inputVoltage < 340)    {   lastButton = KEY_2; }
  else if (inputVoltage < 460)    {   lastButton = KEY_1; }
  
  return lastButton;
}

/*  --------------------------------------------
    -------   Timer handlers
    -------------------------------------------- */

// Wait function, same as delay
void wait(int milliseconds) {
  long initialTime = timeSinceReset();
  while (timeSinceReset() < milliseconds + initialTime) {}
}

// Returns time since last button was pushed
long timeSinceLastButton() {
  return timeSinceReset() - lastInputTime;
}

long timeSinceReset() {
  // 250 clock cycles per millisecond with default scaling factor
  return (timerOverflow0 * TIMER0_RESET_INTERRUPT + TCNT0) / TIMER0_FREQUENCY;
}

void resetTimer() {
  TCNT0 = 0;
  timerOverflow0 = 0;
}

// Called when the timer reaches TIMER_RESET_INTERRUPT // uses timer0 for passcode timeout
ISR (TIMER0_COMPA_vect) {   // interrupt at 200 overflow, counts into timerOverflow0
 timerOverflow0++;
 TCNT0 = 0;
}



long guestCodeResetTime() { // uses timer2 for guest code reset
  // 250 clock cycles per millisecond with default scaling factor
  return (timerOverflow2 * TIMER2_RESET_INTERRUPT + TCNT2) / TIMER2_FREQUENCY;
}

void resetGuestCodeCount() {
  TCNT2 = 0;
  timerOverflow2 = 0;
}

// Called when the timer reaches TIMER_RESET_INTERRUPT
ISR (TIMER2_COMPA_vect) {
 timerOverflow2++;
 TCNT2 = 0;
}

/*  --------------------------------------------
    -------   Light Show
    -------------------------------------------- */

void lightShow() {
  for (byte i = 0; i < 10; i++) {
    digitalWrite(OUTPUT_ACTIVE_CODE, HIGH);
    wait(50);
    digitalWrite(OUTPUT_ACTIVE_CODE, LOW);
    wait(10);
  }

  for (byte i = 0; i < 10; i++) {
    flashLED(0, 0);
    flashLED(1, 0);
    flashLED(2, 0);
    flashLED(2, 1);
    flashLED(2, 2);
    flashLED(1, 2);
    flashLED(0, 2);
    flashLED(0, 1);
  }
}

void flashLED(int x, int y) {
  digitalWrite(x + 3, HIGH);
  digitalWrite(y + 6, LOW);
  wait(20);
  digitalWrite(x + 3, LOW);
  digitalWrite(y + 6, HIGH);
  wait(10);
}

