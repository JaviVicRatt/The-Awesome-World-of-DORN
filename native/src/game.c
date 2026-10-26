#include "game.h"

#include <stdio.h>
#include <string.h>

const DornCharacter dorn_heroes[DORN_ROSTER_SIZE] = {
    {"Almanor", "heroe/face/almanor.png", 6, 3, 1, 3, 1, 0},
    {"Choros", "heroe/face/choros.png", 5, 6, 1, 1, 1, 1},
    {"Eleanor", "heroe/face/eleanor.png", 7, 5, 1, 2, 1, 2},
    {"Gor", "heroe/face/gor.png", 6, 5, 1, 1, 2, 0},
    {"Guelin", "heroe/face/guelin.png", 6, 5, 1, 3, 1, 3},
    {"Kaerdrak", "heroe/face/kaerdrak.png", 6, 5, 1, 1, 1, 1},
    {"Ragnar", "heroe/face/ragnar.png", 6, 5, 1, 1, 1, 1},
    {"Riel", "heroe/face/riel.png", 6, 4, 1, 4, 1, 3},
    {"Vargen", "heroe/face/vargen.png", 7, 5, 1, 1, 1, 1}
};

static const DornCharacter minor_monsters[] = {
    {"Rata", "monster/sprite/rata.png", 7, 1, 1, 1, 1, 1},
    {"Zombi", "monster/sprite/zombi.png", 6, 1, 1, 1, 1, 1},
    {"Esqueleto", "monster/sprite/esqueleto.png", 6, 1, 1, 2, 1, 2},
    {"Engendro", "monster/sprite/engendro.png", 5, 2, 1, 1, 1, 1}
};

static const DornCharacter major_monsters[] = {
    {"Fantasma", "monster/sprite/fantasma.png", 12, 2, 1, 1, 1, 1},
    {"Momia", "monster/sprite/momia.png", 6, 2, 2, 1, 1, 1},
    {"Vampiro", "monster/sprite/vampiro.png", 6, 2, 2, 1, 1, 1},
    {"Gorgona", "monster/sprite/gorgona.png", 6, 2, 1, 3, 1, 3}
};

static const char *ritual_names[] = {
    "Cieno de la Desesperanza", "Fuente Negra", "Prision Gelida",
    "Ansia de Sangre", "Ratas Hambrientas", "Refuerzos",
    "Transformacion Fantasmal", "Maldicion de los Discipulos",
    "Fortaleza Oscura", "Portales Espinosos", "La Tierra Despierta",
    "Frenesi Zombi"
};

static const char *blessing_names[] = {
    "El Viajero Astral", "Las dos caras del heroe",
    "La sangre vuelve a fluir", "El canto de Orfeo",
    "Cancion de la discordia", "Quiero que mueras",
    "La fuerza de la ira", "Las raices firmes", "Lazos curativos",
    "El viento en el pelo", "Escucha la llamada", "Las piernas ardientes"
};

static uint32_t next_random(DornGame *game) {
    uint32_t x = game->random_state;
    if (x == 0) {
        x = 0x9e3779b9u;
    }
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    game->random_state = x;
    return x;
}

static void deal_unique_cards(DornGame *game) {
    int ritual_order[12];
    int blessing_order[12];
    for (int i = 0; i < 12; ++i) {
        ritual_order[i] = i;
        blessing_order[i] = i;
    }
    /* EN: Partial Fisher-Yates draws without replacement, so dealt cards are unique.
     * ES: Fisher-Yates parcial roba sin reemplazo y garantiza cartas unicas.
     */
    for (int i = 0; i < DORN_INITIAL_RITUALS; ++i) {
        int selected = i + (int)(next_random(game) % (uint32_t)(12 - i));
        int temporary = ritual_order[i];
        ritual_order[i] = ritual_order[selected];
        ritual_order[selected] = temporary;
        snprintf(game->ritual_cards[i], sizeof(game->ritual_cards[i]), "%s",
                 ritual_names[ritual_order[i]]);
    }
    for (int i = 0; i < game->hero_count; ++i) {
        int selected = i + (int)(next_random(game) % (uint32_t)(12 - i));
        int temporary = blessing_order[i];
        blessing_order[i] = blessing_order[selected];
        blessing_order[selected] = temporary;
        snprintf(game->hero_blessings[i], sizeof(game->hero_blessings[i]), "%s",
                 blessing_names[blessing_order[i]]);
    }
}

