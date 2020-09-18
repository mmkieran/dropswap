#include <assert.h>

#include "board.h"

//These are all in milliseconds
#define GRACEPERIOD 1000     //Bonus pause time when your board reaches the top before you die
#define FALL_DELAY 100       //The pause before a tile falls after swapping
#define REMOVE_CLEARS 2000   //Time it takes to change a cleared tile to empty
#define ENTER_SILVERS 30000  //Time before silvers start appearing
#define START_TIMER 2000     //Time before the board starts moving and you can swap on startup
#define LANDTIME 1000        //Pause board movement when garbage lands

#define LEVEL_UP 150.0f          //Rate of increase for board level based on tiles cleared

//This functions processes the type of pause to figure out the length of the pause
void boardPauseTime(Board* board, BoardPauseType type, int size) {
   int currentPause = board->pauseLength;
   int time = 0;

   switch (type) {
   case pause_combo:
      time = min( (size - 3) * 1000 + REMOVE_CLEARS, 6000);  //max pause of 6s
      if (time > currentPause) { board->pauseLength = time;}
      break;
   case pause_chain:
      time = min((size - 1) * 1000 + REMOVE_CLEARS, 8000);  //Max pause 8s
      if (time > currentPause) { board->pauseLength = time; }
      break;
   case pause_clear:
      if (currentPause < REMOVE_CLEARS) {  //The board should always be paused if things need to be cleared
         board->pauseLength = REMOVE_CLEARS;
      }
      break;
   case pause_crashland:
      if (board->pauseLength == 0) {  //Little grace period when garbage is landing in case it's at the top
         board->pauseLength = LANDTIME;
      }
      break;
   case pause_garbageclear:
      if (currentPause < REMOVE_CLEARS) {
         board->pauseLength = REMOVE_CLEARS;
      }
      break;
   case pause_danger:
      if (currentPause < GRACEPERIOD) {
         board->pauseLength = GRACEPERIOD;
      }
      break;
   }
   board->paused = true;
};

void _checkClear(std::vector <Tile*> tiles, std::vector <Tile*> &matches);
void boardCheckClear(Board* board, std::vector <Tile*> tileList, bool fallCombo);

//Create the tile array for the board
Tile* _boardCreateArray(int width, int height) {
   Tile* tiles = (Tile*)malloc(sizeof(Tile) * (height * 2 + 1) * width);
   return tiles;
}

//Generate a random tile type
int boardRandomTile(Board* board) {
	int out = board->distribution(board->generator);
   if (out == 7) { 
      if (board->game->timer > ENTER_SILVERS) {
         out = board->distribution(board->generator);
         board->randomCalls++;
      }
      else { out = out % 6; }
   }
	board->randomCalls++;
	return out;
}

//Initialize the random number generator
void boardStartRandom(Board* board) {
	std::default_random_engine gen(board->game->seed);
	board->generator = gen;

	std::uniform_int_distribution<int> dist(1, 7);
	board->distribution = dist;
}

//Restores the state of the random number generator based on number of calls
void boardLoadRandom(Board* board) {
	boardStartRandom(board);

	if (board->randomCalls > 0) {
		board->generator.discard(board->randomCalls);
	}
}

//Create a fresh board and return a pointer
Board* boardCreate(Game* game) {
   Board* board = new Board;
   if (board) {
      Tile* tiles = _boardCreateArray(game->bHeight, game->bWidth);
      if (tiles) {
         board->game = game;

         board->tiles = tiles;

         board->startH = game->bHeight;  //start playing area of the board... everything above is for garbage
         board->endH = game->bHeight * 2;  //end playing area of the board
         board->wBuffer = board->endH + 1;  //Extra row upcoming rows
         board->w = game->bWidth;

         board->tileHeight = game->tHeight;
         board->tileWidth = game->tWidth;

         board->pile = garbagePileCreate();

		   boardStartRandom(board);

         //Set the cursor to midway on the board
         float cursorX = (float)(game->bWidth / 2 - 1) * game->tWidth;
         float cursorY = (float)(game->bHeight / 2 + 1) * game->tHeight;
         board->cursor = cursorCreate(board, cursorX, cursorY);

         return board;
      }
   }
   return nullptr;
}

