#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <raylib.h>

#define UNREACHABLE() assert(false)
#define NOOP() /* no-op */

typedef enum Cell {
	CELL_NOTHING = 0,
	CELL_KNOWN = 1 << 0,
	CELL_BOMB = 1 << 1,
	CELL_FLAG = 1 << 2,
} Cell;

typedef enum State {
	STATE_PLAY = 0,
	STATE_WON,
	STATE_LOST,
	STATE_RESTART,
} State;

typedef struct Game {
	State state;
	int flags;
	int revealed;
	float bomb_density;
	int rows;
	int columns;
	int bombs;
	int cell_size;
	float start_time;
	float finish_time;
	char cells[];
} Game;

typedef struct Tilemap {
	Texture2D texture;
	Rectangle blank;
	Rectangle flag;
	Rectangle bomb;
	Rectangle numbers[9];
} Tilemap;

static Tilemap *load_tilemap(void) {
	const char *file_path = "tileset.png";
	int tile_size = 16;
	
	Tilemap *tilemap = MemAlloc(sizeof *tilemap);
	assert(tilemap != NULL);
	
	tilemap->texture = LoadTexture(file_path);
	tilemap->blank = CLITERAL(Rectangle){0 * tile_size, 0 * tile_size, tile_size, tile_size};
	tilemap->flag = CLITERAL(Rectangle){1 * tile_size, 0 * tile_size, tile_size, tile_size};
	tilemap->bomb = CLITERAL(Rectangle){2 * tile_size, 0 * tile_size, tile_size, tile_size};
	
	tilemap->numbers[0] = CLITERAL(Rectangle){3 * tile_size, 0 * tile_size, tile_size, tile_size};
	tilemap->numbers[1] = CLITERAL(Rectangle){0 * tile_size, 1 * tile_size, tile_size, tile_size};
	tilemap->numbers[2] = CLITERAL(Rectangle){1 * tile_size, 1 * tile_size, tile_size, tile_size};
	tilemap->numbers[3] = CLITERAL(Rectangle){2 * tile_size, 1 * tile_size, tile_size, tile_size};
	tilemap->numbers[4] = CLITERAL(Rectangle){3 * tile_size, 1 * tile_size, tile_size, tile_size};
	tilemap->numbers[5] = CLITERAL(Rectangle){0 * tile_size, 2 * tile_size, tile_size, tile_size};
	tilemap->numbers[6] = CLITERAL(Rectangle){1 * tile_size, 2 * tile_size, tile_size, tile_size};
	tilemap->numbers[7] = CLITERAL(Rectangle){2 * tile_size, 2 * tile_size, tile_size, tile_size};
	tilemap->numbers[8] = CLITERAL(Rectangle){3 * tile_size, 2 * tile_size, tile_size, tile_size};
	return tilemap;
}

static Game *create_game(int rows, int columns, float bomb_density, int cell_size) {
	Game *game = MemAlloc((sizeof *game) + (sizeof *game->cells) * rows * columns);
	assert(game != NULL);

	game->state = STATE_PLAY;
	game->flags = 0;
	game->revealed = 0;
	game->bomb_density = 0.25f;
	game->rows = rows;
	game->columns = columns;
	game->bombs = rows * columns * bomb_density;
	game->cell_size = cell_size;
	game->start_time = GetTime();
	game->finish_time = 0.0f;
	return game;
}

static void destroy_game(Game *game) {
	MemFree(game);
}

static void plant_bombs(Game *game, int safe_x, int safe_y) {
	SetRandomSeed(time(NULL));
	
	for (int planted = 0; planted < game->bombs; ) {
		int x = GetRandomValue(0, game->columns - 1);
		int y = GetRandomValue(0, game->rows - 1);

		bool safe_zone = (safe_x - 1 <= x && x <= safe_x + 1) && (safe_y - 1 <= y && y <= safe_y + 1);
		if (safe_zone) {
			continue;
		}
		
		char *cell = &game->cells[y * game->columns + x];
		if (!(*cell & CELL_BOMB)) {
			*cell |= CELL_BOMB;
			++planted;
		}
	}
}

