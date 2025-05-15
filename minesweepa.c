#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <raylib.h>

#define UNREACHABLE() assert(false)

typedef enum Cell {
	CELL_NOTHING = 0,
	CELL_KNOWN = 1 << 0,
	CELL_BOMB = 1 << 1,
	CELL_FLAG = 1 << 2,
} Cell;

typedef struct Field {
	int rows;
	int columns;
	int bombs;
	char cells[];
} Field;

typedef enum State {
	STATE_PLAY = 0,
	STATE_WON,
	STATE_LOST,
} State;

typedef struct Game {
	int flags;
	State state;
	float bomb_density;
	Field *field;
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

static Field *create_field(int rows, int columns, float bomb_density) {
	Field *field = MemAlloc((sizeof *field) + (sizeof *field->cells) * rows * columns);
	assert(field != NULL);

	field->rows = rows;
	field->columns = columns;
	field->bombs = rows * columns * bomb_density;

	SetRandomSeed(time(NULL));
	for (int planted = 0; planted < field->bombs; ) {
		int x = GetRandomValue(0, columns - 1);
		int y = GetRandomValue(0, rows - 1);
		char *cell = &field->cells[y * columns + x];
		if (!(*cell & CELL_BOMB)) {
			*cell |= CELL_BOMB;
			++planted;
		}
	}

	return field;
}

static void destroy_field(Field *field) {
	MemFree(field);
}

static int count_danger(const Field *field, int x, int y) {
	int danger = 0;
	for (int row = y - 1; row <= y + 1; ++row) {
		if (0 > row || row >= field->rows) {
			continue;
		}
		for (int column = x - 1; column <= x + 1; ++column) {
			if (row == y && column == x) {
				continue;
			}
			if (0 > column || column >= field->columns) {
				continue;
			}
			if (field->cells[row * field->columns + column]) {
				++danger;
			}
		}
	}
	return danger;
}

static void draw_field(const Game *game, const Tilemap *tilemap, int cell_size) {
	const Field *field = game->field;

	Rectangle rectangle;
	rectangle.width = cell_size;
	rectangle.height = cell_size;

	int mouse_x = GetMouseX();
	int mouse_y = GetMouseY();
	
	for (int y = 0; y < field->rows; ++y) {
		rectangle.y = y * cell_size;

		for (int x = 0; x < field->columns; ++x) {
			rectangle.x = x * cell_size;

			Rectangle tile = tilemap->blank;
			Color tint = WHITE;
			char cell = field->cells[y * field->columns + x];

			if (game->state == STATE_PLAY) {
				if (
					(x * cell_size < mouse_x && mouse_x < (x + 1) * cell_size) &&
				    (y * cell_size < mouse_y && mouse_y < (y + 1) * cell_size)
				) {
					tint = CLITERAL(Color){230, 230, 230, 255};
				}
				
				if (cell & CELL_KNOWN) {
					int danger = count_danger(field, x, y);
					assert(0 <= danger && danger <= 8);
					tile = tilemap->numbers[danger];
				} else if (cell & CELL_FLAG) {
					tile = tilemap->flag;
				}
			} else if (game->state == STATE_WON) {
				if (cell & CELL_FLAG) {
					tile = tilemap->flag;
				} else if (cell & CELL_BOMB) {
					tile = tilemap->bomb;
				} else {
					int danger = count_danger(field, x, y);
					assert(0 <= danger && danger <= 8);
					tile = tilemap->numbers[danger];
				}
			} else if (game->state == STATE_LOST) {
				if (cell & CELL_BOMB) {
					tile = tilemap->bomb;
				} else if (cell & CELL_FLAG) {
					tile = tilemap->flag;
				} else if (cell & CELL_KNOWN) {
					int danger = count_danger(field, x, y);
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

int main(void) {
	int cell_size = 100;
	int rows = 12;
	int columns = 16;
	int window_width = columns * cell_size;
	int window_height = rows * cell_size;
	const char *window_title = "Minesweepa";
	InitWindow(window_width, window_height, window_title);

	Tilemap *tilemap = load_tilemap();

	Game game = {0};
	game.flags = 0;
	game.state = STATE_PLAY;
	game.bomb_density = 0.25f;
	game.field = create_field(rows, columns, game.bomb_density);

	while (!WindowShouldClose()) {
		BeginDrawing();
			ClearBackground(GRAY);
			draw_field(&game, tilemap, cell_size);
		EndDrawing();
	}

	destroy_field(game.field);
	CloseWindow();
	return 0;
}
