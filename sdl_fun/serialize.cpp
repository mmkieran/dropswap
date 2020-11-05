#include "serialize.h"

#include "board.h"

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
   writeStream(stream, game->settings.bHeight);
   writeStream(stream, game->settings.bWidth);
   writeStream(stream, game->settings.tWidth);
   writeStream(stream, game->settings.tHeight);
   writeStream(stream, game->isRunning);
   writeStream(stream, game->players);
   writeStream(stream, game->playing);
   writeStream(stream, game->paused);
   writeStream(stream, game->frameCount);
   writeStream(stream, game->timer);
   writeStream(stream, game->seed);
}

void _gameDeserialize(Byte* &start, Game* game) {
   //   GameWindow* sdl = nullptr;
   //readStream(start, game->windowWidth);
   //readStream(start, game->windowHeight);
   //   Vector<Board*>* boards = nullptr;
   //   Resources* resources = nullptr;
   readStream(start, game->settings.bHeight);
   readStream(start, game->settings.bWidth);
   readStream(start, game->settings.tWidth);
   readStream(start, game->settings.tHeight);
   readStream(start, game->isRunning);
   readStream(start, game->players);
   readStream(start, game->playing);
   readStream(start, game->paused);
   readStream(start, game->frameCount);
   readStream(start, game->timer);
   readStream(start, game->seed);
}

void _boardVisualSerialize(std::vector <Byte>& stream, Board* board) {
   int count = board->visualEvents.size();
   writeStream(stream, count);

   for (auto&& pair : board->visualEvents) {
      int type = (int)pair.first;
      writeStream(stream, type);
      writeStream(stream, pair.second.active);
      writeStream(stream, pair.second.end);
      writeStream(stream, pair.second.pos);
   }
}

void _boardVisualDeserialize(Byte*& start, Board* board) {
   int count = 0;
   readStream(start, count);
   for (int i = 0; i < count; i++) {
      int type;
      readStream(start, type);
      VisualEffect effect = (VisualEffect)type;
      readStream(start, board->visualEvents[effect].active);
      readStream(start, board->visualEvents[effect].end);
      readStream(start, board->visualEvents[effect].pos);
   }
}

void _boardStatsSerialize(std::vector <Byte>& stream, Board* board) {

   writeStream(stream, board->boardStats.apm);
   writeStream(stream, board->boardStats.lastChain);
   writeStream(stream, board->boardStats.lastCombo);
   writeStream(stream, board->boardStats.clears);
   writeStream(stream, board->boardStats.dangeresque);
   writeStream(stream, board->boardStats.garbageCrushed);

   int count = board->boardStats.chainCounts.size();
   writeStream(stream, count);
   if (count > 0) {
      for (auto&& pair : board->boardStats.chainCounts) {
         writeStream(stream, pair.first);
         writeStream(stream, pair.second);
      }
   }

   int count2 = board->boardStats.comboCounts.size();
   writeStream(stream, count2);
   if (count2 > 0) {
      for (auto&& pair : board->boardStats.comboCounts) {
         writeStream(stream, pair.first);
         writeStream(stream, pair.second);
      }
   }
}

void _boardStatsDeserialize(Byte*& start, Board* board) {
   board->boardStats.chainCounts.clear();
   board->boardStats.comboCounts.clear();

   readStream(start, board->boardStats.apm);
   readStream(start, board->boardStats.lastChain);
   readStream(start, board->boardStats.lastCombo);
   readStream(start, board->boardStats.clears);
   readStream(start, board->boardStats.dangeresque);
   readStream(start, board->boardStats.garbageCrushed);

   int count = 0;
   readStream(start, count);
   for (int i = 0; i < count; i++) {
      int chain, number;
      readStream(start, chain);
      readStream(start, number);
      board->boardStats.chainCounts[chain] = number;
   }

   int count2 = 0;
   readStream(start, count2);
   for (int i = 0; i < count2; i++) {
      int combo, number;
      readStream(start, combo);
      readStream(start, number);
      board->boardStats.comboCounts[combo] = number;
   }
}

void _boardSerialize(std::vector <Byte> &stream, Board* board) {
   writeStream(stream, board->startH);
   writeStream(stream, board->endH);
   writeStream(stream, board->wBuffer);
   writeStream(stream, board->w);
   writeStream(stream, board->h);
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
   writeStream(stream, board->waitForClear);
   writeStream(stream, board->pauseLength);
   writeStream(stream, board->score);
   writeStream(stream, board->danger);
   writeStream(stream, board->bust);
   writeStream(stream, board->chain);
   writeStream(stream, board->team);
   //   GarbagePile* pile = nullptr;
   writeStream(stream, board->seed);
   writeStream(stream, board->randomCalls);
   _boardStatsSerialize(stream, board);
   _boardVisualSerialize(stream, board);
}

