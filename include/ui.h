#ifndef UI_H
#define UI_H

#include "game.h"

/* Only the main thread calls these functions. */
void ui_show_turn(Game *game);
void ui_show_result(const ShotResult *result);
/* 1 = parsed, 0 = EOF, -1 = invalid line. Player numbers are 1-based. */
int ui_read_attack(int *target, int *row, int *col);

#endif
