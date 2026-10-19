#include "ui.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static void print_board(const Board *board, bool own)
{
    puts("    1  2  3  4  5  6  7  8  9 10");
    for (int row = 0; row < BOARD_SIZE; ++row) {
        printf("%2d ", row + 1);
        for (int col = 0; col < BOARD_SIZE; ++col) {
            char mark = '.';
            if (board->shots[row][col])
                mark = board->cells[row][col] ? 'X' : 'o';
            else if (own && board->cells[row][col])
                mark = 'S';
            printf(" %c ", mark);
        }
        putchar('\n');
    }
}

void ui_show_turn(Game *game)
{
    Board boards[MAX_PLAYERS];
    bool alive[MAX_PLAYERS];
    int current;
    int count;
    pthread_mutex_lock(&game->mutex);
    current = game->current;
    count = game->player_count;
    for (int i = 0; i < count; ++i) {
        boards[i] = game->players[i].board;
        alive[i] = game->players[i].alive;
    }
    pthread_mutex_unlock(&game->mutex);

    printf("\n=== Tour du joueur %d ===\n", current + 1);
    puts("Votre grille : S=bateau, X=touché, o=raté, .=eau inconnue");
    print_board(&boards[current], true);
    for (int i = 0; i < count; ++i) {
        if (i == current || !alive[i])
            continue;
        printf("Tirs connus sur la grille du joueur %d :\n", i + 1);
        print_board(&boards[i], false);
    }
}

void ui_show_result(const ShotResult *result)
{
    if (result->kind == SHOT_INVALID) {
        puts("Tir invalide : cible, joueur ou coordonnées incorrects. Réessayez.");
        return;
    }
    if (result->kind == SHOT_REPEAT) {
        puts("Cette case a déjà été visée. Réessayez sans perdre le tour.");
        return;
    }
    printf("Joueur %d -> joueur %d, case (%d,%d) : ", result->attacker + 1,
           result->target + 1, result->row + 1, result->col + 1);
    switch (result->kind) {
    case SHOT_MISS: puts("raté."); break;
    case SHOT_HIT: puts("touché !"); break;
    case SHOT_SUNK: puts("touché, bateau coulé !"); break;
    case SHOT_ELIMINATED: puts("dernier bateau coulé, joueur éliminé !"); break;
    case SHOT_WIN: printf("dernier bateau coulé. Le joueur %d gagne !\n",
                          result->attacker + 1); break;
    default: break;
    }
}

int ui_read_attack(int *target, int *row, int *col)
{
    char line[128];
    fputs("Entrez cible ligne colonne (ex. 2 4 7), ou Ctrl-D pour quitter : ", stdout);
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin))
        return 0;
    bool complete = false;
    for (size_t i = 0; line[i]; ++i)
        if (line[i] == '\n')
            complete = true;
    if (!complete && !feof(stdin)) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {}
        puts("Entrée trop longue.");
        return -1;
    }

    long values[3];
    char *cursor = line;
    for (int i = 0; i < 3; ++i) {
        while (isspace((unsigned char)*cursor))
            ++cursor;
        if (!*cursor) {
            puts("Format attendu : cible ligne colonne.");
            return -1;
        }
        char *end;
        values[i] = strtol(cursor, &end, 10);
        if (cursor == end || values[i] < 1 || values[i] > 10) {
            puts("Utilisez trois nombres positifs de 1 à 10.");
            return -1;
        }
        cursor = end;
    }
    while (isspace((unsigned char)*cursor))
        ++cursor;
    if (*cursor) {
        puts("Il faut exactement trois nombres.");
        return -1;
    }
    *target = (int)values[0] - 1;
    *row = (int)values[1] - 1;
    *col = (int)values[2] - 1;
    return 1;
}
