#include "board_ai.h"
#include "board.h"

#include <list>

//Everything below here if for AI Logic
enum AIStep {
   ai_none = 0,
   ai_left,
   ai_right,
   ai_up,
   ai_down,
   ai_swap,
   ai_COUNT,
};

enum AIMoveType {
   ai_no_move = 0,
   ai_vert_match,
   ai_horiz_match,
   ai_vert_chain,
   ai_horiz_chain,
   ai_clear_garbage,
   ai_flatten_board,
   ai_waiting,
};

std::map <AIMoveType, const char*> moveNames = {
   {ai_no_move,       "No move"},
   {ai_vert_match,    "Vert Match"},
   {ai_horiz_match,   "Horiz Match"},
   {ai_vert_chain,    "Chain Vert"},
   {ai_horiz_chain,   "Chain Horiz"},
   {ai_clear_garbage, "Clear Garbage"},
   {ai_flatten_board, "Flatten Board"},
   {ai_waiting,       "Waiting"},
};

struct TileIndex {
   int row = INT_MIN;
   int col = INT_MIN;
};

struct MoveInfo {
   TileIndex target;
   TileIndex dest;
};

struct AILogic {

   std::list <MoveInfo> moves;            //Each tile's current row/col position and its row/col destination
   std::list <AIStep> matchSteps;         //The Cursor movements needed to move the tile to its destination
   AIMoveType currentMove = ai_no_move;   //What is the current move of the ai

   //Chain specific logic
   bool waiting = false;                  //Are we delaying actions because of a clear
   Tile* clearedTile = nullptr;           //The tile that needs to be cleared so the chain can fall
   int fallTile = -1;                     //The tile that needs to fall to make the chain

   bool moveUp = false;                   //Should we pull the board up
};

//Holds the move steps for each ai opponent
std::map <int, AILogic> aiLogic;

const char* aiGetMove(int player) {
   return moveNames[aiLogic[player].currentMove];
}

//Get a tile using the row and col
int _checkTileLookup(Board* board, int row, int col) {
   int out = (board->w * row + col);
   return out;
}

//Is the tile something the ai can move right now
static bool _validTile(Board* board, Tile* tile) {
   if (tile->falling == true || tile->status == status_disable || tile->status == status_clear ||
      tile->status == status_stop || tile->type == tile_empty) {
      return false;
   }
   else { return true; }
}

//Based on the game mode, find out which board to work on
void boardAI(Game* game) {
   if (game->settings.mode == single_vs) {
      for (int i = 1; i < game->pList.size(); i++) {  //Skip first player
         aiChooseMove(game->pList[i + 1].board, game->pList[i + 1].number);
      }
   }
   else if (game->net->syncTest == true || game->settings.mode == single_player) {
      aiChooseMove(game->boards[0], 1);
   }
   else if (game->settings.mode == multi_shared || game->settings.mode == multi_solo) {
      aiChooseMove(game->pList[game->user.number].board, game->user.number);
   }

}

static void _stopWaiting(int player) {
   aiLogic[player].waiting = false;
   aiLogic[player].clearedTile = nullptr;
   aiLogic[player].fallTile = -1;
}

//Basically a flow chart of possible actions the AI can take
void aiChooseMove(Board* board, int player) {
   aiMoveBoardUp(board, player);
   aiClearGarbage(board, player);
   if (aiLogic[player].moves.empty() == false) { aiGetSteps(board, player); }  //Figure out cursor movements to move target to destination

   if (aiLogic[player].waiting == true) {  //Check if we're still waiting for a clear to finish
      //aiLogic[player].currentMove = ai_waiting;
      Tile* fallTile = board->tileLookup[aiLogic[player].fallTile];
      if (fallTile) {
         if (aiLogic[player].clearedTile->status != status_clear && fallTile->falling == false) {
            _stopWaiting(player);
         }
      }
      else { _stopWaiting(player); }
   }
   else {
      //These moves can happen even in the middle of another move
      if (aiLogic[player].moves.empty() == true) { aiChain(board, player); }
      if (aiLogic[player].moves.empty() == false) { aiGetSteps(board, player); }  //Figure out cursor movements to move target to destination

      if (aiLogic[player].matchSteps.empty() == true) {  //Only do these if no other moves are in progress
         aiLogic[player].currentMove = ai_no_move;

         if (aiLogic[player].moves.empty() == true) { aiFindVertMatch(board, player); }
         if (aiLogic[player].moves.empty() == true) { aiFindHorizMatch(board, player); }
         if (aiLogic[player].moves.empty() == true) { aiFlattenBoard(board, player); }
      }

      if (aiLogic[player].moves.empty() == false) { aiGetSteps(board, player); }  //Figure out cursor movements to move target to destination
   }

   aiDoStep(board, player);  //Transfer cursor movements to inputs
}

