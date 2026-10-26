#ifndef DORN_BOARD_H
#define DORN_BOARD_H

#include <stdbool.h>

#define DORN_BOARD_MAX_NEIGHBORS 8
#define DORN_BOARD_WIDTH 3354
#define DORN_BOARD_HEIGHT 2363

typedef enum {
    DORN_CELL_THRONE,
    DORN_CELL_ARGOS,
    DORN_CELL_STONE_PATH,
    DORN_CELL_MINE,
    DORN_CELL_FOREST,
    DORN_CELL_HERBALIST,
    DORN_CELL_INITIAL_SPAWN
} DornCellKind;

/* EN: Each traversable space is a graph node; x/y are only render coordinates.
 * ES: Cada casilla transitable es un nodo del grafo; x/y solo ubican el dibujo.
 */
typedef struct {
    char label[8];
    DornCellKind kind;
    int x;
    int y;
    int neighbors[DORN_BOARD_MAX_NEIGHBORS];
    int neighbor_count;
    /* EN: The ochre path is reserved for heroes and cannot be entered by monsters.
     * ES: El camino ocre es exclusivo de los heroes y los monstruos no pueden entrar.
     */
    bool hero_only;
} DornBoardCell;

enum {
    DORN_THRONE_FIRST = 0,
    DORN_ARGOS_FIRST = 11,
    DORN_STONE_PATH_FIRST = 16,
    DORN_MINE_FIRST = 31,
    DORN_FOREST_FIRST = 111,
    DORN_HERBALIST_CELL = 221,
    DORN_INITIAL_SPAWN_FIRST = 222,
    DORN_BOARD_CELL_COUNT = 226
};

void dorn_board_init(void);
const DornBoardCell *dorn_board_cell(int cell_id);
int dorn_board_distance(int start_cell, int target_cell);

#endif
