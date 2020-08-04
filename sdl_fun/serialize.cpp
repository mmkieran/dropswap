#include "serialize.h"

void _gameSerialize(Game* game, FILE* file) {
   //   GameWindow* sdl = nullptr;
   fwrite(&game->windowWidth, sizeof(float), 1, file);
   fwrite(&game->windowHeight, sizeof(float), 1, file);
   //   Vector<Board*>* boards = nullptr;
   //   Resources* resources = nullptr;
   fwrite(&game->bHeight, sizeof(int), 1, file);
   fwrite(&game->bWidth, sizeof(int), 1, file);
   fwrite(&game->tWidth, sizeof(int), 1, file);
   fwrite(&game->tHeight, sizeof(int), 1, file);
   fwrite(&game->isRunning, sizeof(bool), 1, file);
   fwrite(&game->players, sizeof(int), 1, file);
   fwrite(&game->playing, sizeof(bool), 1, file);
   fwrite(&game->paused, sizeof(bool), 1, file);
   fwrite(&game->pauseTimer, sizeof(int), 1, file);
   fwrite(&game->pauseLength, sizeof(int), 1, file);
   fwrite(&game->timer, sizeof(int), 1, file);
   fwrite(&game->timeDelta, sizeof(int), 1, file);
   fwrite(&game->seed, sizeof(uint64_t), 1, file);
}

void _gameDeserialize(Game* game, FILE* file) {
   //   GameWindow* sdl = nullptr;
   fread(&game->windowWidth, sizeof(float), 1, file);
   fread(&game->windowHeight, sizeof(float), 1, file);
   //   Vector<Board*>* boards = nullptr;
   //   Resources* resources = nullptr;
   fread(&game->bHeight, sizeof(int), 1, file);
   fread(&game->bWidth, sizeof(int), 1, file);
   fread(&game->tWidth, sizeof(int), 1, file);
   fread(&game->tHeight, sizeof(int), 1, file);
   fread(&game->isRunning, sizeof(bool), 1, file);
   fread(&game->players, sizeof(int), 1, file);
   fread(&game->playing, sizeof(bool), 1, file);
   fread(&game->paused, sizeof(bool), 1, file);
   fread(&game->pauseTimer, sizeof(int), 1, file);
   fread(&game->pauseLength, sizeof(int), 1, file);
   fread(&game->timer, sizeof(int), 1, file);
   fread(&game->timeDelta, sizeof(int), 1, file);
   fread(&game->seed, sizeof(uint64_t), 1, file);
}

void _boardSerialize(Board* board, FILE* file) {
   fwrite(&board->startH, sizeof(int), 1, file);
   fwrite(&board->endH, sizeof(int), 1, file);
   fwrite(&board->wBuffer, sizeof(int), 1, file);
   fwrite(&board->w, sizeof(int), 1, file);
   fwrite(&board->tileWidth, sizeof(int), 1, file);
   fwrite(&board->tileHeight, sizeof(int), 1, file);
   fwrite(&board->offset, sizeof(float), 1, file);
   fwrite(&board->origin, sizeof(Vec2), 1, file);
   //   Mesh* frame = nullptr;
   //   Tile* tiles = nullptr;
   //   Cursor* cursor = nullptr;
   //   Game* game = nullptr;
   fwrite(&board->level, sizeof(float), 1, file);
   fwrite(&board->fallSpeed, sizeof(float), 1, file);
   fwrite(&board->moveSpeed, sizeof(float), 1, file);
   fwrite(&board->paused, sizeof(bool), 1, file);
   fwrite(&board->pauseLength, sizeof(int), 1, file);
   fwrite(&board->score, sizeof(double), 1, file);
   fwrite(&board->bust, sizeof(bool), 1, file);
   fwrite(&board->combo, sizeof(int), 1, file);
   fwrite(&board->player, sizeof(int), 1, file);
   //   GarbagePile* pile = nullptr;
   fwrite(&board->seed, sizeof(uint64_t), 1, file);
   fwrite(&board->randomCalls, sizeof(uint64_t), 1, file);
   //_serializeRandom(board);

   //fwrite(&board->generator, sizeof(std::default_random_engine), 1, file);
   //fwrite(&board->distribution, sizeof(std::uniform_int_distribution<int>), 1, file);
}

