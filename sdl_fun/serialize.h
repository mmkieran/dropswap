#include "game.h"
#include "board.h"

void _gameSerialize(Game* game, FILE* file);
int _gameDeserialize(Game* game, FILE* file);
void _boardSerialize(Board* board, FILE* file);
int _boardDeserialize(Board* board, FILE* file);
