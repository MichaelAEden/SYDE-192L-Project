const byte CODE_LENGTH = 6;

const char FIRST_VAL = 'a';
const char LAST_VAL = '_';

const byte BUTTON_INPUT = 13;


enum STATE {NONE, INPUT_ADMIN, INPUT_GUEST, RESET_PASSCODE, ADD_ITEM};
enum KEYPAD {KEY_1, KEY_2, KEY_3, KEY_4, ADD_ITEM};


STATE currentState;

bool locked;

void setup() {
  // put your setup code here, to run once:

  locked = true;
  currentState = NONE;

  attachInterrupt(0, buttonPushed, RISING);
}

void loop() {
  if (state != NONE) {
    if (state = )
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