int dorn_game_distance(int start_cell, int target_cell) {
    return dorn_board_distance(start_cell, target_cell);
}

static const DornActor *actor_at_cell_const(const DornGame *game, int cell_id) {
    if (game->guardian.alive && game->guardian.cell_id == cell_id) {
        return &game->guardian;
    }
    for (int i = 0; i < game->hero_count; ++i) {
        if (game->heroes[i].alive && game->heroes[i].cell_id == cell_id) {
            return &game->heroes[i];
        }
    }
    for (int i = 0; i < game->monster_count; ++i) {
        if (game->monsters[i].alive && game->monsters[i].cell_id == cell_id) {
            return &game->monsters[i];
        }
    }
    return NULL;
}

static bool position_open(const DornGame *game, int cell_id) {
    if (dorn_board_cell(cell_id) == NULL) {
        return false;
    }
    return actor_at_cell_const(game, cell_id) == NULL;
}

static void set_actor(DornActor *actor, const DornCharacter *character,
                      DornActorKind kind, int roster_index, int cell_id) {
    memset(actor, 0, sizeof(*actor));
    snprintf(actor->name, sizeof(actor->name), "%s", character->name);
    actor->portrait = character->portrait;
    actor->kind = kind;
    actor->roster_index = roster_index;
    actor->cell_id = cell_id;
    actor->movement = character->movement;
    actor->max_movement = character->movement;
    actor->health = character->health;
    actor->max_health = character->health;
    actor->attacks = character->attacks;
    actor->max_attacks = character->attacks;
    actor->attack_range = character->attack_range;
    actor->damage = character->damage;
    actor->attack_type = character->attack_type;
    actor->alive = true;
}

void dorn_game_init(DornGame *game, const int *hero_roster, int hero_count,
                    uint32_t seed) {
    if (game == NULL) {
        return;
    }
    dorn_board_init();
    memset(game, 0, sizeof(*game));
    if (hero_count < 1) {
        hero_count = 1;
    } else if (hero_count > DORN_MAX_HEROES) {
        hero_count = DORN_MAX_HEROES;
    }
    game->hero_count = hero_count;
    game->round = 1;
    game->random_state = seed;
    game->phase = DORN_PHASE_RITUAL;
    deal_unique_cards(game);

    const DornCharacter zorkal = {
        "Zorkal", "heroe/face/zorkal.png", 6, 4, 2, 1, 1, 1
    };
    set_actor(&game->guardian, &zorkal, DORN_ACTOR_GUARDIAN, 0,
              DORN_THRONE_FIRST + 6);

    int roster_order[DORN_MAX_HEROES];
    for (int i = 0; i < hero_count; ++i) {
        roster_order[i] = hero_roster != NULL ? hero_roster[i] : i;
        if (roster_order[i] < 0 || roster_order[i] >= DORN_ROSTER_SIZE) {
            roster_order[i] = 0;
        }
    }
    for (int i = hero_count - 1; i > 0; --i) {
        int selected = (int)(next_random(game) % (uint32_t)(i + 1));
        int temporary = roster_order[i];
        roster_order[i] = roster_order[selected];
        roster_order[selected] = temporary;
    }

    for (int i = 0; i < hero_count; ++i) {
        int roster = roster_order[i];
        set_actor(&game->heroes[i], &dorn_heroes[roster], DORN_ACTOR_HERO,
                  roster, DORN_ARGOS_FIRST + i);
    }

    static const int spawn_positions[6] = {
        DORN_FOREST_FIRST + 109,
        DORN_MINE_FIRST + 75,
        DORN_INITIAL_SPAWN_FIRST,
        DORN_INITIAL_SPAWN_FIRST + 1,
        DORN_INITIAL_SPAWN_FIRST + 2,
        DORN_INITIAL_SPAWN_FIRST + 3
    };
    for (int i = 0; i < 5; ++i) {
        const DornCharacter *monster =
            &minor_monsters[next_random(game) % 4u];
        set_actor(&game->monsters[game->monster_count], monster,
                  DORN_ACTOR_MONSTER, -1, spawn_positions[i]);
        game->monster_count++;
    }
    const DornCharacter *major =
        &major_monsters[next_random(game) % 4u];
    set_actor(&game->monsters[game->monster_count], major,
              DORN_ACTOR_MONSTER, -1, spawn_positions[5]);
    game->monster_count++;
    snprintf(game->status, sizeof(game->status),
             "Ronda 1: turno del Guardian. Enter para continuar.");
}