void _boardDeserialize(Byte* &start, Board* board) {
   readStream(start, board->startH);
   readStream(start, board->endH);
   readStream(start, board->wBuffer);
   readStream(start, board->w);
   readStream(start, board->h);
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
   readStream(start, board->waitForClear);
   readStream(start, board->pauseLength);
   readStream(start, board->score);
   readStream(start, board->danger);
   readStream(start, board->bust);
   readStream(start, board->chain);
   readStream(start, board->team);
   //   GarbagePile* pile = nullptr;
   readStream(start, board->seed);
   readStream(start, board->randomCalls);
   _boardStatsDeserialize(start, board);
   _boardVisualDeserialize(start, board);
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

void _serializeVisualEffect(std::vector <Byte>& stream, Tile* tile) {
   int effect = 0;
   if (tile->effect) {
      effect = (int)tile->effect;
   }
   writeStream(stream, effect);
}

void _deserializeVisualEffect(Byte*& start, Tile* tile) {
   int effect;
   readStream(start, effect);
   if (effect >= 0 && effect < visual_COUNT) {
      tile->effect = (VisualEffect)effect;
   }
   else { tile->effect = visual_none; }
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
   _serializeVisualEffect(stream, tile);
   writeStream(stream, tile->effectTime);
   writeStream(stream, tile->xpos);
   writeStream(stream, tile->ypos);
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
   _deserializeVisualEffect(start, tile);
   readStream(start, tile->effectTime);
   readStream(start, tile->xpos);
   readStream(start, tile->ypos);
   //   Mesh* mesh;
   readStream(start, tile->falling);
   readStream(start, tile->clearTime);
   readStream(start, tile->statusTime);
   readStream(start, tile->chain);
   //   Garbage* garbage;
   readStream(start, tile->idGarbage);
   _deserializeTileGarbage(start, board, tile);
}

void _cursorSerialize(std::vector <Byte> &stream, Board* board) {
   int cursorNumber = board->cursors.size();
   writeStream(stream, cursorNumber);
   for (int i = 0; i < cursorNumber; i++) {
      Cursor* cursor = board->cursors[i];
      writeStream(stream, cursor->x);
      writeStream(stream, cursor->y);
      //   Mesh* mesh;
      //   Animation* animation;
      writeStream(stream, cursor->h);
      writeStream(stream, cursor->w);
   }
}

void _cursorDeserialize(Byte* &start, Board* board) {
   int cursorNumber = 0;
   readStream(start, cursorNumber );
   for (int i = 0; i < cursorNumber; i++) {
      Cursor* cursor = board->cursors[i];
      readStream(start, cursor->x);
      readStream(start, cursor->y);
      //   Mesh* mesh;
      //   Animation* animation;
      readStream(start, cursor->h);
      readStream(start, cursor->w);
   }
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
      writeStream(stream, garbage->totalFall);
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
      readStream(start, garbage->totalFall);
      readStream(start, garbage->metal);

      board->pile->garbage[garbage->ID] = garbage;
   }
}


std::vector <Byte> gameSave(Game* game) {
   std::vector <Byte> stream;

   //serialize game settings
   _gameSerialize(stream, game);

   for (auto&& board : game->boards) {
      if (board) {
         //serialize board settings
         _boardSerialize(stream, board);

         //serialize garbage
         _garbageSerialize(stream, board);

         //serialize tiles
         for (int row = 0; row < board->wBuffer; row++) {
            for (int col = 0; col < board->w; col++) {
               Tile* tile = boardGetTile(board, row, col);
               if (tile) {
                  _tileSerialize(stream, tile);
               }
            }
         }
         //serialize cursor
         _cursorSerialize(stream, board);
      }
   }
   return stream;
}

int gameCallbackLoad(Game* game, unsigned char*& start) {

   //deserialize game
   _gameDeserialize(start, game);

   for (auto&& board : game->boards) {
      //deserialize board
      if (board) {
         _boardDeserialize(start, board);
         boardLoadRandom(board);  //Return random generator to saved state using discard

         garbagePileEmpty(board->pile);  //Clear the garbage
         //deserialize garbage
         _garbageDeserialize(start, board);

         for (int row = 0; row < board->wBuffer; row++) {
            for (int col = 0; col < board->w; col++) {
               Tile* tile = boardGetTile(board, row, col);
               //deserialize tiles
               tile->garbage = nullptr;
               _tileDeserialize(start, board, tile);
               tileSetTexture(board, tile);
            }
         }

         //deserialize cursor
         _cursorDeserialize(start, board);
      }
   }
   return 0;
}


