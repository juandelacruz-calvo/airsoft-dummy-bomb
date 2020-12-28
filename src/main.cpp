#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

void printMainMenu();
void bigTextLine(String line, int x, int y);
void smallTextLine(String line, int x, int y);
void applyAction(char action);
void applyMainMenuLevelAction(char action);
void applySearchDestroyLevelAction(char action);
void applySabotageMenuLevelAction(char action);
void applyDominationLevelAction(char action);
void applySetupLevelAction(char action);
void requestGameTime();
void requestDefuseTime();
void requestDefuseCode();
void triggerGameStart();
void countdown();
String awaitForInput();
void awaitOkCancel();
boolean inputAvailable();
char readCharacter();
void (*resetFunc)(void) = 0; // Reset Arduino

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

enum MenuLevel
{
  MAIN,
  SEARCH_DESTROY,
  SABOTAGE,
  DOMINATION,
  SETUP
};

MenuLevel menuLevel = MAIN;
int runLevel = 0;
int gameLength;
int disarmtimeLength;
String defuseCode;

void setup()
{
  Serial.begin(115200);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    resetFunc();
  }

  printMainMenu(); // Draw scrolling text
}

void loop()
{
  if (inputAvailable())
  {
    applyAction(readCharacter());
  }
  
  if (runLevel == 1) {
    
    // Start game
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
  display.clearDisplay();
  bigTextLine(F("1.Sear&Des"), 0, 0);
  bigTextLine(F("2.Sabotage"), 0, 16);
  bigTextLine(F("3.Dominati"), 0, 32);
  bigTextLine(F("4.Setup"), 0, 48);
}

void bigTextLine(String line, int x, int y)
{
  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(x, y);
  display.println(line);
  display.display(); // Show initial text
}

void smallTextLine(String line, int x, int y)
{
  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(WHITE);
  display.setCursor(x, y);
  display.println(line);
  display.display(); // Show initial text
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
  case DOMINATION:
    applyDominationLevelAction(action);
    break;
  case SETUP:
    applySetupLevelAction(action);
    break;
  }
}

void applyMainMenuLevelAction(char action)
{
  switch (action)
  {
  case '1':
    menuLevel = SEARCH_DESTROY;
    requestGameTime();
    // requestBombExplosionTime();
    requestDefuseTime();
    requestDefuseCode();
    triggerGameStart();
  }
}

void requestGameTime()
{
  display.clearDisplay();
  bigTextLine(F("Countdown?"), 0, 0);
  smallTextLine(F("Game length in minutes then press #, or cancel with *"), 0, 16);
  gameLength = awaitForInput().toInt();
}

void requestDefuseTime()
{
  display.clearDisplay();
  bigTextLine(F("Defuse?"), 0, 0);
  smallTextLine(F("Defuse time in seconds then press #, or cancel with *"), 0, 16);
  disarmtimeLength = awaitForInput().toInt();
}

void requestDefuseCode()
{
  display.clearDisplay();
  bigTextLine(F("Code?"), 0, 0);
  smallTextLine(F("Insert defuse code then press #, or cancel with *"), 0, 16);
  defuseCode = awaitForInput();
}

void triggerGameStart()
{
  display.clearDisplay();
  bigTextLine(F("Start?"), 0, 0);
  bigTextLine(F("# OK"), 0, 16);
  bigTextLine(F("* Cancel"), 0, 32);
  awaitOkCancel();
  countdown();
}

String awaitForInput()
{
  String input;
  do
  {

    if (inputAvailable())
    {
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
  display.clearDisplay();
  Serial.println(F("Starting game"));
  int countdownTime = 10000;
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

    int currentSecond = (endMillis - millis()) / 1000;

    if (displayedSecond != currentSecond)
    {
      displayedSecond = currentSecond;
      display.clearDisplay();
      bigTextLine(String(currentSecond), 60, 32);
    }

    // Serial.print(F("Current second: "));
    // Serial.println(currentSecond);
    // Serial.println(currentSecond == 1);

    if (currentSecond == 0)
    {
      finished = true;
    }
  }
  
  display.clearDisplay();
  bigTextLine(F("GO!"), 60, 32);
}

void applySearchDestroyLevelAction(char action)
{
}
void applySabotageMenuLevelAction(char action)
{
}
void applyDominationLevelAction(char action)
{
}
void applySetupLevelAction(char action)
{
}
