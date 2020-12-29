#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#include <Ticker.h>

#define DEBUG true

#if DEBUG
#include "MemoryFree.h"
#endif

#include <SdFat.h>

SdFat sd;

#include <TMRpcm.h>

void printMainMenu();
void bigTextLine(String line, int x, int y);
void smallTextLine(String line, int x, int y);
void applyAction(char action);
void applyMainMenuLevelAction(char action);
void applySearchDestroyLevelAction(char action);
void applySabotageMenuLevelAction(char action);
void requestGameTime();
void requestDefuseTime();
void requestBombExplosionTime();
void requestDefuseCode();
void triggerGameStart();
void countdown();
String awaitForInput();
void awaitOkCancel();
boolean inputAvailable();
char readCharacter();
void playBombHasBeenPlanted();
void beepBomb();
void noC4BombTone();
void updateGameTime();
void defusingCallback();
void explodingCallback();
void plantBombActionTrigger();
void cancelDefusingActionTrigger();
void defusingActionTrigger();
void stopTimers();
void displayCountdown(int timeLeft, String firstLine, int firstLineX, String secondLine, int secondLineX);
void (*resetFunc)(void) = 0; // Reset Arduino

enum MenuLevel
{
  MAIN,
  SEARCH_DESTROY,
  SABOTAGE,
};

enum Runtime
{
  PLAYING,
  PLANTED,
  EXPLODED,
  DEFUSING,
  DEFUSED,
  TIME_OVER,
  END
};

#define SDFAT_FILE_TYPE 3
#define SD_CHIP_SELECT_PIN 10
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SPEAKER_PIN 9    // OLED display height, in pixels

TMRpcm audio; // create an object for use in this sketch
Ticker beepBombTicker(beepBomb, 3000, 0, MILLIS);
Ticker noToneBombTicker(noC4BombTone, 3000, 0, MILLIS);
Ticker updateGameTimeTicker(updateGameTime, 1000, 0, MILLIS);
Ticker defusingTicker(defusingCallback, 1000, 0, MILLIS);
Ticker explodingTicker(explodingCallback, 1000, 0, MILLIS);

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
SSD1306AsciiAvrI2c display;

MenuLevel menuLevel = MAIN;
Runtime runlevel;
boolean bombBeep = false;
int gameLengthMinutes;
int defusingTimeLengthSeconds;
int explosionTimeLengthMinutes;
String defuseCode;

long millisGameFinish;
long millisDefuseFinish;
long millisExplosionFinish;

void setup()
{

#if DEBUG
  Serial.begin(115200);
#endif

  display.begin(&Adafruit128x64, 0x3C);
  display.setFont(Adafruit5x7);

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

  beepBombTicker.update();
  noToneBombTicker.update();
  updateGameTimeTicker.update();
  defusingTicker.update();
  explodingTicker.update();

  switch (runlevel)
  {

  case TIME_OVER:
    display.clear();
    bigTextLine(F(""), 10, 20);
    bigTextLine(F("TIME OVER"), 10, 20);
    runlevel = END;
    break;

  case DEFUSED:
    display.clear();
    bigTextLine(F(""), 10, 20);
    bigTextLine(F("Counter"), 30, 20);
    bigTextLine(F("WIN"), 50, 20);
    noC4BombTone();
    audio.play("bombdef-15db.wav");
    delay(2500);
    audio.play("ctwin-15.wav");

    runlevel = END;
    break;

  case EXPLODED:
    display.clear();
    bigTextLine(F(""), 10, 20);
    bigTextLine(F("Terrorist"), 10, 20);
    bigTextLine(F("WIN"), 50, 20);

    audio.play("c4_explode1-5db.wav");
    delay(3000);
    audio.play("terwin-15.wav");

    runlevel = END;
    break;
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
  display.clear();
  bigTextLine(F("1.Search &"), 0, 0);
  bigTextLine(F("  Destroy"), 0, 16);
  bigTextLine(F(""), 0, 32);
  bigTextLine(F("2.Sabotage"), 0, 48);
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
    playBombHasBeenPlanted();
    runlevel = PLANTED;

    millisExplosionFinish = (explosionTimeLengthMinutes * 60L * 1000L) + millis();
    bombBeep = true;
    explodingTicker.start();
    beepBombTicker.start();
    delay(128);
    noToneBombTicker.start();

    break;

  case '2':
    menuLevel = SABOTAGE;
    requestGameTime();
    requestBombExplosionTime();
    requestDefuseTime();
    triggerGameStart();

    runlevel = PLAYING;

    millisGameFinish = (gameLengthMinutes * 60L * 1000L) + millis();
    bombBeep = true;
    updateGameTimeTicker.start();
    beepBombTicker.start();
    delay(128);
    noToneBombTicker.start();
    break;
  }
}