//Free memory for the board and destroy related objects
Board* boardDestroy(Board* board) {
   if (board) {
      for (int row = 0; row < board->wBuffer; row++) {
         for (int col = 0; col < board->w; col++) {
            Tile* tile = boardGetTile(board, row, col);
            tile->mesh = meshDestroy(tile->mesh);
         }
      }
      board->pile = garbagePileDestroy(board->pile);
      board->cursor = cursorDestroy(board->cursor);
      free(board->tiles);
      delete board;
   }
   return nullptr;
}

//Get a tile using the row and col
Tile* boardGetTile(Board* board, int row, int col) {
   if (row < 0 || row > board->wBuffer - 1) {
      return nullptr;
   }
   Tile* tile = &board->tiles[(board->w * row + col)];
   return tile;
}

//Assign the tile into the tile array
void boardSetTile(Board* board, Tile tile, int row, int col) {
   if (row < 0 || row > board->wBuffer - 1) {
      return;
   }
   board->tiles[(board->w * row + col)] = tile;
}

//Update all tiles that are moving, falling, cleared, etc.
void boardUpdate(Board* board, UserInput input) {

   boardRemoveClears(board);

   if (board->pauseLength > 0) {
      board->pauseLength -= 1000/60;  //Static FPS for now to ease syncing

      if (board->pauseLength <= 0) {
         board->paused = false;
         board->pauseLength = 0;
      }
   }
   else {board->paused = false; }

   if (board->game->timer > START_TIMER) {  //2 second count in to start
      if (board->paused == false) {
         boardMoveUp(board, board->moveSpeed/8.0f * (board->tileHeight / 64.0f) * board->level);  //Normalized for tile size of 64
         garbageDeploy(board);
      }
   }

   if (board->danger == true && board->paused == false) {
      board->bust = true;
   }

   boardFall(board, board->fallSpeed * (board->tileHeight / 64.0f) + board->level / 3.0f);  //Normalized for tile size of 64
   garbageFall(board, board->fallSpeed * (board->tileHeight / 64.0f) + board->level / 3.0f);  //Normalized for tile size of 64
   boardAssignSlot(board, false);

   cursorUpdate(board, input);  //This has kinda become player...
}

//Draw all objects on the board
void boardRender(Game* game, Board* board) {

   for (int row = board->startH; row < board->wBuffer; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (meshGetTexture(tile->mesh) == Texture_empty) {  
            continue; 
         }
         if (tile->effect == visual_swapl || tile->effect == visual_swapr) {
            if (tile->effectTime <= board->game->timer) {
               tile->effect = visual_none;
               tile->effectTime = 0;
            }
            tileDraw(board, tile, tile->effect, tile->effectTime);
         }

         //else if (tile->effect == visual_landing) {
         //   if (tile->effectTime <= board->game->timer) {
         //      tile->effect = visual_none;
         //      tile->effectTime = 0;
         //   }
         //   tileDraw(board, tile);
         //   tileDraw(board, tile, tile->effect, tile->effectTime);
         //}
         else { tileDraw(board, tile); }
      }
   }
   cursorDraw(board);
   //Garbage is just drawn as a tile texture right now
   //garbageDraw(board);
}

//Calculates the row based on the pointer difference in the array
int tileGetRow(Board* board, Tile* tile) {
   int out = (tile - board->tiles) / board->w;
   return out;
}

//Calculates the col based on the pointer difference in the array
int tileGetCol(Board* board, Tile* tile) {
   return (tile - board->tiles) % board->w;
}

//Get a full column of tiles on the board
std::vector <Tile*> boardGetAllTilesInCol(Board* board, int col) {
   std::vector <Tile*> tiles;
   for (int i = board->startH -1; i < board->endH; i++) {
      tiles.push_back(boardGetTile(board, i, col));
   }
   return tiles;
}

//Get a full row of tiles on the board
std::vector <Tile*> boardGetAllTilesInRow(Board* board, int row) {
   std::vector <Tile*> tiles;
   //Left to right because it doesn't matter
   for (int i = 0; i < board->w; i++) {
      tiles.push_back(boardGetTile(board, row, i));
   }
   return tiles;
}

