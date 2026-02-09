#ifndef TECLADO_H
#define TECLADO_H

#include "pico/stdlib.h"

#define ROWSPIN 4
#define COLSPIN 4

extern uint8_t rowPins[ROWSPIN];
extern uint8_t colPins[COLSPIN];
extern char keyMap[ROWSPIN][COLSPIN];

void teclado_init();
char teclado_read();

#endif