DornActor *dorn_game_active_actor(DornGame *game) {
    if (game == NULL) {
        return NULL;
    }
    switch (game->phase) {
        case DORN_PHASE_GUARDIAN_MOVE:
        case DORN_PHASE_GUARDIAN_ATTACK:
            if (game->active_actor > 0 &&
                game->active_actor - 1 < game->monster_count) {
                return &game->monsters[game->active_actor - 1];
            }
            return NULL;
        case DORN_PHASE_HERO_MOVE:
        case DORN_PHASE_HERO_ATTACK:
            if (game->active_actor >= 0 && game->active_actor < game->hero_count) {
                return &game->heroes[game->active_actor];
            }
            return NULL;
        default:
            return NULL;
    }
}

static const DornActor *active_actor_const(const DornGame *game) {
    if (game == NULL) {
        return NULL;
    }
    switch (game->phase) {
        case DORN_PHASE_GUARDIAN_MOVE:
        case DORN_PHASE_GUARDIAN_ATTACK:
            if (game->active_actor > 0 &&
                game->active_actor - 1 < game->monster_count) {
                return &game->monsters[game->active_actor - 1];
            }
            return NULL;
        case DORN_PHASE_HERO_MOVE:
        case DORN_PHASE_HERO_ATTACK:
            if (game->active_actor >= 0 && game->active_actor < game->hero_count) {
                return &game->heroes[game->active_actor];
            }
            return NULL;
        default:
            return NULL;
    }
}

DornActor *dorn_game_actor_at(DornGame *game, int cell_id) {
    if (game == NULL) {
        return NULL;
    }
    if (game->guardian.alive && game->guardian.cell_id == cell_id) {
        return &game->guardian;
    }
    for (int i = 0; i < game->hero_count; ++i) {
        if (game->heroes[i].alive && game->heroes[i].cell_id == cell_id) {
            return &game->heroes[i];
        }
    }
    for (int i = 0; i < game->monster_count; ++i) {
        if (game->monsters[i].alive && game->monsters[i].cell_id == cell_id) {
            return &game->monsters[i];
        }
    }
    return NULL;
}

bool dorn_game_summon(DornGame *game) {
    if (game == NULL || game->phase != DORN_PHASE_SUMMON ||
        game->monster_count >= DORN_MAX_MONSTERS ||
        (game->minor_summons_remaining == 0 &&
         game->major_summons_remaining == 0)) {
        return false;
    }
    snprintf(game->status, sizeof(game->status),
             "Invocacion desactivada: faltan las zonas de aparicion del mapa.");
    return false;
}

