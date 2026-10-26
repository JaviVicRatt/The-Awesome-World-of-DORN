#include "board.h"

#include <stdio.h>
#include <string.h>

static DornBoardCell cells[DORN_BOARD_CELL_COUNT];
static bool initialized;

static void set_cell(int id, DornCellKind kind, const char *label, int x, int y) {
    DornBoardCell *cell = &cells[id];
    memset(cell, 0, sizeof(*cell));
    snprintf(cell->label, sizeof(cell->label), "%s", label);
    cell->kind = kind;
    cell->x = x;
    cell->y = y;
}

static bool add_neighbor(int source, int target) {
    DornBoardCell *cell = &cells[source];
    for (int i = 0; i < cell->neighbor_count; ++i) {
        if (cell->neighbors[i] == target) {
            return true;
        }
    }
    if (cell->neighbor_count >= DORN_BOARD_MAX_NEIGHBORS) {
        return false;
    }
    cell->neighbors[cell->neighbor_count++] = target;
    return true;
}

static bool has_neighbor(int source, int target) {
    const DornBoardCell *cell = &cells[source];
    for (int i = 0; i < cell->neighbor_count; ++i) {
        if (cell->neighbors[i] == target) {
            return true;
        }
    }
    return false;
}

static bool connect_cells(int first, int second) {
    bool first_connected = has_neighbor(first, second);
    bool second_connected = has_neighbor(second, first);
    if (first_connected && second_connected) {
        return true;
    }
    if ((!first_connected && cells[first].neighbor_count >= DORN_BOARD_MAX_NEIGHBORS) ||
        (!second_connected &&
         cells[second].neighbor_count >= DORN_BOARD_MAX_NEIGHBORS)) {
        fprintf(stderr, "La casilla %s excede el limite de conexiones.\n",
                cells[first_connected ? second : first].label);
        return false;
    }
    return add_neighbor(first, second) && add_neighbor(second, first);
}

static void set_forest_link(int links[110][8], int first, int direction,
                            int second) {
    static const int opposite[8] = {1, 0, 3, 2, 7, 6, 5, 4};
    if (first < 0 || first >= 110 || second < 0 || second >= 110 ||
        links[first][direction] != -1) {
        return;
    }
    links[first][direction] = second;
    if (links[second][opposite[direction]] == -1) {
        links[second][opposite[direction]] = first;
    }
    (void)connect_cells(DORN_FOREST_FIRST + first, DORN_FOREST_FIRST + second);
}

static int point_distance(int first, int second) {
    int dx = cells[first].x - cells[second].x;
    int dy = cells[first].y - cells[second].y;
    int squared = dx * dx + dy * dy;
    int low = 0;
    int high = squared;
    /* EN: Integer square root keeps board-link checks deterministic without floating point.
     * ES: La raiz cuadrada entera hace deterministas los enlaces sin usar coma flotante.
     */
    while (low < high) {
        int middle = low + (high - low + 1) / 2;
        if (middle <= squared / middle) {
            low = middle;
        } else {
            high = middle - 1;
        }
    }
    return low;
}

