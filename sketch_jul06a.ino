char adminPasscode[6] = {1, 2, 3, 4, 5, 6};
char* guestPasscodes[] = {
  (char[]){1, 2, 3, 4, 5, 6},
  (char[]){1, 2, 3, 4, 5, 6},
};

const byte CODE_LENGTH = 6;

const char FIRST_VAL = 'a';
const char LAST_VAL = '_';

const byte BUTTON_INPUT = 13;


enum STATE {NONE, INPUT_ADMIN, INPUT_GUEST, RESET_PASSCODE, ADD_ITEM};
enum KEYPAD {KEY_1, KEY_2, KEY_3, KEY_4, KEY_ADD_ITEM};


STATE state;

bool locked;

void setup() {
  // put your setup code here, to run once:

  locked = true;
  state = NONE;

  attachInterrupt(0, buttonPushed, RISING);
}

void loop() {
  // pressing guest button on safe's display starts guestMode function
  // pressing admin button on safe's display starts adminMode function

  // automatically locks after X seconds once safe door is closed
  
  if (state != NONE) {
    if (true) {}
  }
}

void buttonPushed() {
  int pinInput = analogRead(BUTTON_INPUT);
  byte lastButton = 0;

  if (pinInput < 10) {        lastButton = 1; }
  else if (pinInput < 20) {   lastButton = 2; }
  else if (pinInput < 30) {   lastButton = 3; }
  else if (pinInput < 40) {   lastButton = 4; }
  else if (pinInput < 50) {   lastButton = 5; }
  else if (pinInput < 60) {   lastButton = 6; }
  else if (pinInput < 10) {   lastButton = 7; }
  else if (pinInput < 20) {   lastButton = 8; }
  else if (pinInput < 20) {   lastButton = 9; }
  else if (pinInput < 20) {   lastButton = 10; }
}

void guestMode(){    // mode where only temp. code can unlock the safe
  
}

void adminMode(){   // mode where regular admin. code can unlock the safe

}