//Helper with logic for swap
static void _swapTiles(Tile* tile1, Tile* tile2) {

   Tile tmp = *tile2;

   tile2->type = tile1->type;
   tile2->mesh = tile1->mesh;

   tile1->type = tmp.type;
   tile1->mesh = tmp.mesh;
}

//Swap two tiles on the board horizontally
void boardSwap(Board* board) {

   if (board->game->timer < START_TIMER) { return; } //No swapping during count in

   float xCursor = cursorGetX(board->cursor);
   float yCursor = cursorGetY(board->cursor);

   int col = cursorGetCol(board);
   int row = cursorGetRow(board);

   Tile* tile1 = boardGetTile(board, row, col);
   Tile* tile2 = boardGetTile(board, row, col + 1);
   assert(tile1 && tile2);

   tile1->chain = tile2->chain = false;

   Tile* below1 = boardGetTile(board, row + 1, col);
   Tile* below2 = boardGetTile(board, row + 1, col + 1);

   Tile* above1 = boardGetTile(board, row - 1, col);
   Tile* above2 = boardGetTile(board, row - 1, col + 1);

   if (tile1->type == tile_garbage || tile2->type == tile_garbage) { return; }    //Don't swap garbage
   if (tile1->type == tile_cleared || tile2->type == tile_cleared) { return; }    //Don't swap clears
   if (tile1->status == status_disable || tile2->status == status_disable) { return; }    //Don't swap disabled tiles

   if (tile1->type == tile_empty && tile2->type != tile_empty) {  //Special empty swap cases
      if (tile2->falling == true && tile2->ypos > yCursor + 1) { return; }  //Don't swap non-empty if it's already falling below
      else if (above1 && above1->type != tile_empty && above1->ypos > tile2->ypos - board->tileHeight + 8) { return; }
      else {
         _swapTiles(tile1, tile2);
         tile1->ypos = tile2->ypos;  //When swapping an empty tile, maintain ypos
      }
   }
   else if (tile2->type == tile_empty && tile1->type != tile_empty) {  //Special empty swap cases
      if (tile1->falling == true && tile1->ypos > yCursor + 1) { return; }  //Don't swap non-empty if it's already falling below
      else if (above2 && above2->type != tile_empty && above2->ypos > tile1->ypos - board->tileHeight + 8) { return; }
      else { 
         _swapTiles(tile1, tile2);
         tile2->ypos = tile1->ypos;  //When swapping an empty tile, maintain ypos
      }
   }
   else {
      _swapTiles(tile1, tile2);
   }

   std::vector <Tile*> tiles;
   //Check if after we swapped them, either tile is falling... these don't get cleared
   if (below1 && (below1->type == tile_empty || below1->falling == true)) {
      //tile1->falling = true;
      tile1->status = status_stop;  
      tile1->statusTime = board->game->timer + FALL_DELAY;  //Short pause before falling
   }

   if (below2 && (below2->type == tile_empty || below2->falling == true)) {
      //tile2->falling = true;
      tile2->status = status_stop;
      tile2->statusTime = board->game->timer + FALL_DELAY;  //Short pause before falling
   }

   tiles.push_back(tile1); 
   tiles.push_back(tile2);

   board->game->soundToggles[sound_swap] = true;  //Send a signal to play swap sound

   tile1->effect = visual_swapl;  //Visual interpolation for swapping left
   tile1->effectTime = board->game->timer + SWAPTIME;

   tile2->effect = visual_swapr;  //Visual interpolation for swapping right
   tile2->effectTime = board->game->timer + SWAPTIME;

   boardCheckClear(board, tiles, false);

   return;
}

//Dumps garbage on the other player depending on chain size
void boardChainGarbage(Game* game, int player, int chain) {
   Board* board;
   if (player == 1) { board = game->boards[1]; }
   else if (player == 2) { board = game->boards[0]; }

   if (board) {
      int gWidth = 6;
      int gHeight = 0;
      if (chain - 1 > 12) { gHeight = 12; }
      else { gHeight = chain - 1; }
      if (gHeight > 0) { garbageCreate(board, gWidth, gHeight); }
   }
}

