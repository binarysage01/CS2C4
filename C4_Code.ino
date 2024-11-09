
//1) WATCH A TUTORIAL TO INSTALL THE LIBRARIES ABOVE THROUGH THE LIBRARY MANAGER
  #include "Arduino.h"
  #include "SoftwareSerial.h"
  #include "DFRobotDFPlayerMini.h"
  #include <Keypad.h>
  #include <LiquidCrystal_I2C.h>

//2) PUT WHICH PINS YOU CONNECTED WHERE
//##############################################################
  #define BLUE A2 //RGB LED BLUE LEG
  #define GREEN A1 //RGB LED GREEN LEG
  #define RED A0 //RGB LED RED LEG

  #define BUTTON 9
  #define SWITCH 12
  #define BUZZER 13

  //KEYPAD PINS (if the keypad prints wrong keys - watch a youtube video on "how to connect a matrix keypad to arduino")
  #define KEYPADROW0 7
  #define KEYPADROW1 2
  #define KEYPADROW2 3
  #define KEYPADROW3 5
  #define KEYPADCOL0 6
  #define KEYPADCOL1 8
  #define KEYPADCOL2 4


//3) DONT CHANGE ANYTHING BELOW, YOU"RE DONE :)
//##############################################################
#define SETUP 0
#define IDLE 1
#define ENTERCODE 2
#define ARMED 3
#define EXPLOSION 4
#define DEFUSED 5

#define BEEPLENGTH 125
#define TONEBUTTON 3000
#define TONEWRONG 500
#define TONEBEEP25 2500
#define TONEBEEP 4500
#define TONEDEF 1700
#define TONEDEFFAIL 1250

// declare variables
byte mode = SETUP;

//armed vars
unsigned long beepInterval = 1000;
unsigned long previousTime;
unsigned long startTime = 0;
unsigned long countdownTime = 40000;

//led blink vars
bool ledState = false;
unsigned long ledBlinkStartTime;
static bool LEDinitialized = false;

//defuse vars
unsigned long defuseTime = 5000;               // 10 seconds to defuse
const unsigned long defuseBeepInterval = 500;  // Beep interval during defuse
unsigned long defuseStartTime = 0;
bool isDefusing = false;
bool deftone = true;

//defused/explosion time flash
unsigned long finalDefuseTime = 0;
bool displayTime = true;           // Flag to toggle the display
unsigned long flashStartTime = 0;  // Timer for flashing

// Buzzer vars
unsigned long previousMillis = 0;
unsigned long toneInterval = 150;                     // Interval between tones in milliseconds
unsigned long extendedInterval = toneInterval * 1.5;  // Extended interval after the third beep
// const int highTone = 2000;                            // Frequency of the high tone (Hz)
int beepCount = 0;

// DFPlayer setup
SoftwareSerial mySoftwareSerial(10, 11);  // RX, TX
DFRobotDFPlayerMini myDFPlayer;

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);
byte tChar[] = { B00000, B01110, B11111, B10101, B11111, B01110, B00000, B01110 };
byte ctChar[] = { B01010, B01010, B00100, B01010, B11011, B10001, B10001, B10001 };

//Keypad setup
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};
byte rowPins[ROWS] = { KEYPADROW0, KEYPADROW1, KEYPADROW2, KEYPADROW3 };
byte colPins[COLS] = { KEYPADCOL0, KEYPADCOL1, KEYPADCOL2 };
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Counter variables
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;

void setup() {
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);

  if (!myDFPlayer.begin(mySoftwareSerial)) {
    while (true)
      ;  // Halt if DFPlayer initialization fails
  }
  // myDFPlayer.volume(15);  // Set volume value. From 0 to 30


  //setup switch
  pinMode(SWITCH, INPUT);

  //setup LED
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);

  analogWrite(BLUE, 138);
  analogWrite(GREEN, 138);
  analogWrite(RED, 138);

  //setup button
  pinMode(BUTTON, INPUT_PULLUP);

  //setup LCD
  lcd.begin();
  lcd.clear();
  lcd.noBacklight();
  lcd.createChar(0, tChar);
  lcd.createChar(1, ctChar);
}

