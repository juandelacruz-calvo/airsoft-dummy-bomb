#include <main.h>
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#include <Ticker.h>
#include <TM1637.h>
#include <Keypad.h>

#define DEBUG true
#define DISPLAY_CONNECTED true
#define LED_DISPLAY_CONNECTED true

#if DEBUG
#include "MemoryFree.h"
#endif

#include <SdFat.h>

SdFat sd;

#include <TMRpcm.h>

enum MenuLevel
{
  MAIN,
  SEARCH_DESTROY,
  SABOTAGE,
};

enum Runtime
{
  PLAYING,
  PLANTING,
  PLANTED,
  EXPLODED,
  DEFUSING,
  DEFUSED,
  TIME_OVER,
  END
};

#define SDFAT_FILE_TYPE 3

#define SD_CHIP_SELECT_PIN 10
#define SPEAKER_PIN 9
#define LED_SCREEN_CLK_PIN 2
#define LED_SCREEN_DIO_PIN 3
#define DEFUSE_BUTTON_PIN A1
#define PLANT_BUTTON_PIN A2

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};
byte rowPins[ROWS] = {15, 8, 7, 6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 4, 14};    //connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

#if LED_DISPLAY_CONNECTED
TM1637 led4DigitDisplay(LED_SCREEN_CLK_PIN, LED_SCREEN_DIO_PIN);
#endif

TMRpcm audio; // create an object for use in this sketch
Ticker beepBombTicker(beepBomb, 3000, 0, MILLIS);
Ticker updateGameTimeTicker(updateGameTime, 1000, 0, MILLIS);
Ticker defusingTicker(defusingCallback, 1000, 0, MILLIS);
Ticker plantingTicker(plantingCallback, 1000, 0, MILLIS);
Ticker explodingTicker(explodingCallback, 1000, 0, MILLIS);

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
SSD1306AsciiAvrI2c display;

MenuLevel menuLevel = MAIN;
Runtime runlevel;
boolean bombBeep = false;
uint8_t gameLengthMinutes;
uint8_t defusingTimeLengthSeconds;
uint8_t plantingTimeLengthSeconds;
uint8_t explosionTimeLengthMinutes;
String defuseCode;

long millisGameFinish;
long millisDefuseFinish;
long millisPlantingFinish;
long millisExplosionFinish;

uint8_t defuseButtonPushed = 0;
uint8_t plantButtonPushed = 0;

void setup()
{
#if DEBUG
  Serial.begin(115200);
  Serial.println(F("Initialisating"));
#endif

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(DEFUSE_BUTTON_PIN, INPUT);
  pinMode(PLANT_BUTTON_PIN, INPUT);

#if DISPLAY_CONNECTED
  display.begin(&Adafruit128x64, 0x3C);
  display.setFont(Adafruit5x7);
#endif

#if LED_DISPLAY_CONNECTED
  led4DigitDisplay.set();
  led4DigitDisplay.init();
#endif

#if DEBUG
  Serial.println(freeMemory(), DEC);
#endif

  audio.speakerPin = SPEAKER_PIN;

  if (!sd.begin(SD_CHIP_SELECT_PIN))
  {

#if DEBUG
    Serial.println(F("SD error"));
#endif

    return;
  }
  else
  {

#if DEBUG
    Serial.println(F("SD OK"));
#endif
  }

#if DEBUG
  Serial.println(freeMemory(), DEC);
#endif

  audio.play("enemydown-15db.wav");
  printMainMenu();
}

