#include "game.h"
#include "board.h"
#include "tile.h"
#include "cursor.h"
#include "garbage.h"

void _gameSerialize(Game* game, FILE* file);
void _gameDeserialize(Game* game, FILE* file);
void _boardSerialize(Board* board, FILE* file);
void _boardDeserialize(Board* board, FILE* file);
void _tileSerialize(Tile* tile, FILE* file);
void _tileDeserialize(Tile* tile, FILE* file);
void _cursorSerialize(Cursor* cursor, FILE* file);
void _cursorDeserialize(Cursor* cursor, FILE* file);
