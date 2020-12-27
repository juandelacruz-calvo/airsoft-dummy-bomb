#include <Arduino.h>
#include <Ticker.h>
#include <SdFat.h>
SdFat sd;

#define SDFAT_FILE_TYPE 3
#define SD_CHIP_SELECT_PIN 10

#include <TMRpcm.h> //  also need to include this library...
#include <Keypad.h>

void plantedBomb();
void beepBomb();

const byte ROWS = 4;  //four rows
const byte COLS = 4;  //four columns
const int buzzer = 9; //buzzer to arduino pin 9

char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

byte rowPins[ROWS] = {4, 3, 2, 0}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {8, 7, 6, 5}; //connect to the column pinouts of the keypad

// //Create an object of keypad
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

TMRpcm audio; // create an object for use in this sketch
Ticker ticker(plantedBomb, 50000, 0, MILLIS);

void setup()
{
  // pinMode(buzzer, OUTPUT); // Set buzzer - pin 9 as an output

  Serial.begin(9600);
  Serial.println("inicio");
  audio.speakerPin = buzzer; //5,6,11 or 46 on Mega, 9 on Uno, Nano, etc

  if (!sd.begin(SD_CHIP_SELECT_PIN))
  {
    Serial.println("SD error");
    return;
  }
  else
  {
    Serial.println("SD OK");
  }

  ticker.start();
}

void loop()
{
  ticker.update();
  Serial.println("loop");
 
  delay(5000);       // ...for 1s

  char key = keypad.getKey(); // Read the key

  // Print if key pressed
  if (key)
  {
    Serial.print("Key Pressed : ");
    Serial.println(key);
  }
}

void plantedBomb()
{
  Serial.println("timer");
  audio.play("planted.wav");
}

void beepBomb()
{
 tone(buzzer, 4435); // Send 1KHz sound signal...
  delay(80);          // ...for 1 sec
  noTone(buzzer);     // Stop sound...
}