void loop()
{
  char read = getInputIfAvailable();
  if (read != NO_KEY)
  {
    applyAction(read);
  }

  updateButtonStatuses();

  beepBombTicker.update();
  updateGameTimeTicker.update();
  defusingTicker.update();
  explodingTicker.update();
  plantingTicker.update();

  switch (runlevel)
  {

  case TIME_OVER:

    displayLinesInDisplay(F(""), 0, F("TIME OVER"), 10, F(""), 20, F(""), 30);

    runlevel = END;
    break;

  case DEFUSED:

    displayLinesInDisplay(F(""), 0, F("Counter"), 30, F("WIN"), 50, F(""), 30);
    audio.play("bombdef-15db.wav");
    delay(2500);
    audio.play("ctwin-15.wav");

    runlevel = END;
    break;

  case EXPLODED:
    displayLinesInDisplay(F(""), 0, F("Terrorist"), 10, F("WIN"), 50, F(""), 30);
    audio.play("new_bomb_explosion-5db.wav");
    delay(3000);
    audio.play("terwin-15.wav");

    runlevel = END;
    break;
  }
}

void updateButtonStatuses()
{

  uint8_t defuseValue = digitalRead(DEFUSE_BUTTON_PIN);
  if (defuseValue != defuseButtonPushed)
  {
    defuseButtonPushed = defuseValue;

    if (defuseButtonPushed)
    {
      defusingActionTrigger();
    }
    else
    {
      cancelDefusingActionTrigger();
    }
  }

  uint8_t planting = digitalRead(PLANT_BUTTON_PIN);
  if (planting != plantButtonPushed)
  {
    plantButtonPushed = planting;

    if (plantButtonPushed)
    {
      plantBombActionTrigger();
    }
    else
    {
      cancelPlantingBombActionTrigger();
    }
  }
}

char getInputIfAvailable()
{

  if (keypad.getKeys())
  {
    for (uint8_t i = 0; i < LIST_MAX; i++) // Scan the whole key list.
    {
      if (keypad.key[i].stateChanged) // Only find keys that have changed state.
      {
        if (keypad.key[i].kstate == PRESSED)
        {
#if DEBUG
          Serial.print("Keypad key pressed: ");
          Serial.println(keypad.key[i].kchar);
#endif

          return keypad.key[i].kchar;
        }
      }
    }
  }

  return NO_KEY;
}

void printMainMenu()
{
  menuLevel = MAIN;
  displayLinesInDisplay(F("1.Search &"), 0, F("Destroy"), 15, F(""), 50, F("2.Sabotage"), 0);
}

void bigTextLine(String line, uint8_t x, uint8_t y)
{
#if DISPLAY_CONNECTED
  display.set2X();
  display.setCursor(x, y);
  display.println(line);
#endif

#if DEBUG
  Serial.println(line);
#endif
}

void smallTextLine(String line, uint8_t x, uint8_t y)
{
#if DISPLAY_CONNECTED
  display.set1X();
  display.setCursor(x, y);
  display.println(line);
#endif

#if DEBUG
  Serial.println(line);
#endif
}

void applyAction(char action)
{
  audio.play("nvg_off-15db.wav");
  switch (menuLevel)
  {
  case MAIN:
    applyMainMenuLevelAction(action);
    break;
  case SEARCH_DESTROY:
    applySearchDestroyLevelAction(action);
    break;
  case SABOTAGE:
    applySabotageMenuLevelAction(action);
    break;
  }
}

void applyMainMenuLevelAction(char action)
{
  switch (action)
  {
  case '1':
    menuLevel = SEARCH_DESTROY;
    requestBombExplosionTime();
    requestDefuseTime();
    triggerGameStart();
    runlevel = PLANTED;
    showBombPlantedLinesInDisplay();
    bombBeep = true;
    audio.play("bombpl-15db.wav");
    delay(1500);
    millisExplosionFinish = (explosionTimeLengthMinutes * 60L * 1000L) + millis();
    explodingTicker.start();
    beepBombTicker.start();
    break;

  case '2':
    menuLevel = SABOTAGE;
    requestGameTime();
    requestPlantingTime();
    requestBombExplosionTime();
    requestDefuseTime();
    triggerGameStart();

    runlevel = PLAYING;

    millisGameFinish = (gameLengthMinutes * 60L * 1000L) + millis();
    bombBeep = true;
    updateGameTimeTicker.start();
    beepBombTicker.start();
    showGameStartedLinesInDisplay();
    break;
  }
}