static int count_adjacent_bombs(const Game *game, int x, int y) {
	int danger = 0;
	for (int row = y - 1; row <= y + 1; ++row) {
		if (0 > row || row >= game->rows) {
			continue;
		}
		
		for (int column = x - 1; column <= x + 1; ++column) {
			if (row == y && column == x) {
				continue;
			}
			if (0 > column || column >= game->columns) {
				continue;
			}
			
			char cell = game->cells[row * game->columns + column];
			if (cell & CELL_BOMB) {
				++danger;
			}
		}
	}
	return danger;
}

static void draw_ui(const Game *game, const Tilemap *tilemap) {
	int window_width = game->columns * game->cell_size;
	int window_height = game->rows * game->cell_size;

	const Color icon_tint = {255, 255, 255, 128};
	const Color font_color = {0, 0, 0, 200};
	
	const int margin = 32;
	const int padding = 64;
	const int font_spacing = 32;
	const int font_size = 48;
	const int icon_size = font_size;
	
	int x = margin;
	int y = margin;
	char buffer[32];
	Rectangle rectangle = {.width = font_size, .height = font_size};
	Vector2 origin = {0, 0};
	float rotation = 0.0f;

	rectangle.x = x;
	rectangle.y = y;
	DrawTexturePro(tilemap->texture, tilemap->bomb, rectangle, origin, rotation, icon_tint);

	x += font_size + font_spacing;

	snprintf(buffer, sizeof buffer, "%d", game->bombs);
	DrawText(buffer, x, y, icon_size, font_color);

	x += font_size + padding;

	rectangle.x = x;
	rectangle.y = y;
	DrawTexturePro(tilemap->texture, tilemap->flag, rectangle, origin, rotation, icon_tint);

	x += font_size + font_spacing;
	
	snprintf(buffer, sizeof buffer, "%d", game->flags);
	DrawText(buffer, x, y, icon_size, font_color);

	x = margin;
	y += font_size + font_spacing;

	snprintf(buffer, sizeof buffer, "%d/%d", game->revealed, game->rows * game->columns - game->bombs);
	DrawText(buffer, x, y, icon_size, font_color);

	x = margin;
	y = window_height - margin - font_size;

	int time = game->state == STATE_PLAY ? (GetTime() - game->start_time) : (game->finish_time - game->start_time);	
	snprintf(buffer, sizeof buffer, "%d s", time);
	DrawText(buffer, x, y, icon_size, YELLOW);	
}

static void draw_field(const Game *game, const Tilemap *tilemap) {
	int cell_size = game->cell_size;

	Rectangle rectangle;
	rectangle.width = cell_size;
	rectangle.height = cell_size;

	int mouse_x = GetMouseX();
	int mouse_y = GetMouseY();
	
	for (int y = 0; y < game->rows; ++y) {
		rectangle.y = y * cell_size;
		
		for (int x = 0; x < game->columns; ++x) {
			rectangle.x = x * cell_size;
			
			Rectangle tile = tilemap->blank;
			Color tint = WHITE;
			
			char cell = game->cells[y * game->columns + x];
			if (game->state == STATE_PLAY) {
				bool mouse_over = (x * cell_size < mouse_x && mouse_x < (x + 1) * cell_size) && (y * cell_size < mouse_y && mouse_y < (y + 1) * cell_size);
				if (mouse_over) {
					tint = CLITERAL(Color){230, 230, 230, 255};
				}
				
				if (cell & CELL_KNOWN) {
					int danger = count_adjacent_bombs(game, x, y);
					assert(0 <= danger && danger <= 8);
					tile = tilemap->numbers[danger];
				}
				else if (cell & CELL_FLAG) {
					tile = tilemap->flag;
				}
			} else if (game->state == STATE_WON) {
				if (cell & CELL_FLAG) {
					tile = tilemap->flag;
				}
				else if (cell & CELL_BOMB) {
					tile = tilemap->bomb;
				}
				else {
					int danger = count_adjacent_bombs(game, x, y);
					assert(0 <= danger && danger <= 8);
					tile = tilemap->numbers[danger];
				}
			} else if (game->state == STATE_LOST) {
				if (cell & CELL_BOMB) {
					tile = tilemap->bomb;
				}
				else if (cell & CELL_FLAG) {
					tile = tilemap->flag;
				}
				else if (cell & CELL_KNOWN) {
					int danger = count_adjacent_bombs(game, x, y);
					assert(0 <= danger && danger <= 8);
					tile = tilemap->numbers[danger];
				}
			} else {
				UNREACHABLE();
			}
			
			DrawTexturePro(tilemap->texture, tile, rectangle, CLITERAL(Vector2){0, 0}, 0.0f, tint);
		}
	}
}

