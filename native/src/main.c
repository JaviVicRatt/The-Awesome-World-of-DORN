#include "game.h"
#include "image.h"

#include <SDL.h>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WINDOW_WIDTH 1360
#define WINDOW_HEIGHT 820
#define BOARD_MAP_X 22
#define BOARD_MAP_Y 142
#define BOARD_MAP_W 846
#define BOARD_MAP_H 596
#define PANEL_X 900
#define PANEL_W 430
#define HERO_ICON_SIZE 26

typedef enum {
    SCREEN_MENU,
    SCREEN_SELECTION,
    SCREEN_GAME
} Screen;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *map;
    SDL_Texture *hero_faces[DORN_ROSTER_SIZE];
    SDL_Texture *monster_icons[8];
    SDL_Texture *guardian_face;
    SDL_AudioDeviceID audio_device;
    SDL_AudioSpec audio_spec;
    DornGame game;
    Screen screen;
    int selected_roster[DORN_MAX_HEROES];
    int selected_count;
    int hover_cell;
    bool smoke_test;
    bool running;
} App;

static const char *monster_names[8] = {
    "rata", "zombi", "esqueleto", "engendro",
    "fantasma", "momia", "vampiro", "gorgona"
};

static const char *monster_paths[8] = {
    "monster/sprite/rata.png", "monster/sprite/zombi.png",
    "monster/sprite/esqueleto.png", "monster/sprite/engendro.png",
    "monster/sprite/fantasma.png", "monster/sprite/momia.png",
    "monster/sprite/vampiro.png", "monster/sprite/gorgona.png"
};