//Calculate the steps to move cursor into place
void aiGetSteps(Board* board, int player) {
   Cursor* cursor;
   if (board->game->net->syncTest == true) { cursor = board->cursors[0]; }
   else if (board->game->settings.mode == single_player) { cursor = board->game->pList[board->game->user.number].cursor; }
   else if (board->game->settings.mode == single_vs) { cursor = board->game->pList[player].cursor; }
   else { cursor = board->game->pList[board->game->user.number].cursor; }
   int cursorCol = cursorGetCol(board, cursor);
   int cursorRow = cursorGetRow(board, cursor);

   for (auto&& move : aiLogic[player].moves) {
      if (move.dest.col == move.target.col && move.dest.row == move.target.row) { continue; }
      //Figure out if the target needs to move left or right
      int moveDirection = move.dest.col - move.target.col;

      int colDiff = move.target.col - cursorCol;
      int rowDiff = move.target.row - cursorRow;

      if (moveDirection < 0) { colDiff -= 1; }  //If we need to swap left, shift the cursor left one

      for (int i = 0; i < abs(colDiff); i++) {
         if (colDiff < 0) {        //move left
            aiLogic[player].matchSteps.push_back(ai_left);
            cursorCol--;
         }
         else if (colDiff > 0) {   //move right
            aiLogic[player].matchSteps.push_back(ai_right);
            cursorCol++;
         }
      }
      for (int i = 0; i < abs(rowDiff); i++) {
         if (rowDiff < 0) {        //move up
            aiLogic[player].matchSteps.push_back(ai_up);
            cursorRow--;
         }
         else if (rowDiff > 0) {   //move down
            aiLogic[player].matchSteps.push_back(ai_down);
            cursorRow++;
         }
      }

      //Figure out how many swaps to move the target tile to the destination
      for (int i = 0; i < abs(moveDirection); i++) {
         aiLogic[player].matchSteps.push_back(ai_swap);
         if (abs(moveDirection) == i + 1) { break; }
         if (moveDirection < 0) {        //move left
            aiLogic[player].matchSteps.push_back(ai_left);
            cursorCol--;
         }
         else if (moveDirection > 0) {   //move right
            aiLogic[player].matchSteps.push_back(ai_right);
            cursorCol++;
         }
      }
   }
   aiLogic[player].moves.clear();
}

//Take the move step and transfer it to the player inputs
void aiDoStep(Board* board, int player) {
   UserInput input;
   if (board->game->settings.mode != single_vs) { input = board->game->user.input; }
   if (aiLogic[player].moveUp == true) { input.nudge.p = true; }
   if (aiLogic[player].matchSteps.empty() == false) {
      if (board->game->frameCount % board->game->aiDelay[0] == 0) {  //This is so it doesn't have 1000 apm
         AIStep step = aiLogic[player].matchSteps.front();
         aiLogic[player].matchSteps.pop_front();

         switch (step) {
         case ai_left:
            input.left.p = true;
            break;
         case ai_right:
            input.right.p = true;
            break;
         case ai_up:
            input.up.p = true;
            break;
         case ai_down:
            input.down.p = true;
            break;
         case ai_swap:
            input.swap.p = true;
            break;
         }
      }
   }
   if (board->game->settings.mode == single_vs) {
      board->game->net->inputs[player - 1] = input;
   }
   else {
      board->game->user.input = input;
   }
}

