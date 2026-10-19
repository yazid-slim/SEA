#include "engine.h"
#include "ui.h"
#include "workers.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static bool parse_uint(const char *text, unsigned long max, unsigned long *value)
{
    if (!text[0] || text[0] == '-')
        return false;
    errno = 0;
    char *end;
    unsigned long parsed = strtoul(text, &end, 10);
    if (errno || *end || parsed > max)
        return false;
    *value = parsed;
    return true;
}

/* Demo uses the hidden boards only to produce a quick complete game. */
static bool demo_attack(Game *game, int *attacker, int *target, int *row, int *col)
{
    pthread_mutex_lock(&game->mutex);
    *attacker = game->current;
    *target = (*attacker + 1) % game->player_count;
    while (!game->players[*target].alive)
        *target = (*target + 1) % game->player_count;
    Board *board = &game->players[*target].board;
    bool found = false;
    for (int r = 0; r < BOARD_SIZE && !found; ++r)
        for (int c = 0; c < BOARD_SIZE && !found; ++c)
            if (board->cells[r][c] && !board->shots[r][c]) {
                *row = r;
                *col = c;
                found = true;
            }
    pthread_mutex_unlock(&game->mutex);
    return found;
}

int main(int argc, char **argv)
{
    bool demo = argc > 1 && strcmp(argv[1], "--demo") == 0;
    int first = demo ? 2 : 1;
    if (argc > first + 2) {
        fprintf(stderr, "Usage: %s [--demo] [joueurs:2-4] [graine]\n", argv[0]);
        return EXIT_FAILURE;
    }
    unsigned long count = 2;
    unsigned long seed = (unsigned long)time(NULL);
    if ((argc > first && (!parse_uint(argv[first], 4, &count) || count < 2)) ||
        (argc > first + 1 && !parse_uint(argv[first + 1], UINT_MAX, &seed))) {
        fprintf(stderr, "Usage: %s [--demo] [joueurs:2-4] [graine]\n", argv[0]);
        return EXIT_FAILURE;
    }

    Game game;
    if (!engine_init(&game, (int)count, (uint32_t)seed)) {
        fputs("Impossible de créer les flottes.\n", stderr);
        return EXIT_FAILURE;
    }
    if (!workers_start(&game)) {
        fputs("Impossible de créer les threads ou les synchronisations.\n", stderr);
        return EXIT_FAILURE;
    }
    printf("Bataille navale : %lu joueurs, graine %lu%s\n", count, seed,
           demo ? " (démonstration automatique)" : "");

    bool ok = true;
    for (;;) {
        pthread_mutex_lock(&game.mutex);
        int winner = game.winner;
        int attacker = game.current;
        pthread_mutex_unlock(&game.mutex);
        if (winner >= 0)
            break;

        int target, row, col;
        if (demo) {
            if (!demo_attack(&game, &attacker, &target, &row, &col)) {
                fputs("La démonstration ne trouve plus de cible.\n", stderr);
                ok = false;
                break;
            }
        } else {
            ui_show_turn(&game);
            int read_status = ui_read_attack(&target, &row, &col);
            if (read_status == 0) {
                puts("Fin de l'entrée : partie arrêtée proprement.");
                break;
            }
            if (read_status < 0)
                continue;
        }
        ShotResult result;
        if (!workers_fire(&game, attacker, target, row, col, &result)) {
            fputs("Erreur de coordination des joueurs.\n", stderr);
            ok = false;
            break;
        }
        ui_show_result(&result);
    }
    workers_stop(&game);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
