#include "serialize.h"

//This resizes a vector and shoves bytes on the end
template <typename T>
void writeStream(std::vector <Byte> &stream, const T &input) {
   int oldSize = stream.size();
   int newSize = oldSize + sizeof(T);
   stream.resize(newSize);
   auto writeLocation = stream.data() + oldSize;
   memcpy(writeLocation, &input, sizeof(T));
}

//Take a vector of things and write the number and then each item
template <typename T>
void writeStream(std::vector <Byte> &stream, std::vector <T> const &input) {
   writeStream(stream, input.size());
   for (auto&& item : input) { writeStream(item); }
}

//This takes a unsigned char* and reads out a chunk, it then increments the stream pointer
template <typename T>
void readStream(Byte* &stream, T& output) {
   memcpy(&output, stream, sizeof(T));
   stream += sizeof(T);
}

//Same as readStream, but for a vector
template <typename T>
void readStream(Byte* &stream, std::vector <T> &output) {
   size_t count;  //size_t can store the size of any object
   readStream(stream, count);
   for (size_t i = 0; i < count; i++){
      T instance{};
      readStream(stream, instance);
      output.push_back(std::move(instance) );  //std::move avoids copying
   }
}

void _gameSerialize(std::vector <Byte> &stream, Game* game) {
   //   GameWindow* sdl = nullptr;
   //writeStream(stream, game->windowWidth);
   //writeStream(stream, game->windowHeight);
   //   Vector<Board*>* boards = nullptr;
   //   Resources* resources = nullptr;
   writeStream(stream, game->bHeight);
   writeStream(stream, game->bWidth);
   writeStream(stream, game->tWidth);
   writeStream(stream, game->tHeight);
   writeStream(stream, game->isRunning);
   writeStream(stream, game->players);
   writeStream(stream, game->playing);
   writeStream(stream, game->paused);
   writeStream(stream, game->pauseTimer);
   writeStream(stream, game->pauseLength);
   //if (game->players == 1) {
   //   writeStream(stream, game->timer);   //Don't serialize for 2p! We are sending as input...
   //}
   writeStream(stream, game->seed);
}

void _gameDeserialize(Byte* &start, Game* game) {
   //   GameWindow* sdl = nullptr;
   //readStream(start, game->windowWidth);
   //readStream(start, game->windowHeight);
   //   Vector<Board*>* boards = nullptr;
   //   Resources* resources = nullptr;
   readStream(start, game->bHeight);
   readStream(start, game->bWidth);
   readStream(start, game->tWidth);
   readStream(start, game->tHeight);
   readStream(start, game->isRunning);
   readStream(start, game->players);
   readStream(start, game->playing);
   readStream(start, game->paused);
   readStream(start, game->pauseTimer);
   readStream(start, game->pauseLength);
   //if (game->players == 1) {
   //   readStream(start, game->timer);
   //}
   readStream(start, game->seed);
}

void _boardSerialize(std::vector <Byte> &stream, Board* board) {
   writeStream(stream, board->startH);
   writeStream(stream, board->endH);
   writeStream(stream, board->wBuffer);
   writeStream(stream, board->w);
   writeStream(stream, board->tileWidth);
   writeStream(stream, board->tileHeight);
   writeStream(stream, board->offset);
   //   Mesh* frame = nullptr;
   //   Tile* tiles = nullptr;
   //   Cursor* cursor = nullptr;
   //   Game* game = nullptr;
   writeStream(stream, board->level);
   writeStream(stream, board->fallSpeed);
   writeStream(stream, board->moveSpeed);
   writeStream(stream, board->paused);
   writeStream(stream, board->pauseLength);
   writeStream(stream, board->score);
   writeStream(stream, board->bust);
   writeStream(stream, board->chain);
   writeStream(stream, board->player);
   //   GarbagePile* pile = nullptr;
   writeStream(stream, board->seed);
   writeStream(stream, board->randomCalls);
}

void _boardDeserialize(Byte* &start, Board* board) {
   readStream(start, board->startH);
   readStream(start, board->endH);
   readStream(start, board->wBuffer);
   readStream(start, board->w);
   readStream(start, board->tileWidth);
   readStream(start, board->tileHeight);
   readStream(start, board->offset);
   //   Mesh* frame = nullptr;
   //   Tile* tiles = nullptr;
   //   Cursor* cursor = nullptr;
   //   Game* game = nullptr;
   readStream(start, board->level);
   readStream(start, board->fallSpeed);
   readStream(start, board->moveSpeed);
   readStream(start, board->paused);
   readStream(start, board->pauseLength);
   readStream(start, board->score);
   readStream(start, board->bust);
   readStream(start, board->chain);
   readStream(start, board->player);
   //   GarbagePile* pile = nullptr;
   readStream(start, board->seed);
   readStream(start, board->randomCalls);
}

