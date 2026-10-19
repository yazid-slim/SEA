#ifndef ENGINE_H
#define ENGINE_H

#include "game.h"

/* Initialize deterministic, nonoverlapping random fleets before threads start. */
bool engine_init(Game *game, int players, uint32_t seed);

/* Caller holds game->mutex once workers have started. */
ShotKind engine_validate_attack(const Game *game, int attacker, int target,
                                int row, int col);
/* Apply an accepted or rejected shot atomically; caller holds game->mutex. */
ShotResult engine_attack(Game *game, int attacker, int target, int row, int col);

#endif
