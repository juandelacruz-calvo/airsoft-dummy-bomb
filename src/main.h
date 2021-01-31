#include <Arduino.h>

void printMainMenu();
void bigTextLine(String line, uint8_t x, uint8_t y);
void smallTextLine(String line, uint8_t x, uint8_t y);
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
char getInputIfAvailable();
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
void displayLinesInDisplay(String firstLine, uint8_t firstLineX, String secondLine, uint8_t secondLineX, String thirdLine, uint8_t thirdLineX, String forthLine, uint8_t forthLineX);
void showDefusingLinesInDisplay();
void showBombPlantedLinesInDisplay();
void showGameStartedLinesInDisplay();
void displayLedCountdown(uint8_t totalSeconds);
void displayLedNumber(uint8_t number);
void clearLedDisplay();
void updateButtonStatuses();
void (*resetFunc)(void) = 0; // Reset Arduino