//Calculates the size of the combo garbage to drop
static void _calcComboGarbage(Board* board, int matchSize) {
   if (matchSize == 4) {
      garbageCreate(board, 3, 1);
   }
   else if (matchSize == 5) {
      garbageCreate(board, 4, 1);
   }
   else if (matchSize == 6) {
      garbageCreate(board, 5, 1);
   }
   else if (matchSize == 7) {
      garbageCreate(board, 6, 1);
   }
   else if (matchSize == 8) {
      garbageCreate(board, 3, 1);
      garbageCreate(board, 4, 1);
   }
   else if (matchSize == 9) {
      garbageCreate(board, 4, 1);
      garbageCreate(board, 5, 1);
   }
   else if (matchSize == 10) {
      garbageCreate(board, 5, 1);
      garbageCreate(board, 5, 1);
   }
   else if (matchSize == 11) {
      garbageCreate(board, 6, 1);
      garbageCreate(board, 6, 1);
   }
   else if (matchSize == 12) {
      garbageCreate(board, 6, 1);
      garbageCreate(board, 6, 1);
      garbageCreate(board, 6, 1);
   }
}

//Dumps garbage on the other player depending on match size
void boardComboGarbage(Game* game, int player, int matchSize) {
   Board* board;
   if (player == 1) { board = game->boards[1]; }
   else if (player == 2) { board = game->boards[0]; }

   if (board) {
      if (player == 1) {
         _calcComboGarbage(board, matchSize);  //Drop on player 2
      }
      else if (player == 2) {
         _calcComboGarbage(board, matchSize);
      }
   }
}

static void _silverClear(Game* game, int size, int player) {
   Board* board;
   if (player == 1) { board = game->boards[1]; }
   else if (player == 2) { board = game->boards[0]; }

   if (board) {
      int metals = min(size, 7);
      for (int i = 3; i <= size; i++) {  //Drop a bunch of metal
         garbageCreate(board, 6, 1, true);
      }

      if (size > 3) {  //Extra non-metal garbage
         int width = min(size - 1, 6);
         garbageCreate(board, width, 1);
      }
   }
}

//Matchmaker matchmaker make me a match!
static void _checkClear(std::vector <Tile*> tiles, std::vector <Tile*> &matches) {
   int current = 0;

   while (current + 2 < tiles.size()) {
      Tile* t1 = tiles[current];
      Tile* t2 = tiles[current + 1];
      Tile* t3 = tiles[current + 2];

      if (t1->falling || t2->falling || t3->falling) {  // if it's falling, don't match it
         current++;
         continue;
      }

      if (t1->status == status_disable || t2->status == status_disable || t3->status == status_disable) {  // if it's disabled, don't match it
         current++;
         continue;
      }
      if (t1->status == status_stop || t2->status == status_stop || t3->status == status_stop) {  // if it's stopped, don't match it
         current++;
         continue;
      }

      if (t1->type != tile_empty && t1->type != tile_cleared && t1->type != tile_garbage) {
         if (t1->type == t2->type && t1->type == t3->type) {
            if ( (t1->ypos == t2->ypos && t1->ypos == t3->ypos) || (t1->xpos == t2->xpos && t1->xpos == t3->xpos) ) {
               //We have a match... add to match list and move counter ahead looking for more
               matches.push_back(t1);
               matches.push_back(t2);
               matches.push_back(t3);

               current = current + 3;
               while (current < tiles.size()) {  //keep matching
                  t1 = tiles[current];
                  if (t1->type == tiles[current - 1]->type) {
                     matches.push_back(t1);
                     current++;
                  }
                  else {
                     current++;
                     break;
                  }
               }
            }
         }
      }
      current++;
   }
}