void dorn_game_reachable(const DornGame *game,
                         int distances[DORN_BOARD_CELL_COUNT]) {
    if (distances == NULL) {
        return;
    }
    for (int i = 0; i < DORN_BOARD_CELL_COUNT; ++i) {
        distances[i] = -1;
    }
    const DornActor *actor = active_actor_const(game);
    if (game == NULL || actor == NULL || !actor->alive ||
        actor->movement <= 0 || actor->cell_id < 0 ||
        actor->cell_id >= DORN_BOARD_CELL_COUNT) {
        return;
    }
    int queue[DORN_BOARD_CELL_COUNT];
    int head = 0;
    int tail = 0;
    distances[actor->cell_id] = 0;
    queue[tail++] = actor->cell_id;
    /* EN: Occupancy and faction rules are applied while expanding each graph step.
     * ES: La ocupacion y las reglas de faccion se aplican al recorrer cada enlace.
     */
    while (head < tail) {
        int current = queue[head++];
        const DornBoardCell *cell = dorn_board_cell(current);
        for (int i = 0; i < cell->neighbor_count; ++i) {
            int next = cell->neighbors[i];
            const DornBoardCell *next_cell = dorn_board_cell(next);
            int next_distance = distances[current] + 1;
            if (next_distance > actor->movement || distances[next] >= 0 ||
                (actor->kind == DORN_ACTOR_MONSTER && next_cell->hero_only)) {
                continue;
            }
            const DornActor *occupant = actor_at_cell_const(game, next);
            /* EN: Heroes may pass one another only when both spaces are on their path.
             * ES: Los heroes solo pueden atravesarse si ambas casillas son del camino.
             */
            bool can_pass_hero = actor->kind == DORN_ACTOR_HERO &&
                                 occupant != NULL &&
                                 occupant->kind == DORN_ACTOR_HERO &&
                                 cell->hero_only && next_cell->hero_only;
            if (occupant != NULL && !can_pass_hero) {
                continue;
            }
            distances[next] = next_distance;
            queue[tail++] = next;
        }
    }
}

bool dorn_game_move(DornGame *game, int destination_cell) {
    DornActor *actor = dorn_game_active_actor(game);
    if (actor == NULL || !actor->alive ||
        (game->phase != DORN_PHASE_GUARDIAN_MOVE &&
         game->phase != DORN_PHASE_HERO_MOVE)) {
        return false;
    }
    if (destination_cell == actor->cell_id || !position_open(game, destination_cell)) {
        return false;
    }
    int distances[DORN_BOARD_CELL_COUNT];
    dorn_game_reachable(game, distances);
    int distance = distances[destination_cell];
    if (distance <= 0) {
        return false;
    }
    actor->cell_id = destination_cell;
    actor->movement -= distance;
    snprintf(game->status, sizeof(game->status),
             "%s se mueve %d casilla(s). Movimiento restante: %d.",
             actor->name, distance, actor->movement);
    return true;
}

static int attack_distance(const DornActor *attacker, int target_cell) {
    if (attacker == NULL || dorn_board_cell(attacker->cell_id) == NULL ||
        dorn_board_cell(target_cell) == NULL) {
        return -1;
    }
    int distances[DORN_BOARD_CELL_COUNT];
    int queue[DORN_BOARD_CELL_COUNT];
    for (int i = 0; i < DORN_BOARD_CELL_COUNT; ++i) {
        distances[i] = -1;
    }
    int head = 0;
    int tail = 0;
    distances[attacker->cell_id] = 0;
    queue[tail++] = attacker->cell_id;
    /* EN: Combat distance uses legal links but ignores occupied intermediate cells.
     * ES: La distancia de combate usa enlaces legales, sin bloquearse por ocupantes.
     */
    while (head < tail) {
        int current = queue[head++];
        if (current == target_cell) {
            return distances[current];
        }
        const DornBoardCell *cell = dorn_board_cell(current);
        for (int i = 0; i < cell->neighbor_count; ++i) {
            int next = cell->neighbors[i];
            const DornBoardCell *next_cell = dorn_board_cell(next);
            if (distances[next] >= 0 ||
                (attacker->kind == DORN_ACTOR_MONSTER && next_cell->hero_only)) {
                continue;
            }
            distances[next] = distances[current] + 1;
            queue[tail++] = next;
        }
    }
    return -1;
}

static bool is_valid_target(const DornActor *attacker, const DornActor *target) {
    int distance = attacker != NULL && target != NULL
                       ? attack_distance(attacker, target->cell_id) : -1;
    if (attacker == NULL || target == NULL || !attacker->alive || !target->alive ||
        attacker == target || attacker->attacks <= 0 || distance < 1 ||
        distance > attacker->attack_range ||
        (attacker->attack_type == 2 && distance != 2)) {
        return false;
    }
    const DornBoardCell *target_cell = dorn_board_cell(target->cell_id);
    if (target_cell == NULL) {
        return false;
    }
    if (attacker->kind == DORN_ACTOR_MONSTER && target_cell->hero_only) {
        return false;
    }
    if (attacker->kind == DORN_ACTOR_HERO) {
        return target->kind == DORN_ACTOR_MONSTER ||
               target->kind == DORN_ACTOR_GUARDIAN;
    }
    if (attacker->kind == DORN_ACTOR_MONSTER) {
        return target->kind == DORN_ACTOR_HERO;
    }
    return target->kind != DORN_ACTOR_GUARDIAN;
}

