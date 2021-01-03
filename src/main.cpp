#include <main.h>
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#include <Ticker.h>
#include <TM1637.h>

#define DEBUG true

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

TM1637 led4DigitDisplay(LED_SCREEN_CLK_PIN, LED_SCREEN_DIO_PIN);

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
int gameLengthMinutes;
int defusingTimeLengthSeconds;
int plantingTimeLengthSeconds;
int explosionTimeLengthMinutes;
String defuseCode;

long millisGameFinish;
long millisDefuseFinish;
long millisPlantingFinish;
long millisExplosionFinish;

int defuseButtonPushed = 0;
int plantButtonPushed = 0;

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(DEFUSE_BUTTON_PIN, INPUT);
  pinMode(PLANT_BUTTON_PIN, INPUT);

#if DEBUG
  Serial.begin(115200);
  Serial.println(F("Initialisating"));
#endif

  display.begin(&Adafruit128x64, 0x3C);
  display.setFont(Adafruit5x7);

  led4DigitDisplay.set();
  led4DigitDisplay.init();

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
  if (inputAvailable())
  {
    applyAction(readCharacter());
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

  int defuseValue = digitalRead(DEFUSE_BUTTON_PIN);
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

  int planting = digitalRead(PLANT_BUTTON_PIN);
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

boolean inputAvailable()
{
  return Serial.available() > 0;
}

char readCharacter()
{
  char obtained = Serial.read();
  Serial.print(obtained);
  return obtained;
}

void printMainMenu()
{
  menuLevel = MAIN;
  displayLinesInDisplay(F("1.Search &"), 0, F("Destroy"), 15, F(""), 50, F("2.Sabotage"), 0);
}

void bigTextLine(String line, int x, int y)
{
  display.set2X();
  display.setCursor(x, y);
  display.println(line);
}

void smallTextLine(String line, int x, int y)
{
  display.set1X();
  display.setCursor(x, y);
  display.println(line);
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
  gameLengthMinutes = awaitForInput().toInt();
}

void requestDefuseTime()
{
  displayLinesInDisplay(F("Defuse time"), 0, F("in seconds?"), 0, F("#-> OK"), 0, F("*-> Cancel"), 0);
  defusingTimeLengthSeconds = awaitForInput().toInt();
}

void requestPlantingTime()
{
  displayLinesInDisplay(F("Bomb Plant"), 0, F("in seconds?"), 0, F("#-> OK"), 0, F("*-> Cancel"), 0);
  plantingTimeLengthSeconds = awaitForInput().toInt();
}

void requestDefuseCode()
{
  displayLinesInDisplay(F("Defusing"), 0, F("code?"), 0, F("#-> OK"), 0, F("*-> Cancel"), 0);
  defuseCode = awaitForInput();
}

void requestBombExplosionTime()
{
  displayLinesInDisplay(F("Bomb time"), 0, F("in minutes?"), 0, F("#-> OK"), 0, F("*-> Cancel"), 0);
  explosionTimeLengthMinutes = awaitForInput().toInt();
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

    if (inputAvailable())
    {
      audio.play("nvg_off-15db.wav");
      char read = readCharacter();

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
      displayLedNumber(input.toInt());
    }
  } while (true);
}

void awaitOkCancel()
{
  do
  {
    if (inputAvailable())
    {
      audio.play("nvg_off-15db.wav");
      char read = readCharacter();

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

  int countdownTime = 10000;
  int initialMillis = millis();
  int endMillis = initialMillis + countdownTime;
  int displayedSecond = 10;
  int currentSecond;
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

  display.clear();
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
    int timeLeft = (millisGameFinish - millis()) / 1000L;
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

void displayLinesInDisplay(String firstLine, int firstLineX, String secondLine, int secondLineX, String thirdLine, int thirdLineX, String forthLine, int forthLineX)
{
  display.clear();
  bigTextLine(firstLine, firstLineX, 0);
  bigTextLine(secondLine, secondLineX, 16);
  bigTextLine(thirdLine, thirdLineX, 32);
  bigTextLine(forthLine, forthLineX, 48);
}

void displayLedCountdown(int totalSeconds)
{
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;

  led4DigitDisplay.clearDisplay();
  led4DigitDisplay.point(1);
  led4DigitDisplay.display(3, seconds % 10);
  led4DigitDisplay.display(2, seconds / 10 % 10);
  led4DigitDisplay.display(1, minutes % 10);
  led4DigitDisplay.display(0, minutes / 10 % 10);
}

void displayLedNumber(int number)
{
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
}

void clearLedDisplay()
{
  led4DigitDisplay.point(0);
  led4DigitDisplay.clearDisplay();
}

void defusingCallback()
{
  if (runlevel == DEFUSING)
  {
    int timeLeft = (millisDefuseFinish - millis()) / 1000L;
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
    int timeLeft = (millisPlantingFinish - millis()) / 1000L;

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
