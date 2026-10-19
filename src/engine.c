#include "engine.h"

#include <string.h>

static const int lengths[FLEET_SIZE] = {5, 4, 3, 3, 2};
static const char *names[FLEET_SIZE] = {
    "Porte-avions", "Croiseur", "Sous-marin A", "Sous-marin B", "Patrouilleur"
};

/* Small local PRNG: avoids shared rand() state and supports reproducible games. */
static uint32_t next_random(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static bool can_place(const Board *board, int row, int col, int length, bool vertical)
{
    for (int i = 0; i < length; ++i) {
        int r = row + (vertical ? i : 0);
        int c = col + (vertical ? 0 : i);
        if (r >= BOARD_SIZE || c >= BOARD_SIZE || board->cells[r][c] != 0)
            return false;
    }
    return true;
}

static bool place_fleet(Board *board, uint32_t *rng)
{
    for (int ship = 0; ship < FLEET_SIZE; ++ship) {
        board->ships[ship] = (Ship){names[ship], lengths[ship], lengths[ship]};
        bool placed = false;
        for (int attempt = 0; attempt < 10000 && !placed; ++attempt) {
            bool vertical = (next_random(rng) & 1U) != 0;
            int row = (int)(next_random(rng) % BOARD_SIZE);
            int col = (int)(next_random(rng) % BOARD_SIZE);
            if (!can_place(board, row, col, lengths[ship], vertical))
                continue;
            for (int i = 0; i < lengths[ship]; ++i)
                board->cells[row + (vertical ? i : 0)][col + (vertical ? 0 : i)] =
                    (uint8_t)(ship + 1);
            board->segments_remaining += lengths[ship];
            placed = true;
        }
        if (!placed)
            return false;
    }
    return true;
}

bool engine_init(Game *game, int players, uint32_t seed)
{
    if (!game || players < 2 || players > MAX_PLAYERS)
        return false;
    memset(game, 0, sizeof(*game));
    game->player_count = players;
    game->alive_count = players;
    game->winner = -1;
    uint32_t rng = seed ? seed : 0x9e3779b9U;
    for (int i = 0; i < players; ++i) {
        game->players[i].id = i;
        game->players[i].alive = true;
        if (!place_fleet(&game->players[i].board, &rng))
            return false;
    }
    return true;
}

ShotKind engine_validate_attack(const Game *game, int attacker, int target,
                                int row, int col)
{
    if (!game || game->winner >= 0 || attacker < 0 || attacker >= game->player_count ||
        target < 0 || target >= game->player_count || attacker == target ||
        !game->players[attacker].alive || !game->players[target].alive ||
        game->current != attacker || row < 0 || row >= BOARD_SIZE ||
        col < 0 || col >= BOARD_SIZE)
        return SHOT_INVALID;
    if (game->players[target].board.shots[row][col])
        return SHOT_REPEAT;
    return SHOT_MISS; /* Valid request; outcome is determined by engine_attack. */
}

ShotResult engine_attack(Game *game, int attacker, int target, int row, int col)
{
    ShotResult result = {SHOT_INVALID, attacker, target, row, col, -1};
    ShotKind valid = engine_validate_attack(game, attacker, target, row, col);
    if (valid != SHOT_MISS) {
        result.kind = valid;
        return result;
    }

    Player *defender = &game->players[target];
    Board *board = &defender->board;
    board->shots[row][col] = true;
    uint8_t cell = board->cells[row][col];
    if (cell == 0) {
        result.kind = SHOT_MISS;
    } else {
        Ship *ship = &board->ships[cell - 1];
        result.ship_index = (int)cell - 1;
        --ship->remaining;
        --board->segments_remaining;
        result.kind = ship->remaining == 0 ? SHOT_SUNK : SHOT_HIT;
        if (board->segments_remaining == 0) {
            defender->alive = false;
            --game->alive_count;
            result.kind = SHOT_ELIMINATED;
        }
    }

    if (game->alive_count == 1) {
        game->winner = attacker;
        result.kind = SHOT_WIN;
    } else {
        do {
            game->current = (game->current + 1) % game->player_count;
        } while (!game->players[game->current].alive);
    }
    return result;
}
