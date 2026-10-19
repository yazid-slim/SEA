#include "engine.h"

#include <assert.h>
#include <stdio.h>

static void test_placement(void)
{
    for (uint32_t seed = 1; seed <= 200; ++seed) {
        Game game;
        assert(engine_init(&game, 4, seed));
        for (int p = 0; p < 4; ++p) {
            Board *board = &game.players[p].board;
            int counts[FLEET_SIZE] = {0};
            for (int r = 0; r < BOARD_SIZE; ++r)
                for (int c = 0; c < BOARD_SIZE; ++c) {
                    assert(board->cells[r][c] <= FLEET_SIZE);
                    if (board->cells[r][c])
                        ++counts[board->cells[r][c] - 1];
                    assert(!board->shots[r][c]);
                }
            for (int ship = 0; ship < FLEET_SIZE; ++ship) {
                assert(counts[ship] == board->ships[ship].length);
                assert(board->ships[ship].remaining == counts[ship]);
            }
            assert(board->segments_remaining == 17);
        }
    }
}

static void test_shots(void)
{
    Game game;
    assert(engine_init(&game, 2, 123));
    assert(engine_attack(&game, 0, 0, 0, 0).kind == SHOT_INVALID);
    assert(engine_attack(&game, 0, 1, -1, 0).kind == SHOT_INVALID);
    assert(engine_attack(&game, 0, 1, 10, 0).kind == SHOT_INVALID);
    assert(engine_attack(&game, 0, 2, 0, 0).kind == SHOT_INVALID);
    assert(game.current == 0);

    Board *target = &game.players[1].board;
    int miss_row = -1, miss_col = -1;
    for (int r = 0; r < BOARD_SIZE && miss_row < 0; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c)
            if (!target->cells[r][c]) {
                miss_row = r;
                miss_col = c;
                break;
            }
    assert(engine_attack(&game, 0, 1, miss_row, miss_col).kind == SHOT_MISS);
    assert(game.current == 1);
    game.current = 0; /* Isolate shot rules from turn scheduling. */
    assert(engine_attack(&game, 0, 1, miss_row, miss_col).kind == SHOT_REPEAT);
    assert(game.current == 0);

    int sunk = 0;
    for (int r = 0; r < BOARD_SIZE; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if (!target->cells[r][c])
                continue;
            game.current = 0;
            ShotResult result = engine_attack(&game, 0, 1, r, c);
            assert(result.kind == SHOT_HIT || result.kind == SHOT_SUNK ||
                   result.kind == SHOT_WIN);
            if (result.kind == SHOT_SUNK)
                ++sunk;
        }
    assert(sunk == 4); /* Final sunk ship is reported as the winning shot. */
    assert(target->segments_remaining == 0);
    assert(!game.players[1].alive);
    assert(game.alive_count == 1);
    assert(game.winner == 0);
}

static void test_multiplayer_elimination(void)
{
    Game game;
    assert(engine_init(&game, 3, 42));
    Board *target = &game.players[1].board;
    ShotKind last = SHOT_INVALID;
    for (int r = 0; r < BOARD_SIZE; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c)
            if (target->cells[r][c]) {
                game.current = 0;
                last = engine_attack(&game, 0, 1, r, c).kind;
            }
    assert(last == SHOT_ELIMINATED);
    assert(game.alive_count == 2);
    assert(game.winner == -1);
    assert(game.current == 2);
    assert(engine_attack(&game, 0, 1, 0, 0).kind == SHOT_INVALID);
}

int main(void)
{
    assert(!engine_init(NULL, 2, 1));
    Game game;
    assert(!engine_init(&game, 1, 1));
    assert(!engine_init(&game, 5, 1));
    test_placement();
    test_shots();
    test_multiplayer_elimination();
    puts("Engine tests passed.");
    return 0;
}
