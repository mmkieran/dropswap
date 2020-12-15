#include "game.h"
#include "netplay.h"

typedef struct Board Board;
typedef struct Tile Tile;
typedef struct Garbage Garbage;
typedef struct Cursor Cursor;

void _gameSerialize(std::vector <Byte> &stream, Game* game);
void _gameDeserialize(Byte* &start, Game* game);
void _boardSerialize(std::vector <Byte> &stream, Board* board);
void _boardDeserialize(Byte* &start, Board* board);
void _tileSerialize(std::vector <Byte> &stream, Tile* tile);
void _tileDeserialize(Byte* &start, Board* board, Tile* tile);
void _cursorSerialize(std::vector <Byte> &stream, Board* board);
void _cursorDeserialize(Byte* &start, Board* board);
void _garbageSerialize(std::vector <Byte> &stream, Board* board);
void _garbageDeserialize(Byte* &start, Board* board);

int gameLoad(Game* game, unsigned char*& start);
int gameCallbackLoad(Game* game, unsigned char*& start);
std::vector <Byte> gameSave(Game* game);

int gameLoadState(Game* game, const char* path);
FILE* gameSaveState(Game* game, const char* filename);

void serializeMultiSetup(Game* game, std::vector <Byte>& stream);
void deserializeMultiSetup(Game* game, Byte*& start);

std::vector <Byte> createReplay(Game* game);
void loadReplay(Game* game, std::vector <Byte> stream);

FILE* streamSaveToFile(const char* path, std::vector <Byte>& stream);
std::vector <Byte> streamLoadFromFile(const char* path);