void _boardDeserialize(Board* board, FILE* file) {
   fread(&board->startH, sizeof(int), 1, file);
   fread(&board->endH, sizeof(int), 1, file);
   fread(&board->wBuffer, sizeof(int), 1, file);
   fread(&board->w, sizeof(int), 1, file);
   fread(&board->tileWidth, sizeof(int), 1, file);
   fread(&board->tileHeight, sizeof(int), 1, file);
   fread(&board->offset, sizeof(float), 1, file);
   fread(&board->origin, sizeof(Vec2), 1, file);
   //   Mesh* frame = nullptr;
   //   Tile* tiles = nullptr;
   //   Cursor* cursor = nullptr;
   //   Game* game = nullptr;
   fread(&board->level, sizeof(float), 1, file);
   fread(&board->fallSpeed, sizeof(float), 1, file);
   fread(&board->moveSpeed, sizeof(float), 1, file);
   fread(&board->paused, sizeof(bool), 1, file);
   fread(&board->pauseLength, sizeof(int), 1, file);
   fread(&board->score, sizeof(double), 1, file);
   fread(&board->bust, sizeof(bool), 1, file);
   fread(&board->combo, sizeof(int), 1, file);
   fread(&board->player, sizeof(int), 1, file);
   //   GarbagePile* pile = nullptr;
   fread(&board->seed, sizeof(uint64_t), 1, file);
   fread(&board->randomCalls, sizeof(uint64_t), 1, file);
   //_deserializeRandom(board);

   //fread(&board->generator, sizeof(std::default_random_engine), 1, file);
   //fread(&board->distribution, sizeof(std::uniform_int_distribution<int>), 1, file);
}

void _tileSerialize(Tile* tile, FILE* file) {
   _serializeTileType(tile, file);
   _serializeTileStatus(tile, file);
   //fwrite(&tile->type, sizeof(TileType), 1, file);
   //fwrite(&tile->status, sizeof(TileStatus), 1, file);
   fwrite(&tile->xpos, sizeof(float), 1, file);
   fwrite(&tile->ypos, sizeof(float), 1, file);
   //   Mesh* mesh;
   fwrite(&tile->falling, sizeof(bool), 1, file);
   fwrite(&tile->clearTime, sizeof(uint64_t), 1, file);
   fwrite(&tile->statusTime, sizeof(uint64_t), 1, file);
   fwrite(&tile->chain, sizeof(bool), 1, file);
   //   Garbage* garbage;
   fwrite(&tile->idGarbage, sizeof(int), 1, file);
}

void _tileDeserialize(Tile* tile, FILE* file) {
   _deserializeTileType(tile, file);
   _deserializeTileStatus(tile, file);
   //fread(&tile->type, sizeof(TileType), 1, file);
   //fread(&tile->status, sizeof(TileStatus), 1, file);
   fread(&tile->xpos, sizeof(float), 1, file);
   fread(&tile->ypos, sizeof(float), 1, file);
   //   Mesh* mesh;
   fread(&tile->falling, sizeof(bool), 1, file);
   fread(&tile->clearTime, sizeof(uint64_t), 1, file);
   fread(&tile->statusTime, sizeof(uint64_t), 1, file);
   fread(&tile->chain, sizeof(bool), 1, file);
   //   Garbage* garbage;
   fread(&tile->idGarbage, sizeof(int), 1, file);
}

void _cursorSerialize(Cursor* cursor, FILE* file) {
   fwrite(&cursor->x, sizeof(float), 1, file);
   fwrite(&cursor->y, sizeof(float), 1, file);
   //   Mesh* mesh;
   //   Animation* animation;
   fwrite(&cursor->h, sizeof(int), 1, file);
   fwrite(&cursor->w, sizeof(int), 1, file);
}

void _cursorDeserialize(Cursor* cursor, FILE* file) {
   fread(&cursor->x, sizeof(float), 1, file);
   fread(&cursor->y, sizeof(float), 1, file);
   //   Mesh* mesh;
   //   Animation* animation;
   fread(&cursor->h, sizeof(int), 1, file);
   fread(&cursor->w, sizeof(int), 1, file);
}

void _garbageSerialize(Garbage* garbage, FILE* file) {
   fwrite(&garbage->ID, sizeof(int), 1, file);
   fwrite(&garbage->width, sizeof(int), 1, file);
   fwrite(&garbage->layers, sizeof(int), 1, file);
   //   Tile* start;  //top left of garbage
   //   Mesh* mesh;
   fwrite(&garbage->falling, sizeof(bool), 1, file);
}

void _garbageDeserialize(Garbage* garbage, FILE* file) {
   fread(&garbage->ID, sizeof(int), 1, file);
   fread(&garbage->width, sizeof(int), 1, file);
   fread(&garbage->layers, sizeof(int), 1, file);
   //   Tile* start;  //top left of garbage
   //   Mesh* mesh;
   fread(&garbage->falling, sizeof(bool), 1, file);
}