static void build_cells(void) {
    static const int throne[11][2] = {
        {1517, 206}, {1592, 254}, {1676, 302}, {1753, 254}, {1828, 206},
        {1592, 350}, {1676, 381}, {1753, 350}, {1592, 446}, {1676, 494},
        {1753, 446}
    };
    static const int argos[5][2] = {
        {1680, 2140}, {1515, 2075}, {1515, 2225}, {1800, 2225}, {1800, 2075}
    };
    static const int stone_path[15][2] = {
        {1147, 1715}, {1280, 1836}, {1473, 1864}, {1685, 1867},
        {1912, 1730}, {2044, 1603}, {2243, 1579}, {2493, 1585},
        {2727, 1585}, {2973, 1570}, {3187, 1567}, {3209, 1242},
        {3200, 931}, {3228, 659}, {3223, 478}
    };
    for (int i = 0; i < 11; ++i) {
        char label[8];
        snprintf(label, sizeof(label), "T%d", i);
        set_cell(i, DORN_CELL_THRONE, label, throne[i][0], throne[i][1]);
    }
    for (int i = 0; i < 5; ++i) {
        char label[8];
        snprintf(label, sizeof(label), "A%d", i);
        set_cell(DORN_ARGOS_FIRST + i, DORN_CELL_ARGOS, label,
                 argos[i][0], argos[i][1]);
    }
    for (int i = 0; i < 15; ++i) {
        char label[8];
        snprintf(label, sizeof(label), "P%d", i);
        set_cell(DORN_STONE_PATH_FIRST + i, DORN_CELL_STONE_PATH, label,
                 stone_path[i][0], stone_path[i][1]);
        cells[DORN_STONE_PATH_FIRST + i].hero_only = true;
    }

    for (int i = 0; i < 80; ++i) {
        int x;
        int y;
        if (i < 24) {
            x = 2130 + (i % 12) * 102;
            y = 1760 + (i / 12) * 100;
        } else {
            int index = i - 24;
            x = 1930 + (index % 14) * 102;
            y = 1960 + (index / 14) * 100;
        }
        char label[8];
        snprintf(label, sizeof(label), "M%d", i + 1);
        set_cell(DORN_MINE_FIRST + i, DORN_CELL_MINE, label, x, y);
    }

    static const int forest_row_lengths[] = {3, 7, 6, 6, 10, 9, 10, 11, 10, 13, 11, 13};
    int forest_index = 0;
    int row_y = 1411;
    for (size_t row = 0; row < sizeof(forest_row_lengths) /
                                   sizeof(forest_row_lengths[0]); ++row) {
        int interruptions = 0;
        int length = forest_row_lengths[row];
        for (int column = 0; column < length; ++column, ++forest_index) {
            int base_x = 0;
            switch (row) {
                case 0: base_x = 518; break;
                case 1: base_x = 202; break;
                case 2: base_x = 155; break;
                case 3: base_x = 202; break;
                case 4: base_x = 155; break;
                case 5: base_x = 202; break;
                case 6: base_x = 155; break;
                case 7: base_x = 110; break;
                case 8: base_x = 334; break;
                case 9: base_x = 110; break;
                case 10: base_x = 155; break;
                default: base_x = 110; break;
            }
            int extra = 0;
            if (row == 2 && (column == 3 || column == 5)) extra = ++interruptions;
            if (row == 3 && column == 1) extra = ++interruptions;
            if (row == 5 && column == 6) extra = ++interruptions;
            if (row == 6 && column == 6) extra = ++interruptions;
            if (row == 7 && (column == 3 || column == 4)) extra = ++interruptions;
            if (row == 8 && column == 12) extra = 2;
            if (row == 9 && column == 2) extra = ++interruptions;
            if (row == 9 && column == 7) extra = interruptions += 2;
            if (row == 10 && column == 2) extra = ++interruptions;
            if (row == 10 && column == 7) extra = interruptions += 2;
            if (row == 11 && (column == 3 || column == 7)) extra = ++interruptions;
            char label[8];
            snprintf(label, sizeof(label), "B%d", forest_index);
            set_cell(DORN_FOREST_FIRST + forest_index, DORN_CELL_FOREST, label,
                     base_x + (column + extra) * 89, row_y);
        }
        row_y += 75;
    }
    set_cell(DORN_FOREST_FIRST + 109, DORN_CELL_FOREST, "B109", 1178, 2095);
    set_cell(DORN_HERBALIST_CELL, DORN_CELL_HERBALIST, "S0", 744, 1317);

    static const int initial_spawns[4][3] = {
        {47, 523, 1232}, {69, 1282, 1569}, {96, 2155, 1436}, {42, 2363, 723}
    };
    for (int i = 0; i < 4; ++i) {
        char label[8];
        snprintf(label, sizeof(label), "H%d", initial_spawns[i][0]);
        set_cell(DORN_INITIAL_SPAWN_FIRST + i, DORN_CELL_INITIAL_SPAWN,
                 label, initial_spawns[i][1], initial_spawns[i][2]);
    }
}

static void connect_mine(int first, int second) {
    (void)connect_cells(DORN_MINE_FIRST + first, DORN_MINE_FIRST + second);
}

static void disconnect_mine(int first, int second) {
    int a = DORN_MINE_FIRST + first;
    int b = DORN_MINE_FIRST + second;
    for (int i = 0; i < cells[a].neighbor_count; ++i) {
        if (cells[a].neighbors[i] == b) {
            cells[a].neighbors[i] = cells[a].neighbors[--cells[a].neighbor_count];
            break;
        }
    }
    for (int i = 0; i < cells[b].neighbor_count; ++i) {
        if (cells[b].neighbors[i] == a) {
            cells[b].neighbors[i] = cells[b].neighbors[--cells[b].neighbor_count];
            break;
        }
    }
}