bool dorn_game_attack(DornGame *game, int target_cell) {
    if (game == NULL ||
        (game->phase != DORN_PHASE_GUARDIAN_ATTACK &&
         game->phase != DORN_PHASE_HERO_ATTACK)) {
        return false;
    }
    DornActor *attacker = dorn_game_active_actor(game);
    DornActor *target = dorn_game_actor_at(game, target_cell);
    if (!is_valid_target(attacker, target)) {
        return false;
    }
    attacker->attacks--;
    target->health -= attacker->damage;
    if (target->health <= 0) {
        target->health = 0;
        target->alive = false;
        snprintf(game->status, sizeof(game->status), "%s derrota a %s.",
                 attacker->name, target->name);
    } else {
        snprintf(game->status, sizeof(game->status), "%s ataca a %s (%d/%d vida).",
                 attacker->name, target->name, target->health, target->max_health);
    }
    return true;
}

static void reset_turn_actor(DornActor *actor) {
    if (actor != NULL && actor->alive) {
        actor->movement = actor->max_movement;
        actor->attacks = actor->max_attacks;
    }
}

static void begin_guardian_move(DornGame *game) {
    game->phase = DORN_PHASE_GUARDIAN_MOVE;
    game->active_actor = 1;
    if (game->monster_count > 0) {
        reset_turn_actor(&game->monsters[0]);
    }
    snprintf(game->status, sizeof(game->status),
             "El Guardian mueve sus criaturas. Enter pasa al siguiente actor.");
}

static void begin_guardian_attack(DornGame *game) {
    game->phase = DORN_PHASE_GUARDIAN_ATTACK;
    game->active_actor = 1;
    if (game->monster_count > 0) {
        reset_turn_actor(&game->monsters[0]);
    }
    snprintf(game->status, sizeof(game->status), "Ataque de las criaturas del Guardian.");
}

static void begin_hero_move(DornGame *game) {
    game->phase = DORN_PHASE_HERO_MOVE;
    game->active_actor = 0;
    while (game->active_actor < game->hero_count &&
           !game->heroes[game->active_actor].alive) {
        game->active_actor++;
    }
    for (int i = 0; i < game->hero_count; ++i) {
        game->heroes[i].movement = dorn_heroes[game->heroes[i].roster_index].movement;
        game->heroes[i].attacks = game->heroes[i].max_attacks;
    }
    snprintf(game->status, sizeof(game->status), "Fase de movimiento de los heroes.");
}

static void begin_hero_attack(DornGame *game) {
    game->phase = DORN_PHASE_HERO_ATTACK;
    game->active_actor = 0;
    while (game->active_actor < game->hero_count &&
           !game->heroes[game->active_actor].alive) {
        game->active_actor++;
    }
    snprintf(game->status, sizeof(game->status), "Fase de ataque de los heroes.");
}

static bool any_heroes_alive(const DornGame *game) {
    for (int i = 0; i < game->hero_count; ++i) {
        if (game->heroes[i].alive) {
            return true;
        }
    }
    return false;
}

static bool guardian_alive(const DornGame *game) {
    return game->guardian.alive;
}

void dorn_game_end_turn(DornGame *game) {
    if (game == NULL) {
        return;
    }
    game->night = !game->night;
    game->round++;
    game->phase = DORN_PHASE_RITUAL;
    game->active_actor = 0;
    snprintf(game->status, sizeof(game->status),
             "Ronda %d: turno del Guardian. Enter para continuar.", game->round);
    if (!any_heroes_alive(game) || !guardian_alive(game)) {
        game->phase = DORN_PHASE_END;
        snprintf(game->status, sizeof(game->status), "%s gana la partida.",
                 guardian_alive(game) ? "El Guardian" : "Los heroes");
    }
}