void requestGameTime()
{
  display.clear();
  bigTextLine(F("Time Length"), 0, 0);
  smallTextLine(F("Game length minutes"), 0, 16);
  smallTextLine(F("#->OK, *-> Cancel"), 0, 16);
  gameLengthMinutes = awaitForInput().toInt();
}

void requestDefuseTime()
{
  display.clear();
  bigTextLine(F("Defuse?"), 0, 0);
  smallTextLine(F("Defuse time in seconds"), 0, 16);
  smallTextLine(F("#->OK, *-> Cancel"), 0, 16);
  defusingTimeLengthSeconds = awaitForInput().toInt();
}

void requestDefuseCode()
{
  display.clear();
  bigTextLine(F("Code?"), 0, 0);
  smallTextLine(F("Defusing code"), 0, 16);
  smallTextLine(F("#->OK, *-> Cancel"), 0, 16);
  defuseCode = awaitForInput();
}

void requestBombExplosionTime()
{
  display.clear();
  bigTextLine(F("Bomb time?"), 0, 0);
  smallTextLine(F("Time to explode"), 0, 16);
  smallTextLine(F("#->OK, *-> Cancel"), 0, 16);
  explosionTimeLengthMinutes = awaitForInput().toInt();
}

void triggerGameStart()
{
  display.clear();
  bigTextLine(F("Start?"), 0, 0);
  bigTextLine(F("# OK"), 0, 16);
  bigTextLine(F("* Cancel"), 0, 32);
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
        return input;
      }

      input += read;
      display.clear();
      bigTextLine(input, 0, 48);
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

  display.clear();
  int countdownTime = 5000;
  int initialMillis = millis();
  int endMillis = initialMillis + countdownTime;
  int displayedSecond = 10;
  int currentSecond;
  boolean finished = false;
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
      display.clear();
      bigTextLine(F(""), 0, 0);
      bigTextLine(String(currentSecond), 60, 32);
    }

#if DEBUG
    Serial.print(F("Current second: "));
    Serial.println(currentSecond);
    Serial.println(currentSecond == 1);
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

void playBombHasBeenPlanted()
{
#if DEBUG
  Serial.println(F("Bomb has been planted"));
#endif
  audio.play("bombpl-15db.wav");
  delay(1500);
}

void beepBomb()
{
  if (bombBeep && runlevel == PLANTED)
  {
    tone(SPEAKER_PIN, 4186); // C8
  }
}

void noC4BombTone()
{
  if (bombBeep)
  {
    noTone(SPEAKER_PIN); // Stop sound...
  }
}

void updateGameTime()
{
  if (runlevel == PLAYING)
  {
    int timeLeft = (millisGameFinish - millis()) / 1000L;
    if (timeLeft > 0)
    {
      displayCountdown(timeLeft, F("Game"), 40, F("time"), 40);
    }
    else
    {
      stopTimers();
      runlevel = TIME_OVER; // The game is over¨
    }
  }
}

void displayCountdown(int timeLeft, String firstLine, int firstLineX, String secondLine, int secondLineX)
{
  display.clear();
  bigTextLine(firstLine, firstLineX, 0);
  bigTextLine(secondLine, secondLineX, 16);
  bigTextLine(F(""), 40, 32);

  char displayArray[6];

  int minutesLeft = timeLeft / 60L;
  int secondsLeft = timeLeft % 60L;

  sprintf(displayArray, "%02d:%02d", minutesLeft, secondsLeft);

  bigTextLine(displayArray, 40, 48);

#if DEBUG
  Serial.println(freeMemory(), DEC);
#endif
}

void defusingCallback()
{
  if (runlevel == DEFUSING)
  {
    int timeLeft = (millisDefuseFinish - millis()) / 1000L;
    if (timeLeft > 0)
    {
      displayCountdown(timeLeft, F(""), 40, F("Defusing"), 20);
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

    if (timeLeft > 0)
    {
      displayCountdown(timeLeft, F("Explosion"), 20, F("in"), 60);
    }
    else
    {

#if DEBUG
      Serial.println(F("Exploded "));
#endif

      runlevel = EXPLODED; // The game is over
      stopTimers();
    }
  }
}

void plantBombActionTrigger()
{

#if DEBUG
  Serial.println(F("Planting"));
#endif

  runlevel = PLANTED;
  millisExplosionFinish = millis() + (explosionTimeLengthMinutes * 60L * 1000L);

#if DEBUG
  Serial.print(F("millisExplosionFinish "));
  Serial.println(millisExplosionFinish);
#endif

  explodingTicker.start();
  audio.play("c4_plant-15db.wav");
  delay(150);
  audio.play("bombpl-15db.wav");
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
    defusingTicker.start();
  }
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
  }
}

void stopTimers()
{
  updateGameTimeTicker.stop();
  beepBombTicker.stop();
  noToneBombTicker.stop();
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
  else if (action == 'd')
  {
    defusingActionTrigger();
  }
  else if (action == 'c')
  {
    cancelDefusingActionTrigger();
  }
}