static void connect_map(void) {
    /* EN: Movement follows verified board links, not visual proximity or diagonals.
     * ES: El movimiento sigue enlaces verificados, no la proximidad visual ni diagonales.
     */
    static const int throne_edges[][2] = {
        {0, 1}, {1, 2}, {1, 5}, {2, 3}, {2, 5}, {2, 6}, {2, 7},
        {3, 4}, {3, 7}, {5, 8}, {5, 6}, {6, 7}, {6, 8}, {6, 9},
        {6, 10}, {7, 10}, {8, 9}, {9, 10}
    };
    for (size_t i = 0; i < sizeof(throne_edges) / sizeof(throne_edges[0]); ++i) {
        (void)connect_cells(throne_edges[i][0], throne_edges[i][1]);
    }

    for (int i = 0; i < 4; ++i) {
        (void)connect_cells(DORN_STONE_PATH_FIRST + i,
                            DORN_STONE_PATH_FIRST + i + 1);
    }
    for (int i = 4; i < 10; ++i) {
        (void)connect_cells(DORN_STONE_PATH_FIRST + i,
                            DORN_STONE_PATH_FIRST + i + 1);
    }
    for (int i = 10; i < 14; ++i) {
        (void)connect_cells(DORN_STONE_PATH_FIRST + i,
                            DORN_STONE_PATH_FIRST + i + 1);
    }
    for (int i = 0; i < 5; ++i) {
        (void)connect_cells(DORN_ARGOS_FIRST + i, DORN_STONE_PATH_FIRST + 3);
    }
    (void)connect_cells(DORN_STONE_PATH_FIRST, DORN_FOREST_FIRST + 40);
    (void)connect_cells(DORN_HERBALIST_CELL, DORN_FOREST_FIRST + 2);

    (void)connect_cells(DORN_MINE_FIRST, DORN_MINE_FIRST + 1);
    (void)connect_cells(DORN_MINE_FIRST, DORN_MINE_FIRST + 12);
    (void)connect_cells(DORN_MINE_FIRST + 1, DORN_STONE_PATH_FIRST + 6);
    (void)connect_cells(DORN_MINE_FIRST + 2, DORN_MINE_FIRST + 3);
    for (int i = 3; i < 10; ++i) {
        (void)connect_cells(DORN_MINE_FIRST + i, DORN_MINE_FIRST + i + 1);
    }
    (void)connect_cells(DORN_MINE_FIRST + 3, DORN_MINE_FIRST + 15);
    for (int i = 4; i < 10; ++i) {
        (void)connect_cells(DORN_MINE_FIRST + i, DORN_MINE_FIRST + i + 12);
    }
    (void)connect_cells(DORN_MINE_FIRST + 10, DORN_MINE_FIRST + 22);
    (void)connect_cells(DORN_MINE_FIRST + 11, DORN_STONE_PATH_FIRST + 10);
    (void)connect_cells(DORN_MINE_FIRST + 11, DORN_MINE_FIRST + 23);
    for (int i = 12; i < 14; ++i) {
        (void)connect_cells(DORN_MINE_FIRST + i, DORN_MINE_FIRST + i + 1);
        (void)connect_cells(DORN_MINE_FIRST + i, DORN_MINE_FIRST + i + 14);
    }
    for (int i = 14; i < 20; ++i) {
        (void)connect_cells(DORN_MINE_FIRST + i, DORN_MINE_FIRST + i + 1);
    }
    for (int i = 16; i < 20; ++i) {
        (void)connect_cells(DORN_MINE_FIRST + i, DORN_MINE_FIRST + i + 14);
    }
    (void)connect_cells(DORN_MINE_FIRST + 21, DORN_MINE_FIRST + 22);
    (void)connect_cells(DORN_MINE_FIRST + 21, DORN_MINE_FIRST + 35);
    (void)connect_cells(DORN_MINE_FIRST + 22, DORN_MINE_FIRST + 23);
    for (int i = 24; i < 80; ++i) {
        int row = (i - 24) / 14;
        int column = (i - 24) % 14;
        if (column < 13) connect_mine(i, i + 1);
        if (row < 3) connect_mine(i, i + 14);
    }
    static const int mine_right_breaks[] = {
        26, 29, 33, 38, 39, 42, 44, 46, 49, 52, 56, 58, 62, 63, 71
    };
    static const int mine_down_breaks[] = {
        26, 27, 32, 35, 36, 40, 41, 42, 44, 45, 47, 48, 51, 52, 53, 54
    };
    for (size_t i = 0; i < sizeof(mine_right_breaks) /
                                   sizeof(mine_right_breaks[0]); ++i) {
        disconnect_mine(mine_right_breaks[i], mine_right_breaks[i] + 1);
    }
    for (size_t i = 0; i < sizeof(mine_down_breaks) /
                                   sizeof(mine_down_breaks[0]); ++i) {
        disconnect_mine(mine_down_breaks[i], mine_down_breaks[i] + 14);
    }

    int forest_links[110][8];
    for (int i = 0; i < 110; ++i) {
        for (int direction = 0; direction < 8; ++direction) {
            forest_links[i][direction] = -1;
        }
    }
    static const int row_lengths[] = {3, 7, 6, 6, 10, 9, 10, 11, 10, 13, 11, 13};
    int offset = 0;
    for (size_t row = 0; row < sizeof(row_lengths) / sizeof(row_lengths[0]); ++row) {
        for (int column = 0; column + 1 < row_lengths[row]; ++column) {
            int first = offset + column;
            if (cells[DORN_FOREST_FIRST + first + 1].x -
                    cells[DORN_FOREST_FIRST + first].x < 90) {
                set_forest_link(forest_links, first, 1, first + 1);
            }
        }
        offset += row_lengths[row];
    }
    double diagonal_limit = point_distance(DORN_FOREST_FIRST,
                                          DORN_FOREST_FIRST + 7) * 1.1;
    for (int i = 0; i < 96; ++i) {
        for (int candidate = i + 5; candidate < i + 16 && candidate < 110;
             ++candidate) {
            int first = DORN_FOREST_FIRST + i;
            int second = DORN_FOREST_FIRST + candidate;
            if (cells[first].x < cells[second].x &&
                cells[first].y < cells[second].y &&
                point_distance(first, second) <= diagonal_limit) {
                set_forest_link(forest_links, i, 5, candidate);
                int left = forest_links[candidate][0];
                if (left >= 0 && point_distance(first, DORN_FOREST_FIRST + left) <=
                                     diagonal_limit) {
                    set_forest_link(forest_links, i, 6, left);
                }
                break;
            }
        }
    }
    for (int i = 0; i < 108; ++i) {
        if (forest_links[i][4] == -1) {
            int lower_right = forest_links[i][7];
            if (lower_right >= 0 && forest_links[lower_right][1] >= 0) {
                set_forest_link(forest_links, i, 4,
                                forest_links[lower_right][1]);
            }
        }
    }
    set_forest_link(forest_links, 21, 4, 15);
    set_forest_link(forest_links, 95, 4, 84);
    set_forest_link(forest_links, 109, 4, 70);
    set_forest_link(forest_links, 109, 5, 94);
    set_forest_link(forest_links, 109, 6, 93);
    set_forest_link(forest_links, 109, 0, 83);
    set_forest_link(forest_links, 109, 7, 69);

}