void requestGameTime()
{
  displayLinesInDisplay(F("Game Length"), 0, F("in minutes?"), 0, F("#-> OK"), 0, F("*-> Cancel"), 0);
  gameLengthMinutes = (uint8_t)awaitForInput().toInt();
}

void requestDefuseTime()
{
  displayLinesInDisplay(F("Defuse time"), 0, F("in seconds?"), 0, F("#-> OK"), 0, F("*-> Cancel"), 0);
  defusingTimeLengthSeconds = (uint8_t)awaitForInput().toInt();
}

void requestPlantingTime()
{
  displayLinesInDisplay(F("Bomb Plant"), 0, F("in seconds?"), 0, F("#-> OK"), 0, F("*-> Cancel"), 0);
  plantingTimeLengthSeconds = (uint8_t)awaitForInput().toInt();
}

void requestDefuseCode()
{
  displayLinesInDisplay(F("Defusing"), 0, F("code?"), 0, F("#-> OK"), 0, F("*-> Cancel"), 0);
  defuseCode = awaitForInput();
}

void requestBombExplosionTime()
{
  displayLinesInDisplay(F("Bomb time"), 0, F("in minutes?"), 0, F("#-> OK"), 0, F("*-> Cancel"), 0);
  explosionTimeLengthMinutes = (uint8_t)awaitForInput().toInt();
}

void triggerGameStart()
{
  displayLinesInDisplay(F("Start game?"), 0, F(""), 0, F("#-> OK"), 0, F("*-> Cancel"), 0);
  awaitOkCancel();
  countdown();

#if DEBUG
  Serial.println(freeMemory(), DEC);

  Serial.print(F("Millis game finish: "));
  Serial.println(millisGameFinish);

  Serial.print(F("Game length "));
  Serial.println(gameLengthMinutes);

  Serial.print(F("(gameLength * 60) "));
  Serial.println((gameLengthMinutes * 60));

#endif

  audio.play("com_go-15.wav");
}

String awaitForInput()
{
  String input;
  do
  {
    char read = getInputIfAvailable();
    if (read != NO_KEY)
    {
      audio.play("nvg_off-15db.wav");

      if (read == '*')
      {
        resetFunc();
      }
      else if (read == '#')
      {
        Serial.print(F("Input read: "));
        Serial.println(input);
        clearLedDisplay();
        return input;
      }

      input += read;
      displayLedNumber((uint8_t)input.toInt());
    }

  } while (true);
}

void awaitOkCancel()
{
  do
  {
    char read = getInputIfAvailable();
    if (read != NO_KEY)
    {
      audio.play("nvg_off-15db.wav");

      if (read == '*')
      {
        resetFunc();
      }
      else if (read == '#')
      {
        return;
      }
    }

  } while (true);
}

void countdown()
{

#if DEBUG
  Serial.println(F("Starting game"));

#endif

  uint8_t countdownTime = 10000;
  unsigned long initialMillis = millis();
  unsigned long endMillis = initialMillis + countdownTime;
  uint8_t displayedSecond = 10;
  uint8_t currentSecond;
  boolean finished = false;

  displayLinesInDisplay(F(""), 0, F("Starting"), 10, F("game"), 40, F(""), 0);
  while (!finished)
  {

    if (millis() - initialMillis > countdownTime)
    {
      return;
    }

#if DEBUG
    Serial.print(F("millis "));
    Serial.println(millis());
#endif

    int currentSecond = (endMillis - millis()) / 1000;

    if (displayedSecond != currentSecond)
    {
      displayedSecond = currentSecond;
      displayLedCountdown(currentSecond);
    }

#if DEBUG
    Serial.print(F("Current second: "));
    Serial.println(currentSecond);
    Serial.print(F("displayedSecond: "));
    Serial.println(displayedSecond);
#endif

    if (currentSecond == 0)
    {
      finished = true;
    }
  }

#if DISPLAY_CONNECTED
  display.clear();
#endif

  bigTextLine(F(""), 0, 0);
  bigTextLine(F("GO!"), 55, 32);
}