void loop() {
  unsigned long currentTime = millis();

  if (mode == SETUP) {
    static char inputBuffer[5] = "";  // Buffer for input
    static byte index = 0;
    static int setupStep = 0;  // Track which value is being entered

    lcd.backlight();
    lcd.setCursor(0, 0);
    if (setupStep == 0) {
      lcd.print("Boom Time(s)");
    } else if (setupStep == 1) {
      lcd.print("Defuse Time(s)");
    } else {
      lcd.print("Set Volume 0-30");
    }
    lcd.setCursor(0, 1);
    lcd.print(inputBuffer);

    char key = keypad.getKey();
    if (key) {
      if (key == '#') {
        if (setupStep == 0) {
          if (strlen(inputBuffer) == 0) {
            countdownTime = 40000;  // Default 40 seconds
          } else {
            countdownTime = atol(inputBuffer) * 1000;  // Convert to milliseconds
          }
          setupStep = 1;  // Move to defuse time input
        } else if (setupStep == 1) {
          if (strlen(inputBuffer) == 0) {
            defuseTime = 5000;  // Default 5 seconds
          } else {
            defuseTime = atol(inputBuffer) * 1000;  // Convert to milliseconds
          }
          setupStep = 2;  // Move to volume input
        } else {
          int volume;
          if (strlen(inputBuffer) == 0) {
            volume = 18;  // Default volume
          } else {
            volume = atoi(inputBuffer);  // Convert input to integer
          }
          if (volume >= 0 && volume <= 30) {
            myDFPlayer.volume(volume);  // Set volume
          } else {
            myDFPlayer.volume(20);  // Default volume if out of range
          }
          mode = IDLE;  // Move to IDLE mode
        }
        memset(inputBuffer, 0, sizeof(inputBuffer));  // Clear input buffer
        index = 0;
        lcd.clear();
      } else if (key == '*') {
        memset(inputBuffer, 0, sizeof(inputBuffer));  // Clear input
        index = 0;
        lcd.clear();
      } else if (index < sizeof(inputBuffer) - 1) {
        inputBuffer[index++] = key;  // Add key to input
        inputBuffer[index] = '\0';   // Null-terminate
      }
    }
  }

  if (mode == IDLE) {  // Standby mode


    if (digitalRead(SWITCH) == HIGH) {
      mode = ENTERCODE;
      myDFPlayer.play(1);  // Play audio 1
    } else {
      lcd.clear();
      lcd.noBacklight();
      analogWrite(BLUE, 138);
      analogWrite(GREEN, 138);
      analogWrite(RED, 138);
      mode = IDLE;
    }
  }

  if (mode == ENTERCODE) {  // Code entering mode
    char key = keypad.getKey();
    static bool initialized = false;
    static byte i = 0;
    const char correctCode[] = "7355608";
    char enteredCode[8];

    if (!initialized) {
      lcd.clear();
      lcd.backlight();
      lcd.setCursor(9, 1);  // character, line
      lcd.print("*******");
      memset(enteredCode, 0, sizeof(enteredCode));
      analogWrite(BLUE, 0);
      analogWrite(GREEN, 0);
      analogWrite(RED, 0);
      initialized = true;
    }

    if (digitalRead(SWITCH) == LOW) {
      delay(250);
      if (digitalRead(SWITCH) == LOW) {
        initialized = false;
        mode = IDLE;
        i = 0;
        memset(enteredCode, 0, sizeof(enteredCode));
      }
    }

    if (key) {
      if (key == '*' || key == '#') {
        initialized = false;
        i = 0;
        memset(enteredCode, 0, sizeof(enteredCode));
      } else {
        lcd.setCursor(15 - i, 1);  // character 0, line 0
        enteredCode[i] = key;
        lcd.print(enteredCode);
        Serial.println(enteredCode);
        i++;
        tone(BUZZER, TONEBUTTON, BEEPLENGTH);
      }

      if (i >= 7) {
        if (strcmp(enteredCode, correctCode) == 0) {
          lcd.clear();
          Serial.println("Correct");
          mode = ARMED;
          initialized = false;
          i = 0;
          memset(enteredCode, 0, sizeof(enteredCode));
          startTime = millis() - 100;
          myDFPlayer.play(3);  // Play audio 3
        } else {
          Serial.println(enteredCode);
          Serial.println(correctCode);
          Serial.println("Wrong");
          initialized = false;
          i = 0;
          memset(enteredCode, 0, sizeof(enteredCode));
          tone(BUZZER, TONEWRONG, BEEPLENGTH);
        }
      }
    }
  }

  if (mode == ARMED) {
    unsigned long elapsedTime = currentTime - startTime;

    if (elapsedTime < countdownTime) {
      float p = (float)elapsedTime / countdownTime;        // Calculate percentage of time elapsed (0.0 to 1.0)
      float bps = 1.049 * exp(0.244 * p + 1.764 * p * p);  // Calculate beeps per second using the formula
      beepInterval = 1000 / bps;                           // Convert BPS to interval in milliseconds

      if (currentTime - previousTime >= beepInterval) {
        previousTime = currentTime;
        Serial.println("Beep!");
        led_blink(isDefusing);

        // Play 2500 Hz tone if less than 25% time is left
        if (elapsedTime >= 0.75 * countdownTime && !isDefusing) {
          tone(BUZZER, TONEBEEP25, BEEPLENGTH);  // Start 2500 Hz tone

          // Delay 50 ms before starting the 3000 Hz tone
          unsigned long toneStartTime = millis();
          while (millis() - toneStartTime < 50) {
            // Wait for 50 ms
          }

          tone(BUZZER, TONEBEEP, BEEPLENGTH);  // Start 3000 Hz tone
        } else if (!isDefusing) {
          tone(BUZZER, TONEBEEP, BEEPLENGTH);  // Play 3000 Hz tone normally
        }

        // Record the time when the LED was turned on
        ledBlinkStartTime = millis();
        ledState = true;
      }

      // Handle LED blinking
      if (ledState && (currentTime - ledBlinkStartTime >= 50)) {
        // Turn off LED after 50ms
        analogWrite(BLUE, 0);
        analogWrite(GREEN, 0);
        analogWrite(RED, 0);
        ledState = false;
      }

      lcd_update(elapsedTime);

      // Check if defuse button is pressed
      if (digitalRead(BUTTON) == LOW) {
        if (!buttonPressed) {
          defuseStartTime = millis();  // Record the time when the button is first pressed
          buttonPressed = true;
          isDefusing = true;
        }

        // Calculate how long the button has been held down
        unsigned long defuseElapsedTime = millis() - defuseStartTime;

        // If the button has been held for 10 seconds, change mode to DEFUSED
        if (defuseElapsedTime >= defuseTime) {
          Serial.println("Button held for 10 seconds! Defused!");
          buttonPressed = false;
          mode = DEFUSED;
          myDFPlayer.play(9);  // Play audio 9
        } else {
          // Display defusing progress
          int progress = (defuseElapsedTime * 16) / defuseTime;  // Calculate progress as 0 to 16
          lcd.setCursor(0, 1);
          lcd.write(1);
          for (int i = 0; i < 15; i++) {
            if (i < progress) {
              lcd.print((char)255);  // Print full block character
            } else {
              lcd.print(" ");
            }
          }
        }
      } else {
        // Reset the counter if the button is released
        if (buttonPressed) lcd.clear();
        // if (buttonPressed) tone(BUZZER, TONEDEFFAIL, 375);
        buttonPressed = false;
        defuseStartTime = 0;
        isDefusing = false;
      }
    } else {
      mode = EXPLOSION;
      myDFPlayer.play(5);  // Play audio 5
      lcd.clear();
    }
  }

  // Buzzer logic for defusing
  if (mode == ARMED && isDefusing) {
    if (deftone) {
      myDFPlayer.play(7);  // Play audio 7
      deftone = false;
    }
    if (currentTime - previousMillis >= (beepCount == 3 ? extendedInterval : toneInterval)) {
      previousMillis = currentTime;

      if (beepCount < 3) {
        // Play the high tone
        tone(BUZZER, TONEDEF, toneInterval);  // Play tone for the duration of toneInterval
        beepCount++;
      } else {
        // Reset the beep count after the third beep
        beepCount = 0;
      }
    }
  } else {
    // Reset the beep count when not defusing
    beepCount = 0;
    deftone = true;
  }

  if (mode == EXPLOSION) {  // Explosion Mode
    analogWrite(BLUE, 0);
    analogWrite(GREEN, 255);
    analogWrite(RED, 255);

    // Flash "00:00:00" on the LCD
    if (currentTime - flashStartTime >= 500) {
      flashStartTime = currentTime;
      displayTime = !displayTime;  // Toggle the display flag
    }

    if (displayTime) {
      lcd.setCursor(4, 0);
      lcd.print("00:00:00");
    } else {
      lcd.setCursor(4, 0);
      lcd.print("        ");
    }

    lcd.setCursor(0, 1);
    lcd.print("T Win ");
    lcd.write(0);

    reset();
  }

  if (mode == DEFUSED) {  // Defused
    analogWrite(BLUE, 255);
    analogWrite(GREEN, 0);
    analogWrite(RED, 0);

    // Store the final defuse time if not already stored
    if (finalDefuseTime == 0) {
      finalDefuseTime = currentTime - startTime;
    }

    // Flash the final defuse time on the LCD
    if (currentTime - flashStartTime >= 500) {
      flashStartTime = currentTime;
      displayTime = !displayTime;  // Toggle the display flag
    }

    if (displayTime) {
      displayTimeOnLCD(finalDefuseTime);
    } else {
      lcd.setCursor(4, 0);
      lcd.print("        ");
    }

    lcd.setCursor(0, 1);
    lcd.print("CT Win ");
    lcd.write(1);
    lcd.print("        ");

    reset();
  }
}