void dorn_board_init(void) {
    if (initialized) {
        return;
    }
    build_cells();
    connect_map();
    initialized = true;
}

const DornBoardCell *dorn_board_cell(int cell_id) {
    dorn_board_init();
    if (cell_id < 0 || cell_id >= DORN_BOARD_CELL_COUNT) {
        return NULL;
    }
    return &cells[cell_id];
}

int dorn_board_distance(int start_cell, int target_cell) {
    dorn_board_init();
    if (start_cell < 0 || start_cell >= DORN_BOARD_CELL_COUNT ||
        target_cell < 0 || target_cell >= DORN_BOARD_CELL_COUNT) {
        return -1;
    }
    int distances[DORN_BOARD_CELL_COUNT];
    int queue[DORN_BOARD_CELL_COUNT];
    for (int i = 0; i < DORN_BOARD_CELL_COUNT; ++i) {
        distances[i] = -1;
    }
    int head = 0;
    int tail = 0;
    distances[start_cell] = 0;
    queue[tail++] = start_cell;
    /* EN: Breadth-first search returns the shortest number of legal graph edges.
     * ES: La busqueda en anchura devuelve el minimo de enlaces legales entre casillas.
     */
    while (head < tail) {
        int current = queue[head++];
        if (current == target_cell) {
            return distances[current];
        }
        for (int i = 0; i < cells[current].neighbor_count; ++i) {
            int next = cells[current].neighbors[i];
            if (distances[next] == -1) {
                distances[next] = distances[current] + 1;
                queue[tail++] = next;
            }
        }
    }
    return -1;
}
