#include "game.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_initial_roster(void) {
    DornGame game;
    int roster[] = {0, 8, 4};
    dorn_game_init(&game, roster, 3, 1234);
    assert(game.hero_count == 3);
    assert(game.phase == DORN_PHASE_RITUAL);
    assert(game.guardian.health == 4);
    assert(game.guardian.cell_id == DORN_THRONE_FIRST + 6);
    bool selected[DORN_ROSTER_SIZE] = {false};
    for (int i = 0; i < game.hero_count; ++i) {
        selected[game.heroes[i].roster_index] = true;
    }
    assert(selected[0] && selected[8] && selected[4]);
    assert(dorn_game_distance(DORN_ARGOS_FIRST, DORN_ARGOS_FIRST + 1) == 2);
    assert(dorn_game_distance(DORN_MINE_FIRST + 24,
                              DORN_MINE_FIRST + 39) == 2);
    assert(dorn_game_distance(DORN_FOREST_FIRST,
                              DORN_FOREST_FIRST + 7) == 1);
    assert(dorn_game_distance(DORN_FOREST_FIRST + 109,
                              DORN_FOREST_FIRST + 83) == 1);
    assert(dorn_game_distance(DORN_THRONE_FIRST,
                              DORN_ARGOS_FIRST) == -1);
    for (int i = 0; i < DORN_INITIAL_RITUALS; ++i) {
        assert(game.ritual_cards[i][0] != '\0');
        for (int j = i + 1; j < DORN_INITIAL_RITUALS; ++j) {
            assert(strcmp(game.ritual_cards[i], game.ritual_cards[j]) != 0);
        }
    }
    for (int i = 0; i < game.hero_count; ++i) {
        assert(game.hero_blessings[i][0] != '\0');
        for (int j = i + 1; j < game.hero_count; ++j) {
            assert(strcmp(game.hero_blessings[i], game.hero_blessings[j]) != 0);
        }
    }
}

static void test_turn_cycle_and_summoning(void) {
    DornGame game;
    int roster[] = {1};
    dorn_game_init(&game, roster, 1, 33);
    dorn_game_advance(&game);
    assert(game.phase == DORN_PHASE_SUMMON);
    assert(!dorn_game_summon(&game));
    dorn_game_advance(&game);
    assert(game.phase == DORN_PHASE_GUARDIAN_MOVE);
    assert(dorn_game_active_actor(&game) == &game.monsters[0]);

    dorn_game_end_turn(&game);
    assert(game.round == 2);
    assert(game.night);
    dorn_game_advance(&game);
    assert(game.phase == DORN_PHASE_SUMMON);
    assert(!dorn_game_summon(&game));
    assert(game.monster_count == 6);
    assert(game.minor_summons_remaining == 2);
    assert(strstr(game.status, "zonas de aparicion") != NULL);
    dorn_game_advance(&game);
    assert(game.phase == DORN_PHASE_GUARDIAN_MOVE);
    assert(dorn_game_active_actor(&game) == &game.monsters[0]);
}

static void test_movement_and_attack_validation(void) {
    DornGame game;
    int roster[] = {0};
    dorn_game_init(&game, roster, 1, 1);
    dorn_game_advance(&game);
    dorn_game_advance(&game);
    assert(game.phase == DORN_PHASE_GUARDIAN_MOVE);
    DornActor *active = dorn_game_active_actor(&game);
    assert(active == &game.monsters[0]);
    const DornBoardCell *current = dorn_board_cell(active->cell_id);
    int destination = -1;
    for (int i = 0; i < current->neighbor_count; ++i) {
        int candidate = current->neighbors[i];
        if (dorn_game_actor_at(&game, candidate) == NULL) {
            destination = candidate;
            break;
        }
    }
    assert(destination >= 0);
    int old_cell = active->cell_id;
    int old_movement = active->movement;
    assert(dorn_game_move(&game, destination));
    assert(active->cell_id == destination);
    assert(active->movement == old_movement - 1);
    assert(!dorn_game_move(&game, destination));
    assert(dorn_game_move(&game, old_cell));

    dorn_game_advance(&game);
    while (game.phase == DORN_PHASE_GUARDIAN_MOVE) {
        dorn_game_advance(&game);
    }
    assert(game.phase == DORN_PHASE_GUARDIAN_ATTACK);
    assert(!dorn_game_attack(&game, game.guardian.cell_id));
}

