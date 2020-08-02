#include "serialize.h"

void _gameSerialize(Game* game, FILE* file) {
   //   GameWindow* sdl = nullptr;
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

int _gameDeserialize(Game* game, FILE* file) {
   //   GameWindow* sdl = nullptr;
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
   fwrite(&board->wBuffer;, sizeof(int), 1, file);
   fwrite(&board->w, sizeof(int), 1, file);
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
   //   GarbagePile* pile = nullptr;
   fwrite(&board->seed, sizeof(uint64_t), 1, file);
}

int _boardDeserialize(Board* board, FILE* file) {
   fread(&board->startH, sizeof(int), 1, file);
   fread(&board->wBuffer;, sizeof(int), 1, file);
   fread(&board->w, sizeof(int), 1, file);
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
   //   GarbagePile* pile = nullptr;
   fread(&board->seed, sizeof(uint64_t), 1, file);
}