static void reveal_adjacent_cells(Game *game, int x, int y) {
	for (int row = y - 1; row <= y + 1; ++row) {
		if (0 > row || row >= game->rows) {
			continue;
		}
		
		for (int column = x - 1; column <= x + 1; ++column) {
			if (row == y && column == x) {
				continue;
			}
			if (0 > column || column >= game->columns) {
				continue;
			}
			
			char *cell = &game->cells[row * game->columns + column];
			if (*cell & (CELL_KNOWN | CELL_BOMB)) {
				continue;
			}
			
			*cell |= CELL_KNOWN;
			++game->revealed;
			
			if (count_adjacent_bombs(game, column, row) == 0) {
				reveal_adjacent_cells(game, column, row);
			}
		}
	}
}

static void handle_input(Game *game) {
	if (IsKeyReleased(KEY_SPACE)) {
		game->state = STATE_RESTART;
	}
	if (game->state != STATE_PLAY) {
		return;
	}

	int x = GetMouseX() / game->cell_size;
	int y = GetMouseY() / game->cell_size;

	if (IsMouseButtonReleased(0)) {
		char *cell = &game->cells[y * game->columns + x];
		if (*cell & CELL_FLAG) {
			NOOP();
		}
		else if (*cell & CELL_BOMB) {
			game->state = STATE_LOST;
			game->finish_time = GetTime();
		}
		else if (!(*cell & CELL_KNOWN)) {
			if (game->revealed == 0) {
				plant_bombs(game, x, y);
			}
			
			*cell |= CELL_KNOWN;
			++game->revealed;
			
			if (count_adjacent_bombs(game, x, y) == 0) {
				reveal_adjacent_cells(game, x, y);
			}
		}
	} else if (IsMouseButtonReleased(1)) {
		char *cell = &game->cells[y * game->columns + x];
		if (*cell & CELL_KNOWN) {
			NOOP();
		}
		else if (*cell & CELL_FLAG) {
			*cell &= ~CELL_FLAG;
			--game->flags;
		}
		else {
			*cell |= CELL_FLAG;
			++game->flags;
		}
	}
}

int main(void) {
	int cell_size = 100;
	int rows = 12;
	int columns = 16;
	int window_width = columns * cell_size;
	int window_height = rows * cell_size;
	float bomb_density = 0.2f;
	
	const char *window_title = "Minesweepa";
	InitWindow(window_width, window_height, window_title);

	Tilemap *tilemap = load_tilemap();
	Game *game = create_game(rows, columns, bomb_density, cell_size);
	
	while (!WindowShouldClose()) {
		BeginDrawing();
			ClearBackground(GRAY);
			handle_input(game);

			if (game->state == STATE_RESTART) {
				destroy_game(game);
				game = create_game(rows, columns, bomb_density, cell_size);
			}

			draw_field(game, tilemap);
			draw_ui(game, tilemap);
		EndDrawing();
	}

	destroy_game(game);
	CloseWindow();
	return 0;
}