//Look for vertical matches along a row (typically below garbage)
static bool _vertMatch(Board* board, int row, int col, int player) {
   bool moveFound = false;

   Tile* tSet[3];
   bool badTiles = false;
   for (int i = 0; i < 3; i++) { //Check if three tiles can be used
      int currRow = row + i;
      tSet[i] = boardGetTile(board, currRow, col);
      if (tSet[i] == nullptr || _validTile(board, tSet[i]) == false || tSet[i]->type == tile_garbage) { badTiles = true;  break; }
   }
   if (badTiles == true) { return false; }

   std::vector <Tile*> matches;  //Can we find three in a row
   for (int i = 1; i < 3; i++) {  //Compare 2nd and 3rd tile with the first below garbage
      int currRow = row + i;
      if (tSet[0]->type != tSet[i]->type) {
         std::vector <Tile*> tiles = boardGetAllTilesInRow(board, currRow);
         for (int j = 0; j < tiles.size(); j++) {  //Go through all tiles in same row
            if (tiles[j]->type == tile_garbage) { break; }
            if (tiles[j]->type == tSet[0]->type) {
               matches.push_back(tiles[j]);
               break;
            }
         }
      }
      else { matches.push_back(tSet[i]); }
   }
   if (matches.size() == 2) {
      for (auto&& match : matches) {
         MoveInfo move;

         move.target.col = tileGetCol(board, match);
         move.target.row = tileGetRow(board, match);

         move.dest.col = col;
         move.dest.row = move.target.row;
         aiLogic[player].moves.push_front(move);
      }
      moveFound = true;
   }
   return moveFound;
}

//Very basic search for vertical 2 and decide on tile to swap in to match
bool aiFindVertMatch(Board* board, int player) {
   bool moveFound = false;

   for (int row = board->endH - 3; row >= board->startH - 1; row--) {
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);
         if (_validTile(board, tile) == false || tile->type == tile_garbage) { continue; }

         moveFound = _vertMatch(board, row, col, player);
         if (moveFound == true) { 
            aiLogic[player].currentMove = ai_vert_match;
            return moveFound; 
         }
      }
   }

   return moveFound;
}

//Search for a horizontal match
bool aiFindHorizMatch(Board* board, int player) {
   bool moveFound = false;
   for (int row = board->startH - 1; row < board->endH; row++) {
      if (moveFound == true) { break; }
      std::vector <Tile*> tiles = boardGetAllTilesInRow(board, row);  //tiles in row ordered left to right
      std::map <TileType, std::vector <Tile*> > tileCounts;  //Hash of tile type counts

      for (auto&& tile : tiles) {  //Skip this stuff
         if (tile->falling == true || tile->status != status_normal || tile->type == tile_empty || tile->type == tile_garbage) {
            continue;
         }
         tileCounts[tile->type].push_back(tile);
      }

      for (auto&& pair : tileCounts) {
         if (pair.second.size() >= 3) {  //If we have three tiles of the same type in a row
            int row = tileGetRow(board, pair.second[0]);
            int col = tileGetCol(board, pair.second[0]);

            for (int i = 1; i < 3; i++) {

               MoveInfo move;
               move.dest.col = col + i;
               move.dest.row = row;
               int r = tileGetRow(board, pair.second[i]);
               int c = tileGetCol(board, pair.second[i]);
               move.target.col = c;
               move.target.row = r;
               aiLogic[player].moves.push_back(move);

            }
            moveFound = true;
            aiLogic[player].currentMove = ai_horiz_match;
            break;
         }
      }
   }
   return moveFound;
}

void aiMoveBoardUp(Board* board, int player) {
   int highestRow = -1;
   for (int row = board->startH - 1; row < board->wBuffer; row++) {
      if (highestRow != -1) { break; }
      for (int col = 0; col < board->w; col++) {
         Tile* tile = boardGetTile(board, row, col);

         if (tile->type != tile_empty) {
            highestRow = row;
            break;
         }
      }
   }
   if (highestRow != -1 && highestRow > board->startH + (board->h / 2)) {
      aiLogic[player].moveUp = true;
   }
   else {
      aiLogic[player].moveUp = false;
   }
}

//Look for a vertical match below garbage
bool aiClearGarbage(Board* board, int player) {
   bool moveFound = false;
   for (auto&& pair : board->pile->garbage) {
      Garbage* garbage = pair.second;
      if (garbage->deployed && garbage->falling == false) {
         int gRow = tileGetRow(board, garbage->start);
         int col = tileGetCol(board, garbage->start);

         moveFound = _vertMatch(board, gRow + 1, col, player);
         if (moveFound == true) {
            aiLogic[player].currentMove = ai_clear_garbage;
            return moveFound;
         }
      }
   }
   return moveFound;
}

