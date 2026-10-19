#ifndef GAME_H
#define GAME_H

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#define BOARD_SIZE 10
#define FLEET_SIZE 5
#define MAX_PLAYERS 4

typedef struct {
    const char *name;
    int length;
    int remaining;
} Ship;

typedef struct {
    /* 0 means water; 1..FLEET_SIZE identify a ship. */
    uint8_t cells[BOARD_SIZE][BOARD_SIZE];
    bool shots[BOARD_SIZE][BOARD_SIZE];
    Ship ships[FLEET_SIZE];
    int segments_remaining;
} Board;

typedef enum { MAIL_NONE, MAIL_FIRE, MAIL_INCOMING } MailKind;

typedef struct {
    MailKind kind;
    int attacker;
    int target;
    int row;
    int col;
} Mail;

typedef struct {
    int id;
    Board board;
    pthread_t thread;
    pthread_cond_t wake;
    Mail mail;
    bool alive;
} Player;

typedef enum {
    SHOT_INVALID, SHOT_REPEAT, SHOT_MISS, SHOT_HIT,
    SHOT_SUNK, SHOT_ELIMINATED, SHOT_WIN
} ShotKind;

typedef struct {
    ShotKind kind;
    int attacker;
    int target;
    int row;
    int col;
    int ship_index; /* -1 for water or an invalid request */
} ShotResult;

typedef struct Game {
    Player players[MAX_PLAYERS];
    int player_count;
    int alive_count;
    int current;
    int winner; /* -1 until the game ends */
    bool shutdown;
    bool result_ready;
    ShotResult result;
    /* Protects every board, turn, mailbox, result, and shutdown flag. */
    pthread_mutex_t mutex;
    pthread_cond_t result_wake;
} Game;

#endif
