#include "game.h"
#include "board.h"
#include "tile.h"
#include "cursor.h"
#include "garbage.h"

typedef unsigned char Byte;


//void testReadStream();

void _gameSerialize(std::vector <Byte> &stream, Game* game);
void _gameDeserialize(Byte* start, Game* game);
void _boardSerialize(std::vector <Byte> &stream, Board* board);
void _boardDeserialize(Board* board, Byte* start);
void _tileSerialize(std::vector <Byte> &stream, Tile* tile);
void _tileDeserialize(Byte* start, Board* board, Tile* tile);
void _cursorSerialize(std::vector <Byte> &stream, Cursor* cursor);
void _cursorDeserialize(Byte* start, Cursor* cursor);
void _garbageSerialize(std::vector <Byte> &stream, Board* board);
void _garbageDeserialize(Byte* start, Board* board);