void beepBomb()
{
  if (bombBeep && runlevel == PLANTED)
  {
    tone(SPEAKER_PIN, 4186, 120); // C8
  }
}

void updateGameTime()
{
  if (runlevel == PLAYING)
  {
    unsigned long timeLeft = (millisGameFinish - millis()) / 1000L;
    if (timeLeft > 0)
    {
      displayLedCountdown(timeLeft);
    }
    else
    {
      stopTimers();
      runlevel = TIME_OVER; // The game is over¨
    }
  }
}

void displayLinesInDisplay(String firstLine, uint8_t firstLineX, String secondLine, uint8_t secondLineX, String thirdLine, uint8_t thirdLineX, String forthLine, uint8_t forthLineX)
{
#if DISPLAY_CONNECTED
  display.clear();
#endif

  bigTextLine(firstLine, firstLineX, 0);
  bigTextLine(secondLine, secondLineX, 16);
  bigTextLine(thirdLine, thirdLineX, 32);
  bigTextLine(forthLine, forthLineX, 48);
}

void displayLedCountdown(uint8_t totalSeconds)
{
  uint8_t minutes = totalSeconds / 60;
  uint8_t seconds = totalSeconds % 60;

#if DEBUG
  Serial.print("Led countdown: ");
  Serial.print(minutes);
  Serial.print(":");
  Serial.println(seconds);
#endif

#if LED_DISPLAY_CONNECTED
  led4DigitDisplay.clearDisplay();
  led4DigitDisplay.point(1);
  led4DigitDisplay.display(3, seconds % 10);
  led4DigitDisplay.display(2, seconds / 10 % 10);
  led4DigitDisplay.display(1, minutes % 10);
  led4DigitDisplay.display(0, minutes / 10 % 10);
#endif
}

void displayLedNumber(uint8_t number)
{

#if LED_DISPLAY_CONNECTED
  led4DigitDisplay.clearDisplay();
  led4DigitDisplay.point(0);
  led4DigitDisplay.display(3, number % 10);

  if (number > 9)
  {
    led4DigitDisplay.display(2, number / 10 % 10);
  }

  if (number > 99)
  {
    led4DigitDisplay.display(1, number / 100 % 10);
  }

  if (number > 999)
  {
    led4DigitDisplay.display(0, number / 1000 % 10);
  }

#endif

#if DEBUG
  Serial.print("Led number: ");
  Serial.println(number);
#endif
}

void clearLedDisplay()
{
#if LED_DISPLAY_CONNECTED
  led4DigitDisplay.point(0);
  led4DigitDisplay.clearDisplay();
#endif
}

void defusingCallback()
{
  if (runlevel == DEFUSING)
  {
    unsigned long timeLeft = (millisDefuseFinish - millis()) / 1000L;
    if (timeLeft > 0)
    {
      displayLedCountdown(timeLeft);
    }
    else
    {
      audio.play("c4_disarmed-15db.wav");
      delay(250);
      runlevel = DEFUSED; // The game is over
      stopTimers();
    }
  }
}

void plantingCallback()
{
  if (runlevel == PLANTING)
  {
    unsigned long timeLeft = (millisPlantingFinish - millis()) / 1000L;

#if DEBUG
    Serial.print(F("planting timeleft "));
    Serial.println(timeLeft);
    Serial.print(F("millisPlantingFinish  "));
    Serial.println(millisPlantingFinish);
#endif

    if (timeLeft > 0)
    {
      displayLedCountdown(timeLeft);
    }
    else
    {
      audio.play("c4_plant-15db.wav");
      delay(160);
      runlevel = PLANTED;
      millisExplosionFinish = millis() + (explosionTimeLengthMinutes * 60L * 1000L);
      showBombPlantedLinesInDisplay();

#if DEBUG
      Serial.print(F("millisExplosionFinish "));
      Serial.println(millisExplosionFinish);
#endif
      plantingTicker.stop();
      explodingTicker.start();
      audio.play("bombpl-15db.wav");
    }
  }
}

