#include <Arduino.h>

void printMainMenu();
void bigTextLine(String line, int x, int y);
void smallTextLine(String line, int x, int y);
void applyAction(char action);
void applyMainMenuLevelAction(char action);
void applySearchDestroyLevelAction(char action);
void applySabotageMenuLevelAction(char action);
void requestGameTime();
void requestPlantingTime();
void requestDefuseTime();
void requestBombExplosionTime();
void requestDefuseCode();
void triggerGameStart();
void countdown();
String awaitForInput();
void awaitOkCancel();
boolean inputAvailable();
char readCharacter();
void beepBomb();
void updateGameTime();
void defusingCallback();
void plantingCallback();
void explodingCallback();
void plantBombActionTrigger();
void cancelPlantingBombActionTrigger();
void cancelDefusingActionTrigger();
void defusingActionTrigger();
void stopTimers();
void displayLinesInDisplay(String firstLine, int firstLineX, String secondLine, int secondLineX, String thirdLine, int thirdLineX, String forthLine, int forthLineX);
void showDefusingLinesInDisplay();
void showBombPlantedLinesInDisplay();
void showGameStartedLinesInDisplay();
void displayLedCountdown(int totalSeconds);
void displayLedNumber(int number);
void clearLedDisplay();
void updateButtonStatuses();
void (*resetFunc)(void) = 0; // Reset Arduino