void dorn_game_advance(DornGame *game) {
    if (game == NULL) {
        return;
    }
    /* EN: Advancing is the single transition point for the ordered turn phases.
     * ES: Este es el punto unico que avanza entre las fases ordenadas del turno.
     */
    switch (game->phase) {
        case DORN_PHASE_RITUAL:
            game->phase = DORN_PHASE_SUMMON;
            game->minor_summons_remaining =
                game->round == 1 ? 0 : (game->night ? 2 : 1);
            game->major_summons_remaining =
                game->round == 1 ? 0 : (game->night ? 1 : 0);
            if (game->minor_summons_remaining == 0 &&
                game->major_summons_remaining == 0) {
                snprintf(game->status, sizeof(game->status),
                         "Invocacion inicial ya preparada. Enter sigue.");
            } else {
                snprintf(game->status, sizeof(game->status),
                         "Q invoca %d criatura(s) menor(es) y %d mayor(es).",
                         game->minor_summons_remaining,
                         game->major_summons_remaining);
            }
            break;
        case DORN_PHASE_SUMMON:
            begin_guardian_move(game);
            break;
        case DORN_PHASE_GUARDIAN_MOVE:
            game->active_actor++;
            while (game->active_actor <= game->monster_count &&
                   !game->monsters[game->active_actor - 1].alive) {
                game->active_actor++;
            }
            if (game->active_actor > game->monster_count) {
                begin_guardian_attack(game);
            } else {
                reset_turn_actor(&game->monsters[game->active_actor - 1]);
                snprintf(game->status, sizeof(game->status),
                         "Movimiento de criatura: %s.", game->monsters[game->active_actor - 1].name);
            }
            break;
        case DORN_PHASE_GUARDIAN_ATTACK:
            game->active_actor++;
            while (game->active_actor <= game->monster_count &&
                   !game->monsters[game->active_actor - 1].alive) {
                game->active_actor++;
            }
            if (game->active_actor > game->monster_count) {
                game->phase = DORN_PHASE_HERO_BLESSING;
                snprintf(game->status, sizeof(game->status),
                         "Turno de bendiciones. Enter para continuar.");
            } else {
                reset_turn_actor(&game->monsters[game->active_actor - 1]);
                snprintf(game->status, sizeof(game->status),
                         "Ataque de criatura: %s.", game->monsters[game->active_actor - 1].name);
            }
            break;
        case DORN_PHASE_HERO_BLESSING:
            begin_hero_move(game);
            break;
        case DORN_PHASE_HERO_MOVE:
            do {
                game->active_actor++;
            } while (game->active_actor < game->hero_count &&
                     !game->heroes[game->active_actor].alive);
            if (game->active_actor >= game->hero_count) {
                begin_hero_attack(game);
            } else {
                snprintf(game->status, sizeof(game->status),
                         "Movimiento de %s.", game->heroes[game->active_actor].name);
            }
            break;
        case DORN_PHASE_HERO_ATTACK:
            do {
                game->active_actor++;
            } while (game->active_actor < game->hero_count &&
                     !game->heroes[game->active_actor].alive);
            if (game->active_actor >= game->hero_count) {
                game->phase = DORN_PHASE_END;
                snprintf(game->status, sizeof(game->status),
                         "Turno completado. Enter alterna dia/noche.");
            } else {
                snprintf(game->status, sizeof(game->status),
                         "Ataque de %s.", game->heroes[game->active_actor].name);
            }
            break;
        case DORN_PHASE_END:
            dorn_game_end_turn(game);
            break;
    }
}

const char *dorn_game_phase_name(DornPhase phase) {
    static const char *names[] = {
        "RITUAL", "INVOCAR", "MOVER GUARDIAN", "ATACAR GUARDIAN",
        "BENDICIONES", "MOVER HEROES", "ATACAR HEROES", "FIN DE TURNO"
    };
    if (phase < DORN_PHASE_RITUAL || phase > DORN_PHASE_END) {
        return "DESCONOCIDA";
    }
    return names[phase];
}