//If there's nothing else to do, flatten your towers
bool aiFlattenBoard(Board* board, int player) {
   bool moveFound = false;
   for (int row = board->startH - 1; row < board->endH - 1; row++) {
      if (moveFound == true) { break; }
      for (int col = 0; col < board->w; col++) {
         if (moveFound == true) { break; }
         Tile* tile = boardGetTile(board, row, col);
         if (_validTile(board, tile) == false || tile->type == tile_garbage) { continue; }
         Tile* below = boardGetTile(board, row + 1, col);
         if (below && (below->falling == true || below->type == tile_empty)) { continue; }

         //Find a hole in the next row down and send tile there
         std::vector <Tile*> tiles = boardGetAllTilesInRow(board, row + 1);
         int closestCol = 1000;
         int closestDist = 1000;
         for (auto&& below : tiles) {
            if (below->type == tile_empty) {
               int belowCol = tileGetCol(board, below);
               int dist = abs(col - belowCol);
               if (dist < closestDist) {
                  closestCol = belowCol;
                  closestDist = dist;
               }
            }
         }
         if (closestCol != 1000) {
            MoveInfo move;
            move.target.col = col;
            move.target.row = row;

            move.dest.col = closestCol;
            move.dest.row = row;
            aiLogic[player].moves.push_back(move);
            moveFound = true;
            aiLogic[player].currentMove = ai_flatten_board;
            break;
         }
      }
   }
   return moveFound;
}

void _aiVertChain(Board* board, Tile* tile, std::map <int, bool>& checkedTiles, bool& moveFound, int row, int col, int player) {
   std::map <TileType, std::vector <Tile*> > tileCounts;  //Hash of tile type counts

   int botRow = row;
   while (botRow < board->wBuffer) {
      Tile* current = boardGetTile(board, botRow, col);
      if (current && (current->status == status_clear || current->type == tile_empty)) {
         if (current->status == status_clear) { 
            checkedTiles[_checkTileLookup(board, botRow, col)] = true;
         }
         botRow++;
      }
      else { 
         botRow--;
         break; 
      }
   }

   //Get all tiles in bottom of the clear
   for (int i = 0; i < board->w; i++) {
      Tile* t = boardGetTile(board, botRow, i);
      if (!t || t->falling == true || t->status != status_normal || t->type == tile_empty || t->type == tile_garbage) {
         continue;
      }
      else { tileCounts[t->type].push_back(t); }
   }

   //Look for the bottom row and see if we have two of the same
   for (auto&& pair : tileCounts) {
      if (pair.second.size() >= 2) {
         MoveInfo moves[3];  //Holds the moves for the top and bottom tiles

         //Look for a tile above the clear that matches the bottom row couple
         bool topFound = false;
         for (int i = 0; i < board->w; i++) {
            Tile* t = boardGetTile(board, row - 1, i);
            if (!t || t->falling == true || t->status != status_normal || t->type == tile_empty || t->type == tile_garbage) {
               continue;
            }
            if (t->type == pair.second[0]->type) {
               moves[2].target.col = i;
               moves[2].target.row = row - 1;

               moves[2].dest.col = col;
               moves[2].dest.row = row - 1;

               aiLogic[player].fallTile = t->ID;  //Wait for this tile to fall so we don't mess up the chain
               topFound = true;
               break;
            }
         }
         if (topFound == false) { continue; }

         //Check which bottom tile we need to swap first (order of list is left to right)
         moves[0].target.row = moves[1].target.row = botRow;
         moves[0].dest.row = moves[1].dest.row = botRow;

         int botTileCol[2] = { tileGetCol(board, pair.second[0]), tileGetCol(board, pair.second[1]) };
         if (botTileCol[1] < col) {  //both on the left
            moves[0].target.col = botTileCol[1];
            moves[1].target.col = botTileCol[0];

            moves[0].dest.col = col - 1;
            moves[1].dest.col = col - 2;
         }
         else if (botTileCol[0] > col) {  //both on the right
            moves[0].target.col = botTileCol[0];
            moves[1].target.col = botTileCol[1];

            moves[0].dest.col = col + 1;
            moves[1].dest.col = col + 2;
         }
         else if (botTileCol[0] < col && botTileCol[1] > col) {  //They are split
            moves[0].target.col = botTileCol[0];
            moves[1].target.col = botTileCol[1];

            moves[0].dest.col = col - 1;
            moves[1].dest.col = col + 1;
         }

         aiLogic[player].moves.clear();
         for (int i = 0; i < 3; i++) {
            aiLogic[player].moves.push_back(moves[i]);
         }
         moveFound = true;
         aiLogic[player].waiting = true;
         aiLogic[player].clearedTile = tile;
         break;
      }
   }
}

