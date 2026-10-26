#ifndef DORN_GAME_H
#define DORN_GAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "board.h"

#define DORN_MAX_HEROES 5
#define DORN_MAX_MONSTERS 24
#define DORN_ROSTER_SIZE 9
#define DORN_INITIAL_RITUALS 6

typedef enum {
    DORN_PHASE_RITUAL,
    DORN_PHASE_SUMMON,
    DORN_PHASE_GUARDIAN_MOVE,
    DORN_PHASE_GUARDIAN_ATTACK,
    DORN_PHASE_HERO_BLESSING,
    DORN_PHASE_HERO_MOVE,
    DORN_PHASE_HERO_ATTACK,
    DORN_PHASE_END
} DornPhase;

/* EN: Actor selection is derived from the current phase and active_actor index.
 * ES: El actor activo se determina por la fase actual y el indice active_actor.
 */
typedef enum {
    DORN_ACTOR_GUARDIAN,
    DORN_ACTOR_HERO,
    DORN_ACTOR_MONSTER
} DornActorKind;

typedef struct {
    const char *name;
    const char *portrait;
    int movement;
    int health;
    int attacks;
    int attack_range;
    int damage;
    int attack_type;
} DornCharacter;

typedef struct {
    char name[32];
    const char *portrait;
    DornActorKind kind;
    int roster_index;
    int cell_id;
    int movement;
    int max_movement;
    int health;
    int max_health;
    int attacks;
    int max_attacks;
    int attack_range;
    int damage;
    int attack_type;
    bool alive;
} DornActor;

typedef struct {
    DornActor guardian;
    DornActor heroes[DORN_MAX_HEROES];
    char ritual_cards[DORN_INITIAL_RITUALS][48];
    char hero_blessings[DORN_MAX_HEROES][48];
    int hero_count;
    DornActor monsters[DORN_MAX_MONSTERS];
    int monster_count;
    DornPhase phase;
    int active_actor;
    int round;
    int minor_summons_remaining;
    int major_summons_remaining;
    bool night;
    uint32_t random_state;
    char status[160];
} DornGame;

extern const DornCharacter dorn_heroes[DORN_ROSTER_SIZE];

void dorn_game_init(DornGame *game, const int *hero_roster, int hero_count,
                    uint32_t seed);
bool dorn_game_summon(DornGame *game);
bool dorn_game_move(DornGame *game, int destination_cell);
void dorn_game_reachable(const DornGame *game,
                         int distances[DORN_BOARD_CELL_COUNT]);
bool dorn_game_attack(DornGame *game, int target_cell);
void dorn_game_advance(DornGame *game);
void dorn_game_end_turn(DornGame *game);
DornActor *dorn_game_active_actor(DornGame *game);
DornActor *dorn_game_actor_at(DornGame *game, int cell_id);
const char *dorn_game_phase_name(DornPhase phase);
int dorn_game_distance(int start_cell, int target_cell);

#endif