int gameLoad(Game* game, unsigned char*& start) {

   //destroy the boards
   for (auto&& board : game->boards) {
      if (board) { boardDestroy(board); }
   }
   game->boards.clear();

   //deserialize game
   _gameDeserialize(start, game);

   int players = game->players;
   if (players > 2) { players = 2; }
   for (int i = 0; i < players; i++) {
      Board* board = nullptr;
      if (game->playing) {
         board = boardCreate(game, i + 1, 64 , 64);
         //deserialize board
         if (board) {
            _boardDeserialize(start, board);
            boardLoadRandom(board);  //Return random generator to saved state using discard

            //deserialize garbage
            _garbageDeserialize(start, board);

            for (int row = 0; row < board->wBuffer; row++) {
               for (int col = 0; col < board->w; col++) {
                  Tile* tile = boardGetTile(board, row, col);
                  //deserialize tiles
                  tile->garbage = nullptr;
                  _tileDeserialize(start, board, tile);
                  tileSetTexture(board, tile);
               }
            }

            //deserialize cursor
            _cursorDeserialize(start, board);

            game->boards.push_back(board);
         }
      }

   }

   return 0;
}

FILE* gameSaveState(Game* game, const char* filename) {
   FILE* out;
   int err = fopen_s(&out, filename, "w");
   std::vector <Byte> stream;
   stream = gameSave(game);

   if (err == 0 && stream.size() > 0) {
      int streamSize = stream.size();
      fwrite(&streamSize, sizeof(int), 1, out);

      //todo make this betterer?
      for (int i = 0; i < streamSize; i++) {
         fwrite(&stream[i], sizeof(Byte), 1, out);
      }

   }
   else { printf("Failed to save file... Err: %d\n", err); }
   game->settings.save = stream;  
   fclose(out);
   return out;
}

int gameLoadState(Game* game, const char* path) {
   FILE* in;
   int err = fopen_s(&in, path, "r");
   std::vector <Byte> stream;
   if (err == 0) {
      int streamSize = 0;
      fread(&streamSize, sizeof(int), 1, in);
      stream.resize(streamSize);

      //Byte* start = stream.data();  //Find out how to make this work, lol
      //fread(&start, sizeof(Byte) * streamSize, 1, in);

      //todo read the entire file into memory and memcpy it to the vector
      for (int i = 0; i < streamSize; i++) {
         char c;
         fread(&c, sizeof(Byte), 1, in);
         stream[i] = c;
      }

      unsigned char* start = stream.data();
      gameLoad(game, start);
   }
   else { printf("Failed to load file... Err: %d\n", err); }
   fclose(in);
   return 1;
}

//Serialize hostSetup (port numbers, ips, player number, game info) so we can send it over tcp
void serializeGameSetup(Game* game, std::vector <Byte>& stream) {
   writeStream(stream, game->net->participants);
   int mode = (int)game->settings.mode;
   writeStream(stream, mode);
   for (int i = 0; i < game->net->participants; i++) {
      writeStream(stream, game->net->hostSetup[i].host);
      writeStream(stream, game->net->hostSetup[i].ipAddress);
      writeStream(stream, game->net->hostSetup[i].localPort);
      writeStream(stream, game->net->hostSetup[i].playerType);
      writeStream(stream, game->net->hostSetup[i].name);
      writeStream(stream, game->net->hostSetup[i].pNum);
      writeStream(stream, game->net->hostSetup[i].team);

      writeStream(stream, game->net->frameDelay[0]);
      writeStream(stream, game->net->disconnectTime[0]);

      writeStream(stream, game->timings.gracePeriod[0]);
      writeStream(stream, game->timings.fallDelay[0]);
      writeStream(stream, game->timings.removeClear[0]);
      writeStream(stream, game->timings.enterSilver[0]);
      writeStream(stream, game->timings.countIn[0]);
      writeStream(stream, game->timings.landPause[0]);
      writeStream(stream, game->timings.deployTime[0]);
   }
}

void deserializeGameSetup(Game* game, Byte*& start) {
   readStream(start, game->net->participants);
   int mode = 0;
   readStream(start, mode);
   game->settings.mode = (GameMode)mode;
   for (int i = 0; i < game->net->participants; i++) {
      readStream(start, game->net->hostSetup[i].host);
      readStream(start, game->net->hostSetup[i].ipAddress);
      readStream(start, game->net->hostSetup[i].localPort);
      readStream(start, game->net->hostSetup[i].playerType);
      readStream(start, game->net->hostSetup[i].name);
      readStream(start, game->net->hostSetup[i].pNum);
      readStream(start, game->net->hostSetup[i].team);

      readStream(start, game->net->frameDelay[0]);
      readStream(start, game->net->disconnectTime[0]);

      readStream(start, game->timings.gracePeriod[0]);
      readStream(start, game->timings.fallDelay[0]);
      readStream(start, game->timings.removeClear[0]);
      readStream(start, game->timings.enterSilver[0]);
      readStream(start, game->timings.countIn[0]);
      readStream(start, game->timings.landPause[0]);
      readStream(start, game->timings.deployTime[0]);
   }
}