//Checks a list of tiles to see if any matches were made
void boardCheckClear(Board* board, std::vector <Tile*> tileList, bool fallCombo) {
   std::vector <Tile*> matches;

   for (auto&& tile : tileList) {
      std::vector <Tile*> cols = boardGetAllTilesInCol(board, tileGetCol(board, tile));
      std::vector <Tile*> rows = boardGetAllTilesInRow(board, tileGetRow(board, tile));

      _checkClear(cols, matches);
      _checkClear(rows, matches);
   }

   if (matches.size() <= 0) { return; }

   std::vector <Tile*> uniqueMatches;
   for (int i = 0; i < matches.size(); i++) {  //Check if the match list is unique
      bool unique = true;
      for (int j = i + 1; j < matches.size(); j++) {
         if (matches[j] == matches[i]) {
            unique = false;
         }
      }
      if (unique == true) {
         uniqueMatches.push_back(matches[i]);
      }
   }

   if (board->game->players > 1 && uniqueMatches.size() > 3) {  //Check for combos in clear
      boardComboGarbage(board->game, board->player, uniqueMatches.size() );
   }

   int silvers = 0;
   if (uniqueMatches.size() > 0) {
      board->boardStats.clears += uniqueMatches.size();  //Board stats
      board->boardStats.comboCounts[uniqueMatches.size()] += 1;
      boardPauseTime(board, pause_combo, uniqueMatches.size() );
      board->game->soundToggles[sound_clear] = true; 

      //first try to render arbitrary text over the board
      VisualEvent event;
      event.effect = visual_clear;
      event.end = board->game->timer + REMOVE_CLEARS/2;
      event.pos.x = board->cursor->x;
      event.pos.y = board->cursor->y;
      board->visualEvents.push_back(event);

      if (board->level < 10) {  
         board->level += (float) uniqueMatches.size() / LEVEL_UP;  //The more you clear, the faster you go
      }

      int clearTime = board->game->timer;  
      for (auto&& m : uniqueMatches) {
         if (m->type == tile_silver) { silvers++; } 

         garbageCheckClear(board, m);  //Make sure we didn't clear garbage
         //clear block and set timer
         meshSetTexture(board->game, m->mesh, Texture_cleared);
         m->type = tile_cleared;
         m->clearTime = clearTime;
         m->falling = false;
         if (fallCombo && m->chain == true) {
            board->chain += 1;
            fallCombo = false;
            board->game->soundToggles[sound_chain] = true;
            //todo visual queue that chain is occuring
         }

         //todo add score logic here
      }
   }
   if (silvers > 0 && board->game->players > 1) { _silverClear(board->game, silvers, board->player); }
}

//Detects and adjusts all the positions of the tiles that are falling
void boardFall(Board* board, float velocity) {
   std::vector <Tile*> tilesToCheck;
   float drop = velocity;

   for (int col = 0; col < board->w; col++) {
      for (int row = board->wBuffer - 1; row >= 0; row--) {
         Tile* tile = boardGetTile(board, row, col);
         bool prevFall = tile->falling;  //Was the tile falling previously

         if (tile->type == tile_empty || tile->type == tile_cleared || tile->type == tile_garbage) {continue; } 
         if (tile->status == status_disable || tile->status == status_stop) {  //Don't fall if it's stopped/disabled
            tile->falling = true;
            continue; 
         }  
         if (row >= board->wBuffer - 1) { //skip the bottom row
            tile->falling = false;
            continue;
         }
         
         int lookDown = 2;
         Tile* below = boardGetTile(board, row + 1, col);
         while (below && below->type == tile_empty) {
            below = boardGetTile(board, row + lookDown, col);
            lookDown++;
         }
         
         float potentialDrop = drop;
         potentialDrop = below->ypos - (tile->ypos + (float)board->tileHeight);  //check how far we can drop it

         if (potentialDrop <= drop) {  //We swapped a tile into it as it fell
            tile->ypos = below->ypos - board->tileHeight;
            if (below->falling == true) { 
               tile->falling = true; }
            else { 
               tile->falling = false;
            }
         }
         else if (potentialDrop > drop) {  //We can fall as much as we want
            tile->ypos += drop;
            tile->falling = true;
         }

         if (prevFall == true && tile->falling == false) {
            tilesToCheck.push_back(tile);
            board->game->soundToggles[sound_land] = true;
         }
         else if (prevFall == false && tile->falling == false) {
            int lookDown = 2;
            Tile* below = boardGetTile(board, row + 1, col);
            while (below && below->type != tile_cleared) {
               below = boardGetTile(board, row + lookDown, col);
               lookDown++;
            }
            if (below == nullptr) {
               tile->chain = false;
            }
         }
      }
   }
   if (tilesToCheck.size() > 0) {
      boardCheckClear(board, tilesToCheck, true);
   }
}