void _aiHorizChain(Board* board, Tile* tile, std::map <int, bool>& checkedTiles, bool& moveFound, int row, int col, int player) {
   if (row == board->startH - 1) { return; }
   std::map <TileType, std::vector <Tile*> > tCountAbove;  //Hash of tile type counts
   std::map <TileType, std::vector <Tile*> > tCountAlong;  //Hash of tile type counts

   int rightCol = col;
   while (rightCol < board->w) {
      Tile* current = boardGetTile(board, row, rightCol);
      if (current && current->status == status_clear) {
         checkedTiles[_checkTileLookup(board, row, rightCol)] = true;
         rightCol++;
      }
      else {
         rightCol--;
         break;
      }
   }

   int startCol = col;
   int endCol = rightCol;

   //Get all tiles above the clear
   for (int i = 0; i < board->w; i++) {
      Tile* t = boardGetTile(board, row - 1, i);
      if (!t || t->falling == true || t->status != status_normal || t->type == tile_empty || t->type == tile_garbage) {
         continue;
      }
      else { tCountAbove[t->type].push_back(t); }
   }

   //Get all tiles above the clear
   for (int i = 0; i < board->w; i++) {
      Tile* t = boardGetTile(board, row, i);
      if (!t || t->falling == true || t->status != status_normal || t->type == tile_empty || t->type == tile_garbage) {
         continue;
      }
      else { tCountAlong[t->type].push_back(t); }
   }

   //Look through the top row and see if we can find two of the same
   for (auto&& aboveGroup : tCountAbove) {
      if (aboveGroup.second.size() >= 2) {
         if (moveFound == true) { break; }
         MoveInfo moves[3];
         for (auto&& alongGroup : tCountAlong) {  //Look in row with clear for matching type
            if (alongGroup.first == aboveGroup.first) {
               moves[0].target.row = moves[1].target.row = row - 1;
               moves[0].dest.row = moves[1].dest.row = row - 1;

               moves[2].target.row = moves[2].dest.row = row;

               //Take care of the single tile
               Tile* leftSideTile = nullptr;
               Tile* rightSideTile = nullptr;
               if (alongGroup.second[0]->xpos < tile->xpos) {  //The along tile is on the left
                  moves[2].target.col = tileGetCol(board, alongGroup.second[0]);
                  moves[2].dest.col = startCol - 1;

                  leftSideTile = boardGetTile(board, row - 1, startCol + 1);
               }
               else {  //The along tile is on the right
                  moves[2].target.col = tileGetCol(board, alongGroup.second[0]);
                  moves[2].dest.col = endCol + 1;

                  rightSideTile = boardGetTile(board, row - 1, endCol - 1);
               }

               //Take care of the two tiles on top
               if (leftSideTile != nullptr) {  //Overhang is on the left
                  if (aboveGroup.second[1]->xpos <= leftSideTile->xpos) {  //Both tiles are to the left of the destination overhang 
                     moves[0].target.col = tileGetCol(board, aboveGroup.second[1]);
                     moves[1].target.col = tileGetCol(board, aboveGroup.second[0]);

                     moves[0].dest.col = startCol + 1;
                     moves[1].dest.col = startCol;
                  }
                  else {  //Both tiles are to the right of the destination overhang 
                     moves[0].target.col = tileGetCol(board, aboveGroup.second[0]);
                     moves[1].target.col = tileGetCol(board, aboveGroup.second[1]);

                     moves[0].dest.col = startCol;
                     moves[1].dest.col = startCol + 1;
                  }
               }
               else if (rightSideTile != nullptr) {  //Overhang is on the right
                  if (aboveGroup.second[0]->xpos >= rightSideTile->xpos) {  //Both tiles are to the right of the destination overhang 
                     moves[0].target.col = tileGetCol(board, aboveGroup.second[0]);
                     moves[1].target.col = tileGetCol(board, aboveGroup.second[1]);

                     moves[0].dest.col = endCol - 1;
                     moves[1].dest.col = endCol;
                  }
                  else {  //Both tiles are to the left of the destination overhang 
                     moves[0].target.col = tileGetCol(board, aboveGroup.second[1]);
                     moves[1].target.col = tileGetCol(board, aboveGroup.second[0]);

                     moves[0].dest.col = endCol;
                     moves[1].dest.col = endCol - 1;
                  }
               }

               aiLogic[player].moves.clear();
               for (int i = 0; i < 3; i++) {
                  aiLogic[player].moves.push_back(moves[i]);
               }
               moveFound = true;
               aiLogic[player].waiting = true;
               aiLogic[player].fallTile = aboveGroup.second[0]->ID;  //Wait for this tile to fall so we don't mess up the chain
               aiLogic[player].clearedTile = tile;
               return;
            }
         }
      }
   }

   //If that fails, look through the top for 1 and the clear row for 2
   for (auto&& alongGroup : tCountAlong) {  //Loop through along row, looking for 2
      if (moveFound == true) { break; }
      if (alongGroup.second.size() >= 2) {
         MoveInfo moves[3];
         if (alongGroup.second[0]->xpos < tile->xpos && alongGroup.second[1]->xpos > tile->xpos) { continue; }  //Skip a split

         for (auto&& aboveGroup : tCountAbove) {  //Look in above row for one matching type
            if (alongGroup.first == aboveGroup.first) {
               moves[0].target.row = moves[1].target.row = row;
               moves[0].dest.row = moves[1].dest.row = row;

               moves[2].target.row = moves[2].dest.row = row - 1;

               //Take care of the two tiles in the along row
               bool leftOverhang = false;
               int alongTileCol[2] = { tileGetCol(board, alongGroup.second[0]), tileGetCol(board, alongGroup.second[1]) };
               if (alongTileCol[1] <= startCol) {  //Both tiles are to the left of the clear
                  moves[0].target.col = alongTileCol[1];
                  moves[1].target.col = alongTileCol[0];

                  moves[0].dest.col = startCol - 1;
                  moves[1].dest.col = startCol - 2;
                  leftOverhang = true;
               }
               else {  //Both tiles are to the right of the clear
                  moves[0].target.col = alongTileCol[0];
                  moves[1].target.col = alongTileCol[1];

                  moves[0].dest.col = endCol + 1;
                  moves[1].dest.col = endCol + 2;
               }

               //Take care of the single tile
               if (leftOverhang == true) {  //Make a left overhang
                  moves[2].target.col = tileGetCol(board, aboveGroup.second[0]);
                  moves[2].dest.col = startCol;
               }
               else {  //The above tile is on the right
                  moves[2].target.col = tileGetCol(board, aboveGroup.second[0]);
                  moves[2].dest.col = endCol;
               }

               aiLogic[player].moves.clear();
               for (int i = 0; i < 3; i++) {
                  aiLogic[player].moves.push_back(moves[i]);
               }
               moveFound = true;
               aiLogic[player].waiting = true;
               aiLogic[player].fallTile = aboveGroup.second[0]->ID;  //Wait for this tile to fall so we don't mess up the chain
               aiLogic[player].clearedTile = tile;
               return;
            }
         }
      }
   }
}