static const uint8_t font_5x7[36][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
    {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
    {14,4,4,4,4,4,14}, {7,2,2,2,18,18,12},
    {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
    {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
    {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14}
};

static SDL_Color rgb(Uint8 r, Uint8 g, Uint8 b) {
    SDL_Color color = {r, g, b, 255};
    return color;
}

static bool is_movement_phase(DornPhase phase) {
    return phase == DORN_PHASE_GUARDIAN_MOVE ||
           phase == DORN_PHASE_HERO_MOVE;
}

static SDL_Point cell_to_screen(int cell_id) {
    const DornBoardCell *cell = dorn_board_cell(cell_id);
    SDL_Point point = {
        BOARD_MAP_X + cell->x * BOARD_MAP_W / DORN_BOARD_WIDTH,
        BOARD_MAP_Y + cell->y * BOARD_MAP_H / DORN_BOARD_HEIGHT
    };
    return point;
}

static void draw_text(App *app, int x, int y, const char *text,
                      SDL_Color color, int scale) {
    SDL_SetRenderDrawColor(app->renderer, color.r, color.g, color.b, color.a);
    for (const unsigned char *cursor = (const unsigned char *)text;
         *cursor != '\0'; ++cursor) {
        unsigned char c = (unsigned char)toupper(*cursor);
        if (c >= 'A' && c <= 'Z') {
            const uint8_t *rows = font_5x7[c - 'A'];
            for (int row = 0; row < 7; ++row) {
                for (int column = 0; column < 5; ++column) {
                    if ((rows[row] & (1u << (4 - column))) != 0) {
                        SDL_Rect pixel = {
                            x + column * scale, y + row * scale, scale, scale
                        };
                        SDL_RenderFillRect(app->renderer, &pixel);
                    }
                }
            }
        } else if (c >= '0' && c <= '9') {
            const uint8_t *rows = font_5x7[26 + c - '0'];
            for (int row = 0; row < 7; ++row) {
                for (int column = 0; column < 5; ++column) {
                    if ((rows[row] & (1u << (4 - column))) != 0) {
                        SDL_Rect pixel = {
                            x + column * scale, y + row * scale, scale, scale
                        };
                        SDL_RenderFillRect(app->renderer, &pixel);
                    }
                }
            }
        } else if (c == ':' || c == '.' || c == '-' || c == '/') {
            int column = c == ':' ? 2 : c == '.' ? 2 : c == '-' ? 1 : 2;
            int row = c == ':' ? 2 : c == '.' ? 6 : c == '-' ? 3 : 0;
            if (c == '/') {
                for (int i = 0; i < 7; ++i) {
                    SDL_Rect pixel = {x + (4 - i / 2) * scale,
                                      y + i * scale, scale, scale};
                    SDL_RenderFillRect(app->renderer, &pixel);
                }
            } else {
                SDL_Rect pixel = {x + column * scale, y + row * scale,
                                  scale, scale};
                SDL_RenderFillRect(app->renderer, &pixel);
                if (c == ':') {
                    pixel.y += 3 * scale;
                    SDL_RenderFillRect(app->renderer, &pixel);
                }
            }
        }
        x += 6 * scale;
    }
}

static void draw_centered_text(App *app, int center_x, int y, const char *text,
                               SDL_Color color, int scale) {
    draw_text(app, center_x - (int)strlen(text) * 6 * scale / 2,
              y, text, color, scale);
}

static bool asset_path(char *buffer, size_t capacity, const char *relative) {
    /* EN: Try the packaged location first, then project-root paths for development runs.
     * ES: Primero busca junto al ejecutable y luego en rutas del proyecto para desarrollo.
     */
    char *base_path = SDL_GetBasePath();
    int written = snprintf(buffer, capacity, "%sassets/%s",
                           base_path != NULL ? base_path : "", relative);
    FILE *file = written > 0 && (size_t)written < capacity
                     ? fopen(buffer, "rb") : NULL;
    if (file != NULL) {
        SDL_free(base_path);
        fclose(file);
        return true;
    }
    SDL_free(base_path);
    written = snprintf(buffer, capacity, "assets/%s", relative);
    file = written > 0 && (size_t)written < capacity ? fopen(buffer, "rb") : NULL;
    if (file != NULL) {
        fclose(file);
        return true;
    }
    written = snprintf(buffer, capacity, "src/com/dorn/assets/%s", relative);
    file = written > 0 && (size_t)written < capacity ? fopen(buffer, "rb") : NULL;
    if (file != NULL) {
        fclose(file);
        return true;
    }
    written = snprintf(buffer, capacity, "../../src/com/dorn/assets/%s", relative);
    file = written > 0 && (size_t)written < capacity ? fopen(buffer, "rb") : NULL;
    if (file != NULL) {
        fclose(file);
        return true;
    }
    return false;
}

static SDL_Texture *load_asset(App *app, const char *relative) {
    char path[512];
    if (!asset_path(path, sizeof(path), relative)) {
        fprintf(stderr, "No se encontro el recurso '%s'.\n", relative);
        return NULL;
    }
    return dorn_load_texture(app->renderer, path);
}

static void play_sound(App *app, const char *relative) {
    if (app->audio_device == 0) {
        return;
    }
    char path[512];
    if (!asset_path(path, sizeof(path), relative)) {
        return;
    }
    SDL_AudioSpec source_spec;
    Uint8 *source_data = NULL;
    Uint32 source_length = 0;
    if (SDL_LoadWAV(path, &source_spec, &source_data, &source_length) == NULL) {
        fprintf(stderr, "No se pudo cargar sonido '%s': %s\n", path, SDL_GetError());
        return;
    }
    SDL_AudioCVT conversion;
    int needs_conversion = SDL_BuildAudioCVT(
        &conversion, source_spec.format, source_spec.channels, source_spec.freq,
        app->audio_spec.format, app->audio_spec.channels, app->audio_spec.freq);
    if (needs_conversion < 0) {
        fprintf(stderr, "No se pudo convertir '%s': %s\n", path, SDL_GetError());
        SDL_FreeWAV(source_data);
        return;
    }
    if (needs_conversion == 0) {
        if (SDL_QueueAudio(app->audio_device, source_data, source_length) != 0) {
            fprintf(stderr, "No se pudo reproducir '%s': %s\n", path, SDL_GetError());
        }
    } else {
        conversion.len = (int)source_length;
        conversion.buf = (Uint8 *)SDL_malloc(
            (size_t)conversion.len * (size_t)conversion.len_mult);
        if (conversion.buf == NULL) {
            SDL_FreeWAV(source_data);
            return;
        }
        SDL_memcpy(conversion.buf, source_data, source_length);
        if (SDL_ConvertAudio(&conversion) == 0) {
            if (SDL_QueueAudio(app->audio_device, conversion.buf,
                               (Uint32)conversion.len_cvt) != 0) {
                fprintf(stderr, "No se pudo reproducir '%s': %s\n",
                        path, SDL_GetError());
            }
        } else {
            fprintf(stderr, "No se pudo convertir '%s': %s\n",
                    path, SDL_GetError());
        }
        SDL_free(conversion.buf);
    }
    SDL_FreeWAV(source_data);
}

static void draw_panel(App *app, SDL_Rect panel, SDL_Color color) {
    SDL_SetRenderDrawColor(app->renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(app->renderer, &panel);
}

static void draw_button(App *app, SDL_Rect bounds, const char *label,
                        bool enabled) {
    SDL_Color fill = enabled ? rgb(75, 59, 36) : rgb(44, 43, 41);
    draw_panel(app, bounds, fill);
    SDL_SetRenderDrawColor(app->renderer, 183, 146, 82, 255);
    SDL_RenderDrawRect(app->renderer, &bounds);
    draw_centered_text(app, bounds.x + bounds.w / 2, bounds.y + 10,
                       label, enabled ? rgb(245, 221, 173) : rgb(130, 130, 130), 2);
}

static void draw_actor(App *app, const DornActor *actor, bool active) {
    if (!actor->alive) {
        return;
    }
    SDL_Point position = cell_to_screen(actor->cell_id);
    int x = position.x;
    int y = position.y;
    SDL_Color base = actor->kind == DORN_ACTOR_GUARDIAN
                         ? rgb(176, 56, 45)
                         : actor->kind == DORN_ACTOR_HERO
                               ? rgb(47, 117, 162) : rgb(116, 58, 113);
    SDL_Rect badge = {x - HERO_ICON_SIZE / 2, y - HERO_ICON_SIZE / 2,
                      HERO_ICON_SIZE, HERO_ICON_SIZE};
    SDL_SetRenderDrawColor(app->renderer, base.r, base.g, base.b, 255);
    SDL_RenderFillRect(app->renderer, &badge);
    SDL_Texture *portrait = NULL;
    if (actor->kind == DORN_ACTOR_GUARDIAN) {
        portrait = app->guardian_face;
    } else if (actor->kind == DORN_ACTOR_HERO &&
               actor->roster_index >= 0 && actor->roster_index < DORN_ROSTER_SIZE) {
        portrait = app->hero_faces[actor->roster_index];
    } else {
        for (size_t i = 0; i < 8; ++i) {
            if (strstr(actor->portrait, monster_names[i]) != NULL) {
                portrait = app->monster_icons[i];
                break;
            }
        }
    }
    if (portrait != NULL) {
        SDL_RenderCopy(app->renderer, portrait, NULL, &badge);
    }
    if (active) {
        SDL_SetRenderDrawColor(app->renderer, 255, 220, 74, 255);
        SDL_Rect outline = {badge.x - 3, badge.y - 3, badge.w + 6, badge.h + 6};
        SDL_RenderDrawRect(app->renderer, &outline);
        SDL_RenderDrawRect(app->renderer, &badge);
    }
    SDL_Rect health = {badge.x, badge.y + badge.h + 3,
                       badge.w * actor->health / actor->max_health, 4};
    SDL_SetRenderDrawColor(app->renderer, 82, 191, 97, 255);
    SDL_RenderFillRect(app->renderer, &health);
}

static void draw_game_board(App *app) {
    SDL_Rect board = {20, 92, 850, 694};
    draw_panel(app, board, rgb(28, 29, 27));
    if (app->map != NULL) {
        SDL_SetTextureAlphaMod(app->map, 255);
        SDL_Rect map_area = {
            BOARD_MAP_X, BOARD_MAP_Y, BOARD_MAP_W, BOARD_MAP_H
        };
        SDL_RenderCopy(app->renderer, app->map, NULL, &map_area);
    }
    int reachable[DORN_BOARD_CELL_COUNT];
    dorn_game_reachable(&app->game, reachable);
    for (int cell_id = 0; cell_id < DORN_BOARD_CELL_COUNT; ++cell_id) {
        const DornBoardCell *cell = dorn_board_cell(cell_id);
        SDL_Point position = cell_to_screen(cell_id);
        SDL_Rect marker = {position.x - 3, position.y - 3, 6, 6};
        SDL_Color color = cell->hero_only ? rgb(226, 176, 73) :
                                            rgb(232, 221, 174);
        if (is_movement_phase(app->game.phase) && reachable[cell_id] > 0 &&
            dorn_game_actor_at(&app->game, cell_id) == NULL) {
            color = rgb(78, 228, 128);
            marker = (SDL_Rect){position.x - 7, position.y - 7, 14, 14};
            SDL_SetRenderDrawColor(app->renderer, 20, 25, 17, 210);
            SDL_RenderFillRect(app->renderer, &marker);
        } else if (cell_id == app->hover_cell) {
            color = rgb(255, 235, 92);
            marker = (SDL_Rect){position.x - 7, position.y - 7, 14, 14};
        }
        SDL_SetRenderDrawColor(app->renderer, color.r, color.g, color.b, 230);
        SDL_RenderDrawRect(app->renderer, &marker);
    }
    DornActor *active = dorn_game_active_actor(&app->game);
    draw_actor(app, &app->game.guardian, active == &app->game.guardian);
    for (int i = 0; i < app->game.hero_count; ++i) {
        draw_actor(app, &app->game.heroes[i], active == &app->game.heroes[i]);
    }
    for (int i = 0; i < app->game.monster_count; ++i) {
        draw_actor(app, &app->game.monsters[i], active == &app->game.monsters[i]);
    }
}

static void draw_game(App *app) {
    draw_game_board(app);
    SDL_Rect panel = {PANEL_X, 92, PANEL_W, 694};
    draw_panel(app, panel, rgb(27, 25, 24));
    draw_text(app, PANEL_X + 20, 116, "DORN", rgb(220, 190, 126), 4);

    char line[96];
    snprintf(line, sizeof(line), "RONDA: %d", app->game.round);
    draw_text(app, PANEL_X + 22, 160, line, rgb(219, 212, 190), 2);
    snprintf(line, sizeof(line), "FASE: %s", dorn_game_phase_name(app->game.phase));
    draw_text(app, PANEL_X + 22, 182, line, rgb(219, 212, 190), 2);
    draw_text(app, PANEL_X + 22, 204, app->game.night ? "NOCHE" : "DIA",
              app->game.night ? rgb(130, 160, 222) : rgb(234, 204, 111), 2);

    DornActor *active = dorn_game_active_actor(&app->game);
    draw_text(app, PANEL_X + 22, 242, "ACTOR", rgb(220, 190, 126), 2);
    if (active != NULL) {
        snprintf(line, sizeof(line), "%s  VIDA %d/%d", active->name,
                 active->health, active->max_health);
        draw_text(app, PANEL_X + 22, 262, line, rgb(235, 235, 224), 2);
        snprintf(line, sizeof(line), "MOV %d  ATAQUES %d", active->movement,
                 active->attacks);
        draw_text(app, PANEL_X + 22, 282, line, rgb(191, 196, 180), 2);
        const DornBoardCell *cell = dorn_board_cell(active->cell_id);
        snprintf(line, sizeof(line), "CASILLA: %s", cell->label);
        draw_text(app, PANEL_X + 22, 304, line, rgb(191, 196, 180), 1);
    } else {
        draw_text(app, PANEL_X + 22, 262, "--", rgb(191, 196, 180), 2);
    }

    if (app->game.phase == DORN_PHASE_RITUAL ||
        app->game.phase == DORN_PHASE_SUMMON) {
        draw_text(app, PANEL_X + 22, 325, "RITUALES DEL GUARDIAN",
                  rgb(220, 190, 126), 2);
        for (int i = 0; i < DORN_INITIAL_RITUALS; ++i) {
            snprintf(line, sizeof(line), "%d. %s", i + 1,
                     app->game.ritual_cards[i]);
            draw_text(app, PANEL_X + 22, 350 + i * 20, line,
                      rgb(205, 215, 204), 1);
        }
    } else if (app->game.phase == DORN_PHASE_HERO_BLESSING) {
        draw_text(app, PANEL_X + 22, 325, "BENDICIONES",
                  rgb(220, 190, 126), 2);
        for (int i = 0; i < app->game.hero_count; ++i) {
            snprintf(line, sizeof(line), "%s: %s",
                     app->game.heroes[i].name, app->game.hero_blessings[i]);
            draw_text(app, PANEL_X + 22, 350 + i * 24, line,
                      rgb(205, 215, 204), 1);
        }
    } else {
        draw_text(app, PANEL_X + 22, 325, "JUGADORES",
                  rgb(220, 190, 126), 2);
        for (int i = 0; i < app->game.hero_count; ++i) {
            DornActor *hero = &app->game.heroes[i];
            snprintf(line, sizeof(line), "%s %d/%d", hero->name, hero->health,
                     hero->max_health);
            draw_text(app, PANEL_X + 22, 350 + i * 24, line,
                      hero->alive ? rgb(205, 215, 204) : rgb(115, 91, 91), 2);
        }
    }
    snprintf(line, sizeof(line), "CRIATURAS: %d", app->game.monster_count);
    draw_text(app, PANEL_X + 22, 490, line, rgb(205, 178, 154), 2);
    draw_text(app, PANEL_X + 22, 528, app->game.status, rgb(225, 218, 200), 1);
    draw_button(app, (SDL_Rect){PANEL_X + 22, 580, 180, 38},
                "ENTER: SIGUIENTE", true);
    draw_button(app, (SDL_Rect){PANEL_X + 22, 628, 180, 38},
                "Q: INVOCAR", false);
    draw_text(app, PANEL_X + 22, 680, "MOV: CLIC EN VERDE", rgb(164, 191, 158), 1);
    draw_text(app, PANEL_X + 22, 700, "ATAQUE: CLIC EN BLANCO", rgb(164, 171, 158), 1);
    draw_text(app, PANEL_X + 22, 720, "GRAFO PARCIAL DEL MAPA", rgb(204, 171, 116), 1);
    draw_text(app, PANEL_X + 22, 746, "ESC: MENU", rgb(164, 171, 158), 1);
}

static void draw_menu(App *app) {
    draw_panel(app, (SDL_Rect){0, 0, WINDOW_WIDTH, WINDOW_HEIGHT},
               rgb(23, 22, 20));
    if (app->map != NULL) {
        SDL_SetTextureAlphaMod(app->map, 150);
        SDL_RenderCopy(app->renderer, app->map, NULL,
                       &(SDL_Rect){0, 0, WINDOW_WIDTH, WINDOW_HEIGHT});
        SDL_SetTextureAlphaMod(app->map, 255);
    }
    draw_centered_text(app, WINDOW_WIDTH / 2, 220,
                       "THE AWESOME WORLD OF DORN", rgb(223, 195, 136), 5);
    draw_centered_text(app, WINDOW_WIDTH / 2, 320,
                       "JUEGO DE TABLERO PARA VARIOS JUGADORES", rgb(234, 225, 202), 2);
    draw_button(app, (SDL_Rect){WINDOW_WIDTH / 2 - 130, 410, 260, 55},
                "ENTER: NUEVA PARTIDA", true);
    draw_text(app, WINDOW_WIDTH / 2 - 190, 510,
              "SELECCIONA DE UNO A CINCO HEROES Y JUEGA CONTRA ZORKAL.",
              rgb(199, 194, 180), 2);
    draw_text(app, WINDOW_WIDTH / 2 - 95, 550,
              "ESC: SALIR", rgb(199, 194, 180), 2);
}

static void draw_selection(App *app) {
    draw_panel(app, (SDL_Rect){0, 0, WINDOW_WIDTH, WINDOW_HEIGHT},
               rgb(28, 26, 23));
    draw_centered_text(app, WINDOW_WIDTH / 2, 58,
                       "SELECCION DE HEROES", rgb(222, 193, 131), 4);
    draw_text(app, 80, 112,
              "PULSA 1-9 PARA ANADIR/QUITAR. MAXIMO CINCO. ENTER INICIA.",
              rgb(215, 211, 196), 2);
    for (int i = 0; i < DORN_ROSTER_SIZE; ++i) {
        int column = i % 3;
        int row = i / 3;
        int x = 125 + column * 370;
        int y = 190 + row * 185;
        SDL_Rect card = {x, y, 300, 146};
        bool selected = false;
        for (int j = 0; j < app->selected_count; ++j) {
            selected |= app->selected_roster[j] == i;
        }
        draw_panel(app, card, selected ? rgb(73, 64, 43) : rgb(44, 42, 38));
        SDL_SetRenderDrawColor(app->renderer, selected ? 220 : 120,
                               selected ? 181 : 115,
                               selected ? 95 : 102, 255);
        SDL_RenderDrawRect(app->renderer, &card);
        if (app->hero_faces[i] != NULL) {
            SDL_Rect face = {x + 13, y + 13, 84, 112};
            SDL_RenderCopy(app->renderer, app->hero_faces[i], NULL, &face);
        }
        char line[80];
        snprintf(line, sizeof(line), "%d. %s", i + 1, dorn_heroes[i].name);
        draw_text(app, x + 110, y + 22, line, rgb(240, 226, 192), 2);
        snprintf(line, sizeof(line), "VIDA %d  MOV %d", dorn_heroes[i].health,
                 dorn_heroes[i].movement);
        draw_text(app, x + 110, y + 54, line, rgb(192, 194, 182), 1);
        snprintf(line, sizeof(line), "ATAQUE %d  ALCANCE %d",
                 dorn_heroes[i].damage, dorn_heroes[i].attack_range);
        draw_text(app, x + 110, y + 76, line, rgb(192, 194, 182), 1);
        draw_text(app, x + 110, y + 112, selected ? "SELECCIONADO" : "",
                  rgb(222, 193, 131), 1);
    }
    char selection[80] = "ELEGIDOS: ";
    for (int i = 0; i < app->selected_count; ++i) {
        size_t used = strlen(selection);
        snprintf(selection + used, sizeof(selection) - used, "%s%s",
                 i == 0 ? "" : ", ", dorn_heroes[app->selected_roster[i]].name);
    }
    draw_text(app, 95, 760, selection, rgb(233, 218, 187), 2);
    draw_text(app, 970, 760, "BACKSPACE: VOLVER", rgb(181, 178, 164), 1);
}

static void render(App *app) {
    SDL_SetRenderDrawColor(app->renderer, 20, 19, 18, 255);
    SDL_RenderClear(app->renderer);
    if (app->screen == SCREEN_MENU) {
        draw_menu(app);
    } else if (app->screen == SCREEN_SELECTION) {
        draw_selection(app);
    } else {
        draw_game(app);
    }
    SDL_RenderPresent(app->renderer);
}

static void toggle_roster(App *app, int index) {
    if (index < 0 || index >= DORN_ROSTER_SIZE) {
        return;
    }
    for (int i = 0; i < app->selected_count; ++i) {
        if (app->selected_roster[i] == index) {
            for (int j = i; j + 1 < app->selected_count; ++j) {
                app->selected_roster[j] = app->selected_roster[j + 1];
            }
            app->selected_count--;
            return;
        }
    }
    if (app->selected_count < DORN_MAX_HEROES) {
        app->selected_roster[app->selected_count++] = index;
    }
}

static void start_game(App *app) {
    if (app->selected_count == 0) {
        return;
    }
    dorn_game_init(&app->game, app->selected_roster, app->selected_count,
                   (uint32_t)SDL_GetTicks());
    app->screen = SCREEN_GAME;
}

static void handle_key(App *app, SDL_Keycode key) {
    if (key == SDLK_ESCAPE) {
        if (app->screen == SCREEN_GAME || app->screen == SCREEN_SELECTION) {
            app->screen = SCREEN_MENU;
            play_sound(app, "sound/click.wav");
        } else {
            app->running = false;
        }
        return;
    }
    if (app->screen == SCREEN_MENU) {
        if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            app->selected_count = 0;
            app->screen = SCREEN_SELECTION;
        }
        return;
    }
    if (app->screen == SCREEN_SELECTION) {
        if (key >= SDLK_1 && key <= SDLK_9) {
            toggle_roster(app, (int)(key - SDLK_1));
            play_sound(app, "sound/click.wav");
        } else if (key == SDLK_RETURN || key == SDLK_KP_ENTER) {
            start_game(app);
        } else if (key == SDLK_BACKSPACE) {
            app->screen = SCREEN_MENU;
        }
        return;
    }

    switch (key) {
        case SDLK_q:
            if (dorn_game_summon(&app->game)) {
                play_sound(app, "sound/magic.wav");
            }
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            dorn_game_advance(&app->game);
            play_sound(app, "sound/click.wav");
            break;
        default:
            break;
    }
}

static int mouse_to_cell(int mouse_x, int mouse_y) {
    if (mouse_x < BOARD_MAP_X || mouse_y < BOARD_MAP_Y ||
        mouse_x >= BOARD_MAP_X + BOARD_MAP_W ||
        mouse_y >= BOARD_MAP_Y + BOARD_MAP_H) {
        return -1;
    }
    int nearest_cell = -1;
    int nearest_distance_squared = INT_MAX;
    /* EN: Map screen coordinates to the nearest graph node, within the marker hit area.
     * ES: Asocia el clic al nodo del grafo mas cercano, dentro del area del marcador.
     */
    for (int cell_id = 0; cell_id < DORN_BOARD_CELL_COUNT; ++cell_id) {
        SDL_Point position = cell_to_screen(cell_id);
        int dx = position.x - mouse_x;
        int dy = position.y - mouse_y;
        int distance_squared = dx * dx + dy * dy;
        if (distance_squared < nearest_distance_squared) {
            nearest_cell = cell_id;
            nearest_distance_squared = distance_squared;
        }
    }
    if (nearest_distance_squared > 18 * 18) {
        return -1;
    }
    return nearest_cell;
}

static bool point_on_board_map(int x, int y) {
    return x >= BOARD_MAP_X && y >= BOARD_MAP_Y &&
           x < BOARD_MAP_X + BOARD_MAP_W && y < BOARD_MAP_Y + BOARD_MAP_H;
}

static bool init_textures(App *app) {
    bool loaded = true;
    app->map = load_asset(app, "map/mapa_escalado.jpg");
    app->guardian_face = load_asset(app, "heroe/face/zorkal.png");
    loaded &= app->map != NULL && app->guardian_face != NULL;
    for (int i = 0; i < DORN_ROSTER_SIZE; ++i) {
        app->hero_faces[i] = load_asset(app, dorn_heroes[i].portrait);
        loaded &= app->hero_faces[i] != NULL;
    }
    for (int i = 0; i < 8; ++i) {
        app->monster_icons[i] = load_asset(app, monster_paths[i]);
        loaded &= app->monster_icons[i] != NULL;
    }
    return loaded;
}

static void destroy_app(App *app) {
    if (app->audio_device != 0) {
        SDL_CloseAudioDevice(app->audio_device);
    }
    if (app->map != NULL) SDL_DestroyTexture(app->map);
    if (app->guardian_face != NULL) SDL_DestroyTexture(app->guardian_face);
    for (int i = 0; i < DORN_ROSTER_SIZE; ++i) {
        if (app->hero_faces[i] != NULL) SDL_DestroyTexture(app->hero_faces[i]);
    }
    for (int i = 0; i < 8; ++i) {
        if (app->monster_icons[i] != NULL) SDL_DestroyTexture(app->monster_icons[i]);
    }
    if (app->renderer != NULL) SDL_DestroyRenderer(app->renderer);
    if (app->window != NULL) SDL_DestroyWindow(app->window);
    SDL_Quit();
}

int main(int argc, char **argv) {
    App app;
    memset(&app, 0, sizeof(app));
    app.running = true;
    app.screen = SCREEN_MENU;
    app.hover_cell = -1;
    app.smoke_test = argc > 1 && strcmp(argv[1], "--smoke-test") == 0;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "No se pudo iniciar SDL2: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    app.window = SDL_CreateWindow(
        "The Awesome World of DORN",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (app.window == NULL) {
        fprintf(stderr, "No se pudo crear la ventana: %s\n", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }
    app.renderer = SDL_CreateRenderer(
        app.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (app.renderer == NULL) {
        app.renderer = SDL_CreateRenderer(app.window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (app.renderer == NULL) {
        fprintf(stderr, "No se pudo crear el renderer: %s\n", SDL_GetError());
        destroy_app(&app);
        return EXIT_FAILURE;
    }
    SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) == 0) {
        SDL_AudioSpec wanted;
        SDL_zero(wanted);
        wanted.freq = 44100;
        wanted.format = AUDIO_S16SYS;
        wanted.channels = 2;
        wanted.samples = 2048;
        app.audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted,
                                               &app.audio_spec, 0);
        if (app.audio_device != 0) {
            SDL_PauseAudioDevice(app.audio_device, 0);
        } else {
            fprintf(stderr, "Audio desactivado: %s\n", SDL_GetError());
        }
    } else {
        fprintf(stderr, "Audio desactivado: %s\n", SDL_GetError());
    }
    bool textures_loaded = init_textures(&app);
    if (app.smoke_test && !textures_loaded) {
        destroy_app(&app);
        return EXIT_FAILURE;
    }
    int rendered_frames = 0;
    while (app.running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                app.running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                handle_key(&app, event.key.keysym.sym);
            } else if (event.type == SDL_MOUSEBUTTONDOWN &&
                       event.button.button == SDL_BUTTON_LEFT &&
                       app.screen == SCREEN_GAME) {
                /* EN: The game engine validates clicks; the UI only forwards the intent.
                 * ES: El motor valida los clics; la interfaz solo reenvia la accion.
                 */
                int cell_id = mouse_to_cell(event.button.x, event.button.y);
                if (cell_id >= 0 && is_movement_phase(app.game.phase)) {
                    if (dorn_game_move(&app.game, cell_id)) {
                        play_sound(&app, "sound/move.wav");
                    }
                } else if (cell_id >= 0 &&
                           dorn_game_attack(&app.game, cell_id)) {
                    play_sound(&app, "sound/attack0.wav");
                }
            } else if (event.type == SDL_MOUSEMOTION &&
                       app.screen == SCREEN_GAME) {
                app.hover_cell = point_on_board_map(event.motion.x, event.motion.y)
                                     ? mouse_to_cell(event.motion.x, event.motion.y)
                                     : -1;
            }
        }
        render(&app);
        if (app.smoke_test && ++rendered_frames >= 3) {
            app.running = false;
        }
        SDL_Delay(16);
    }
    destroy_app(&app);
    return EXIT_SUCCESS;
}