//Generates a random tile type that doesn't create a match
static TileType _tileGenType(Board* board, Tile* tile) {
   //Used to randomly generate the type of a tile while not matching it to surrounding tiles
   int current = boardRandomTile(board);
   TileType type = (TileType)current;

   int row = tileGetRow(board, tile);
   int col = tileGetCol(board, tile);

   //make sure we don't create any matches in new tiles
   Tile* left = boardGetTile(board, row, col - 1);
   Tile* left2 = boardGetTile(board, row, col - 2);
   Tile* up = boardGetTile(board, row - 1, col);
   Tile* up2 = boardGetTile(board, row - 2, col);

   int total = 6;
   while ((type == left->type && type == left2->type) || (type == up->type && type == up2->type)) {
      current++;
      if (current > total) {
         current = current % total;
      }
      type = (TileType)current;
   }
   return type;
}

//Removes any tiles that have the cleared type and statuses
void boardRemoveClears(Board* board) {
   int current = board->game->timer;
   bool stillChaining = false;
   board->game->soundToggles[sound_anxiety] = false;

   for (int row = board->endH -1; row >= board->startH; row--) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type == tile_empty) { continue; }

         //todo Anxiety doesn't really belong here
         if (tileGetRow(board, tile) <= board->startH + 1 && tile->falling == false) { 
            if (tile->type == tile_garbage) {
               Garbage* garbage = garbageGet(board->pile, tile->idGarbage);
               if (garbage->falling == false) {
                  board->game->soundToggles[sound_anxiety] = true;
                  board->boardStats.dangeresque += 1;  //Board Stats
               }
            }
            else {
               board->game->soundToggles[sound_anxiety] = true;
               board->boardStats.dangeresque += 1;  //Board Stats
            }
         }

         if (tile->status != status_normal && tile->statusTime <= current) {  //Remove special temporary tile statuses
            tile->status = status_normal;
            tile->statusTime = 0;
         }

         if (tile->type == tile_cleared) {
            if (tile->idGarbage >= 0 && tile->clearTime <= current) {

               tile->type = _tileGenType(board, tile);
               tileSetTexture(board, tile);
               tile->idGarbage = -1;
               tile->status = status_disable;
               tile->statusTime += current + REMOVE_CLEARS;
               tile->clearTime = 0;
               tile->chain = true;
               boardPauseTime(board, pause_garbageclear);
            }

            else if (tile->clearTime + REMOVE_CLEARS <= current) {  //Regular tile clearing
               tile->type = tile_empty;
               tile->chain = false;
               meshSetTexture(board->game, tile->mesh, Texture_empty);
               tile->clearTime = 0;

               //flag blocks above the clear as potentially part of a chain
               int r = row - 1;
               Tile* above = boardGetTile(board, r, col);
               while (above && r >= 0) {
                  if (above->type != tile_empty && above->type != tile_cleared && above->type != tile_garbage) { above->chain = true; }
                  r--;
                  above = boardGetTile(board, r, col);
               }
            }
         }
         if (tile->chain == true) { stillChaining = true; }  //Is any tile still part of a chain?
      }
   }
   if (stillChaining == false) { //No tiles are part of a chain
      if (board->chain > 1) {
         if (board->game->players > 1) { boardChainGarbage(board->game, board->player, board->chain); }
         boardPauseTime(board, pause_chain, board->chain); 
         board->boardStats.lastChain = board->chain;
         board->boardStats.chainCounts[board->chain] += 1;  //Board Stats
      }
      board->chain = 1; 
   }
   //else if (stillChaining == true) {
   //   boardPauseTime(board, pause_clear);
   //}
   return;
}

