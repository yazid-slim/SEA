#include "workers.h"

#include "engine.h"

typedef struct {
    Game *game;
    int id;
} WorkerArg;

static WorkerArg worker_args[MAX_PLAYERS];

static void publish(Game *game, ShotResult result)
{
    game->result = result;
    game->result_ready = true;
    pthread_cond_signal(&game->result_wake);
}

static void *player_main(void *argument)
{
    WorkerArg *arg = argument;
    Game *game = arg->game;
    int id = arg->id;
    Player *self = &game->players[id];

    pthread_mutex_lock(&game->mutex);
    while (!game->shutdown) {
        /* The predicate is checked after every wake, including spurious wakes. */
        while (self->mail.kind == MAIL_NONE && !game->shutdown)
            pthread_cond_wait(&self->wake, &game->mutex);
        if (game->shutdown)
            break;
        Mail mail = self->mail;
        self->mail.kind = MAIL_NONE;

        if (mail.kind == MAIL_FIRE) {
            ShotKind valid = engine_validate_attack(game, id, mail.target,
                                                    mail.row, mail.col);
            if (valid != SHOT_MISS) {
                ShotResult result = {valid, id, mail.target, mail.row, mail.col, -1};
                publish(game, result);
            } else {
                /* The defender's worker owns the actual board update. */
                Player *defender = &game->players[mail.target];
                defender->mail = (Mail){MAIL_INCOMING, id, mail.target,
                                        mail.row, mail.col};
                pthread_cond_signal(&defender->wake);
            }
        } else if (mail.kind == MAIL_INCOMING) {
            ShotResult result = engine_attack(game, mail.attacker, id,
                                              mail.row, mail.col);
            publish(game, result);
        }
    }
    pthread_mutex_unlock(&game->mutex);
    return NULL;
}

bool workers_start(Game *game)
{
    if (pthread_mutex_init(&game->mutex, NULL) != 0)
        return false;
    if (pthread_cond_init(&game->result_wake, NULL) != 0) {
        pthread_mutex_destroy(&game->mutex);
        return false;
    }
    int initialized = 0;
    for (; initialized < game->player_count; ++initialized) {
        if (pthread_cond_init(&game->players[initialized].wake, NULL) != 0)
            break;
    }
    if (initialized != game->player_count) {
        for (int i = 0; i < initialized; ++i)
            pthread_cond_destroy(&game->players[i].wake);
        pthread_cond_destroy(&game->result_wake);
        pthread_mutex_destroy(&game->mutex);
        return false;
    }

    int started = 0;
    for (; started < game->player_count; ++started) {
        worker_args[started].game = game;
        worker_args[started].id = started;
        if (pthread_create(&game->players[started].thread, NULL, player_main,
                           &worker_args[started]) != 0)
            break;
    }
    if (started != game->player_count) {
        pthread_mutex_lock(&game->mutex);
        game->shutdown = true;
        for (int i = 0; i < started; ++i)
            pthread_cond_signal(&game->players[i].wake);
        pthread_mutex_unlock(&game->mutex);
        for (int i = 0; i < started; ++i)
            pthread_join(game->players[i].thread, NULL);
        for (int i = 0; i < game->player_count; ++i)
            pthread_cond_destroy(&game->players[i].wake);
        pthread_cond_destroy(&game->result_wake);
        pthread_mutex_destroy(&game->mutex);
        return false;
    }
    return true;
}

bool workers_fire(Game *game, int attacker, int target, int row, int col,
                  ShotResult *result)
{
    pthread_mutex_lock(&game->mutex);
    if (game->shutdown || attacker < 0 || attacker >= game->player_count ||
        game->result_ready || game->players[attacker].mail.kind != MAIL_NONE) {
        pthread_mutex_unlock(&game->mutex);
        return false;
    }
    game->players[attacker].mail = (Mail){MAIL_FIRE, attacker, target, row, col};
    pthread_cond_signal(&game->players[attacker].wake);
    while (!game->result_ready && !game->shutdown)
        pthread_cond_wait(&game->result_wake, &game->mutex);
    bool ok = game->result_ready;
    if (ok) {
        *result = game->result;
        game->result_ready = false;
    }
    pthread_mutex_unlock(&game->mutex);
    return ok;
}

void workers_stop(Game *game)
{
    pthread_mutex_lock(&game->mutex);
    game->shutdown = true;
    for (int i = 0; i < game->player_count; ++i)
        pthread_cond_signal(&game->players[i].wake);
    pthread_cond_signal(&game->result_wake);
    pthread_mutex_unlock(&game->mutex);

    for (int i = 0; i < game->player_count; ++i)
        pthread_join(game->players[i].thread, NULL);
    for (int i = 0; i < game->player_count; ++i)
        pthread_cond_destroy(&game->players[i].wake);
    pthread_cond_destroy(&game->result_wake);
    pthread_mutex_destroy(&game->mutex);
}