//Below are special deserializers for Tile enums
void _serializeTileType(std::vector <Byte> &stream, Tile* tile) {
   int type = 0;
   if (tile->type) {
      type = (int)tile->type;
   }
   writeStream(stream, type);
}

void _deserializeTileType(Byte* &start, Tile* tile) {
   int type;
   readStream(start, type);
   if (type >= 0 && type < tile_COUNT) {
      tile->type = (TileType)type;
   }
   else { tile->type = tile_empty; }
}

void _serializeTileStatus(std::vector <Byte> &stream, Tile* tile) {
   int status = 0;
   if (tile->status) {
      status = (int)tile->status;
   }
   writeStream(stream, status);
}

void _deserializeTileStatus(Byte* &start, Tile* tile) {
   int status;
   readStream(start, status);
   if (status >= 0 && status < status_COUNT) {
      tile->status = (TileStatus)status;
   }
   else { tile->status = status_normal; }
}

//Special serializers for Garbage
void _serializeTileGarbage(std::vector <Byte> &stream, Tile* tile) {
   bool garbageStart = false;
   if (tile->garbage != nullptr) {
      garbageStart = true;
   }
   writeStream(stream, garbageStart);
}

void _deserializeTileGarbage(Byte* &start, Board* board, Tile* tile) {
   bool garbageStart = false;
   readStream(start, garbageStart);
   if (garbageStart == true) {
      garbageSetStart(board->pile, tile);
      tile->garbage = garbageGet(board->pile, tile->idGarbage);
   }
}


void _tileSerialize(std::vector <Byte> &stream, Tile* tile) {
   _serializeTileType(stream, tile);
   _serializeTileStatus(stream, tile);
   //writeStream(stream, tile->type);
   writeStream(stream, tile->xpos);
   writeStream(stream, tile->ypos);
   //writeStream(stream, tile->status);
   //   Mesh* mesh;
   writeStream(stream, tile->falling);
   writeStream(stream, tile->clearTime);
   writeStream(stream, tile->statusTime);
   writeStream(stream, tile->chain);
   //   Garbage* garbage;
   writeStream(stream, tile->idGarbage);
   _serializeTileGarbage(stream, tile);
}

void _tileDeserialize(Byte* &start, Board* board, Tile* tile) {
   _deserializeTileType(start, tile);
   _deserializeTileStatus(start, tile);
   //readStream(start, tile->type);
   readStream(start, tile->xpos);
   readStream(start, tile->ypos);
   //readStream(start, tile->status);
   //   Mesh* mesh;
   readStream(start, tile->falling);
   readStream(start, tile->clearTime);
   readStream(start, tile->statusTime);
   readStream(start, tile->chain);
   //   Garbage* garbage;
   readStream(start, tile->idGarbage);
   _deserializeTileGarbage(start, board, tile);
}

void _cursorSerialize(std::vector <Byte> &stream, Cursor* cursor) {
   writeStream(stream, cursor->x);
   writeStream(stream, cursor->y);
   //   Mesh* mesh;
   //   Animation* animation;
   writeStream(stream, cursor->h);
   writeStream(stream, cursor->w);
}

void _cursorDeserialize(Byte* &start, Cursor* cursor) {
   readStream(start, cursor->x);
   readStream(start, cursor->y);
   //   Mesh* mesh;
   //   Animation* animation;
   readStream(start, cursor->h);
   readStream(start, cursor->w);
}


void _garbageSerialize(std::vector <Byte> &stream, Board* board) {

   writeStream(stream, board->pile->nextID);
   int count = board->pile->garbage.size();
   writeStream(stream, count);

   for (auto&& pair : board->pile->garbage) {  //iterating a map gives std::pair (use first and second)
      Garbage* garbage = pair.second;

      writeStream(stream, garbage->ID);
      writeStream(stream, garbage->width);
      writeStream(stream, garbage->layers);
      //   Tile* start;  
      //   Mesh* mesh;
      writeStream(stream, garbage->deployed);
      writeStream(stream, garbage->deployTime);
      writeStream(stream, garbage->falling);
      writeStream(stream, garbage->metal);
   }
}

void _garbageDeserialize(Byte* &start, Board* board) {

   readStream(start, board->pile->nextID);

   int count = 0;
   readStream(start, count);

   for (int i = 0; i < count; i++) {  //iterating a map gives std::pair (use first and second)
      Garbage* garbage = garbageCreateEmpty(board);

      readStream(start, garbage->ID);
      readStream(start, garbage->width);
      readStream(start, garbage->layers);
      //   Tile* start;  
      //   Mesh* mesh;
      readStream(start, garbage->deployed);
      readStream(start, garbage->deployTime);
      readStream(start, garbage->falling);
      readStream(start, garbage->metal);

      board->pile->garbage[garbage->ID] = garbage;
   }
}