//Moves all the tile on the board up a given amount
void boardMoveUp(Board* board, float height) {
   //Moves all tiles up a fixed amount
   float nudge = height;
   board->offset -= nudge;

   bool dangerZone = false;  //About to bust

   cursorSetY(board->cursor, cursorGetY(board->cursor) - nudge);  //adjust cursor position

   if (board->offset <= -1 * board->tileHeight) {
      board->offset += board->tileHeight;
   }

   std::vector <Tile*> checkTiles;
   for (int row = 0; row < board->wBuffer; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type == tile_empty) { continue; }  //don't move up empty blocks

         //Bust logic
         if (tile->ypos <= 0.0f && tile->falling == false) { //Tile is above the top of the board and not falling
            if (tile->type == tile_garbage) {  
               Garbage* garbage = garbageGet(board->pile, tile->idGarbage);
               if (garbage->falling == false) { dangerZone = true; } 
            }
            else { dangerZone = true; }  
         } 
         if (row == board->endH - 1) { checkTiles.push_back(tile); }  //Check the bottom row for clears
         tile->ypos -= nudge;  
      }
   }
   
   if (dangerZone == true) {
      if (board->danger == false) {  //grace period
         boardPauseTime(board, pause_danger);
      }
      board->danger = true;
   }
   else { board->danger = false; }

   boardCheckClear(board, checkTiles, false);
}

//Fills half the board with tiles so that there are no matches
int boardFillTiles(Board* board) {
   for (int row = 0; row < board->wBuffer; row++) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (row < board->startH + (board->endH - board->startH) / 2) {
            tileInit(board, tile, row, col, tile_empty, true);
            continue;
         }
         
         TileType type = _tileGenType(board, tile);
         if (col % 2 == 0) {
            tileInit(board, tile, row - board->startH - 3, col, type, true);
         }
         else {
            tileInit(board, tile, row - board->startH - 2, col, type, true);
         }
      }
   }
   return 0;
}

//Takes all the non-empty tiles in the board and assigns them a slot based on their position
void boardAssignSlot(Board* board, bool buffer = false) {
   std::vector <Tile> tileList;

   for (int row = 0; row < board->wBuffer; row++) {  //Loop through all the tiles and save them in a vector
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (tile->type != tile_empty) { 
            tileList.push_back(*tile);
         }
         tileInit(board, tile, row, col, tile_empty);  //Set each tile in the array to empty in the starting position
      }
   }

   for (auto&& t : tileList) {  //Take all the tiles and write them back into the array, adjusted for xy position
      int row, col;

      row = (t.ypos + board->tileHeight - 0.00001) / board->tileHeight + board->startH;  //Moving up triggers on last pixel, down on first
      col = t.xpos / board->tileWidth;

      Tile* current = boardGetTile(board, row, col);
      assert(current->type == tile_empty);  //This is a position conflict... bad
      t.mesh = current->mesh;
      *current = t;
      tileSetTexture(board, current);

      if (current->type == tile_garbage && current->garbage != nullptr) {  //if the start tile moves, we need to tell the garbage
         garbageSetStart(board->pile, current);
      }
   }

   for (int col = 0; col < board->w; col++) {  //Finally, check if the buffer row is empty and fill it
      int row = board->wBuffer - 1;
      Tile* current = boardGetTile(board, row, col);
      if (current->type == tile_empty) {
         tileInit(board, current, row, col, (TileType)boardRandomTile(board) );
         current->ypos += board->offset;
      }
   }
}

//Drop a row of tiles from the top of the board
void makeItRain(Board* board) {
   //Debug tool
   int row = 0;
   for (int col = 0; col < board->w; col++) {
      Tile* tile = boardGetTile(board, row, col);
      if (tile->type != tile_empty) { continue; }

      int current = boardRandomTile(board);
      TileType type = (TileType)current;

      //make sure we don't create any matches on startup
      Tile* left = boardGetTile(board, row, col - 1);
      Tile* left2 = boardGetTile(board, row, col - 2);

      int total = 6;
      if (left && left2) {
         while ((type == left->type && type == left2->type)) {
            current++;
            if (current > total) {
               current = current % total;
            }
            type = (TileType)current;
         }
         if (col % 2 == 0) {
            tileInit(board, tile, row, col, type, true);
         }
         else {
            tileInit(board, tile, row + 1, col, type, true);
         }
      }
   }
}

//Clears the board except the bottom layer
void boardClear(Board* board) {
   //Debug tool
   garbagePileDestroy(board->pile);
   board->pile = garbagePileCreate();

   for (int row = 0; row < board->wBuffer; row++) {  //Loop through all the tiles and save them in a vector
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         tileInit(board, tile, row, col, tile_empty);
         tile->idGarbage = -1;
         tile->garbage = nullptr;
      }
   }
}