#include <assert.h>
#include <stdio.h>
#include <time.h>

#include <raylib.h>

#define COLOR_ODD   DARKGRAY
#define COLOR_EVEN  GRAY
#define COLOR_BOMB  RED
#define COLOR_FLAG  YELLOW
#define COLOR_KNOWN GREEN

typedef enum Cell {
	CELL_DEFAULT = 0,
	CELL_BOMB    = 1 << 0,
	CELL_FLAG    = 1 << 1,
	CELL_KNOWN   = 1 << 2
} Cell;

typedef struct Field {
	int  rows;
	int  columns;
	int  bombs;
	char cells[];
} Field;

static void BlendColorsInto(Color *dest, const Color *source, float amount)
{
	dest->r = (1.0f - amount)*dest->r + amount*source->r;
	dest->g = (1.0f - amount)*dest->g + amount*source->g;
	dest->b = (1.0f - amount)*dest->b + amount*source->b;
	dest->a = (1.0f - amount)*dest->a + amount*source->a;
}

static Field *CreateField(int rows, int columns, int bombs)
{
	if (bombs > rows*columns) return NULL;
	
	Field *field = MemAlloc((sizeof *field) + rows*columns*(sizeof *field->cells));
	assert(field != NULL);

	field->rows    = rows;
	field->columns = columns;
	field->bombs   = 0;

	SetRandomSeed(time(NULL));
	while (field->bombs != bombs) {
		int x = GetRandomValue(0, columns - 1);
		int y = GetRandomValue(0, rows - 1);
		char *cell = &field->cells[y*columns + x];
		if (!(*cell&CELL_BOMB)) {
			*cell |= CELL_BOMB;
			++field->bombs;
		}
	}

	return field;
}

static void DrawCheckerBoard(const Field *field, int cellSize)
{
	Rectangle rect;
	rect.width  = cellSize;
	rect.height = cellSize;
	for (int y = 0; y < field->rows; ++y) {
		rect.y = y*cellSize;
		for (int x = 0; x < field->columns; ++x) {
			rect.x = x*cellSize;

			Color color = ((x + y)%2 == 0) ? COLOR_EVEN : COLOR_ODD;

			char cell = field->cells[y * field->columns + x];
			if      (cell&CELL_BOMB)  BlendColorsInto(&color, &COLOR_BOMB,  0.25f);
			else if (cell&CELL_FLAG)  BlendColorsInto(&color, &COLOR_FLAG,  0.25f);
			else if (cell&CELL_KNOWN) BlendColorsInto(&color, &COLOR_KNOWN, 0.25f);			
			
			DrawRectangleRec(rect, color);
		}
	}
}

int main(void)
{
	Field *field     = CreateField(10, 8, 20);
	int cellSize     = 100;
	int windowWidth  = field->columns*cellSize;
	int windowHeight = field->rows*cellSize;
	InitWindow(windowWidth, windowHeight, "Minesweeper");
	
	while (!WindowShouldClose()) {
		BeginDrawing();
			ClearBackground(GRAY);
			DrawCheckerBoard(field, cellSize);
		EndDrawing();
	}

	MemFree(field);	
	CloseWindow();
}