void led_blink(bool isDefusing) {
  if (LEDinitialized) {
    if (!isDefusing) {
      analogWrite(BLUE, 0);
      analogWrite(GREEN, 0);
      analogWrite(RED, 255);
    } else {
      analogWrite(BLUE, 255);
      analogWrite(GREEN, 0);
      analogWrite(RED, 0);
    }
  }
  LEDinitialized = true;

  // Record the time when the LED was turned on
  ledBlinkStartTime = millis();
  ledState = true;
  delay(10);
}

void lcd_update(unsigned long elapsedTime) {
  // Display time left in MM:SS:MsMs format
  unsigned long timeLeft = countdownTime - elapsedTime;
  int minutes = timeLeft / 60000;
  int seconds = (timeLeft % 60000) / 1000;
  int milliseconds = (timeLeft % 1000) / 10;  // Two decimal places

  lcd.setCursor(4, 0);
  if (minutes < 10) lcd.print("0");
  lcd.print(minutes);
  lcd.print(":");
  if (seconds < 10) lcd.print("0");
  lcd.print(seconds);
  lcd.print(":");
  if (milliseconds < 10) lcd.print("0");
  lcd.print(milliseconds);
}

void displayTimeOnLCD(unsigned long elapsedTime) {
  unsigned long timeLeft = countdownTime - elapsedTime;
  int minutes = timeLeft / 60000;
  int seconds = (timeLeft % 60000) / 1000;
  int milliseconds = (timeLeft % 1000) / 10;  // Two decimal places

  lcd.setCursor(4, 0);
  if (minutes < 10) lcd.print("0");
  lcd.print(minutes);
  lcd.print(":");
  if (seconds < 10) lcd.print("0");
  lcd.print(seconds);
  lcd.print(":");
  if (milliseconds < 10) lcd.print("0");
  lcd.print(milliseconds);
}


void (*resetFunc)(void) = 0;  // declare reset function at address 0

void reset() {
  char key = keypad.getKey();

  if (key == '*') {
    LEDinitialized = false;
    finalDefuseTime = 0;
    analogWrite(BLUE, 0);
    analogWrite(GREEN, 0);
    analogWrite(RED, 0);
    Serial.println("Soft Reset");
    mode = IDLE;
  }
  if (key == '#') {
    Serial.println("Hard Reset");
    resetFunc();  // call reset
  }
}