void explodingCallback()
{
  if (runlevel == PLANTED)
  {

    int timeLeft = (millisExplosionFinish - millis()) / 1000L;

#if DEBUG
    Serial.print(F("millisExplosionFinish "));
    Serial.println(millisExplosionFinish);

    Serial.print(F("timeLeft "));
    Serial.println(timeLeft);
#endif

    if (timeLeft >= 15 && timeLeft <= 30)
    {
      beepBombTicker.interval(1000);
    }
    else if (timeLeft > 5 && timeLeft <= 15)
    {
      beepBombTicker.interval(500);
    }
    else if (timeLeft <= 5)
    {
      beepBombTicker.interval(250);
    }

    if (timeLeft > 0)
    {
      displayLedCountdown(timeLeft);
    }
    else
    {

#if DEBUG
      Serial.println(F("Exploded"));
#endif

      runlevel = EXPLODED; // The game is over
      stopTimers();
    }
  }
}

void plantBombActionTrigger()
{
  if (runlevel == PLAYING)
  {

#if DEBUG
    Serial.println(F("Planting the bomb"));
#endif

    runlevel = PLANTING;
    audio.play("c4_disarm-15db.wav");
    millisPlantingFinish = millis() + (plantingTimeLengthSeconds * 1000L);
    plantingTicker.start();
    displayLinesInDisplay(F("Planting"), 10, F("the bomb"), 20, F("Please"), 30, F("wait"), 40);
  }
}

void defusingActionTrigger()
{
  if (runlevel == PLANTED)
  {

#if DEBUG
    Serial.println(F("Defusing"));
#endif

    runlevel = DEFUSING;
    audio.play("c4_disarm-15db.wav");
    millisDefuseFinish = millis() + (defusingTimeLengthSeconds * 1000L);
    showDefusingLinesInDisplay();
    defusingTicker.start();
  }
}

void showDefusingLinesInDisplay()
{
  displayLinesInDisplay(F("Defusing"), 15, F("bomb."), 40, F("Please"), 30, F("wait"), 40);
}

void showBombPlantedLinesInDisplay()
{
  displayLinesInDisplay(F("The Bomb "), 15, F("has been"), 15, F("planted"), 15, F(""), 0);
}

void showGameStartedLinesInDisplay()
{
  displayLinesInDisplay(F("Game"), 40, F("running"), 20, F("Plant"), 30, F("the bomb"), 15);
}

void cancelDefusingActionTrigger()
{
  if (runlevel == DEFUSING)
  {
#if DEBUG
    Serial.println(F("Cancel defusing"));
#endif

    runlevel = PLANTED;
    defusingTicker.stop();
    showBombPlantedLinesInDisplay();
  }
}

void cancelPlantingBombActionTrigger()
{
  if (runlevel == PLANTING)
  {
#if DEBUG
    Serial.println(F("Cancel planting"));
#endif

    runlevel = PLAYING;
    plantingTicker.stop();
    showGameStartedLinesInDisplay();
  }
}

void stopTimers()
{
  clearLedDisplay();
  updateGameTimeTicker.stop();
  beepBombTicker.stop();
  defusingTicker.stop();
  explodingTicker.stop();
}

void applySearchDestroyLevelAction(char action)
{
  if (action == 'd')
  {
    defusingActionTrigger();
  }
  else if (action == 'c')
  {
    cancelDefusingActionTrigger();
  }
}

void applySabotageMenuLevelAction(char action)
{

  if (action == 'p')
  {
    plantBombActionTrigger();
  }
  else if (action == 'n')
  {
    cancelPlantingBombActionTrigger();
  }
  else if (action == 'd')
  {
    defusingActionTrigger();
  }
  else if (action == 'c')
  {
    cancelDefusingActionTrigger();
  }
}