//Look at a snapshot of the board and decide how to make a chain
void aiChain(Board* board, int player) {
   bool moveFound = false;
   std::map <int, bool> checkedVert;
   std::map <int, bool> checkedHoriz;

   for (int row = board->startH; row < board->endH - 1; row++) {  //sweet spot
      for (int col = 0; col < board->w; col++) {
         if (moveFound == true) { return; }
         Tile* tile = boardGetTile(board, row, col);
         if (tile->status == status_clear) {
            Tile* below = boardGetTile(board, row + 1, col);
            Tile* right = boardGetTile(board, row, col + 1);

            if (below && below->status == status_clear) {  //Check for vertical clear
               if (checkedHoriz[_checkTileLookup(board, row, col)] == false) {
                  _aiVertChain(board, tile, checkedHoriz, moveFound, row, col, player);
               }
               if (moveFound == true) { 
                  aiLogic[player].currentMove = ai_vert_chain;
                  return; 
               }
            }
            if (right && right->status == status_clear) {  //Check for horizontal clear
               if (checkedHoriz[_checkTileLookup(board, row, col)] == false) {
                  _aiHorizChain(board, tile, checkedHoriz, moveFound, row, col, player);
               }
               if (moveFound == true) { 
                  aiLogic[player].currentMove = ai_horiz_chain;
                  return; 
               }
            }
         }
      }
   }
}
