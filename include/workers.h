#ifndef WORKERS_H
#define WORKERS_H

#include "game.h"

bool workers_start(Game *game);
void workers_stop(Game *game);
/* One UI request at a time. Returns false if shutdown has begun. */
bool workers_fire(Game *game, int attacker, int target, int row, int col,
                  ShotResult *result);

#endif