static void test_attack_and_night_cycle(void) {
    DornGame game;
    int roster[] = {0};
    dorn_game_init(&game, roster, 1, 99);
    game.phase = DORN_PHASE_HERO_ATTACK;
    game.active_actor = 0;
    DornActor *hero = &game.heroes[0];
    DornActor *target = &game.monsters[0];
    hero->cell_id = DORN_ARGOS_FIRST;
    target->cell_id = DORN_STONE_PATH_FIRST + 3;
    int old_health = target->health;
    assert(dorn_game_attack(&game, target->cell_id));
    assert(target->health == old_health - 1);
    assert(hero->attacks == 0);
    game.phase = DORN_PHASE_END;
    bool old_night = game.night;
    int old_round = game.round;
    dorn_game_advance(&game);
    assert(game.night != old_night);
    assert(game.round == old_round + 1);
    assert(game.phase == DORN_PHASE_RITUAL);
}

static void test_path_movement_and_spear_range(void) {
    DornGame game;
    int roster[] = {2, 0};
    dorn_game_init(&game, roster, 2, 404);
    DornActor *hero = &game.heroes[0];
    DornActor *target = &game.monsters[0];
    hero->cell_id = DORN_ARGOS_FIRST;
    hero->movement = 7;
    game.phase = DORN_PHASE_HERO_MOVE;
    game.active_actor = 0;

    int reachable[DORN_BOARD_CELL_COUNT];
    dorn_game_reachable(&game, reachable);
    assert(reachable[DORN_STONE_PATH_FIRST + 3] == 1);
    assert(reachable[DORN_STONE_PATH_FIRST + 4] == 2);
    assert(reachable[DORN_THRONE_FIRST] == -1);
    assert(dorn_game_move(&game, DORN_STONE_PATH_FIRST + 4));
    assert(hero->cell_id == DORN_STONE_PATH_FIRST + 4);
    assert(hero->movement == 5);

    game.phase = DORN_PHASE_HERO_ATTACK;
    target->cell_id = DORN_STONE_PATH_FIRST + 6;
    assert(dorn_game_attack(&game, target->cell_id));
    hero->attacks = 1;
    target->cell_id = DORN_STONE_PATH_FIRST + 5;
    assert(!dorn_game_attack(&game, target->cell_id));

    hero->cell_id = DORN_STONE_PATH_FIRST + 2;
    game.heroes[1].cell_id = DORN_STONE_PATH_FIRST + 3;
    hero->movement = 2;
    game.phase = DORN_PHASE_HERO_MOVE;
    assert(!dorn_game_move(&game, DORN_STONE_PATH_FIRST + 3));
    assert(dorn_game_move(&game, DORN_STONE_PATH_FIRST + 4));
}

static void test_monster_path_restrictions(void) {
    DornGame game;
    int roster[] = {0};
    dorn_game_init(&game, roster, 1, 505);
    game.phase = DORN_PHASE_GUARDIAN_MOVE;
    game.active_actor = 1;
    DornActor *monster = dorn_game_active_actor(&game);
    monster->cell_id = DORN_FOREST_FIRST + 40;
    monster->movement = 1;
    int reachable[DORN_BOARD_CELL_COUNT];
    dorn_game_reachable(&game, reachable);
    assert(reachable[DORN_STONE_PATH_FIRST] == -1);

    game.phase = DORN_PHASE_GUARDIAN_ATTACK;
    monster->cell_id = DORN_STONE_PATH_FIRST + 2;
    game.heroes[0].cell_id = DORN_STONE_PATH_FIRST + 3;
    assert(!dorn_game_attack(&game, game.heroes[0].cell_id));
}

int main(void) {
    test_initial_roster();
    test_turn_cycle_and_summoning();
    test_movement_and_attack_validation();
    test_attack_and_night_cycle();
    test_path_movement_and_spear_range();
    test_monster_path_restrictions();
    puts("Todos los tests del motor pasaron.");
    return 0;
}
