#pragma once

#include "game.h"

typedef struct Board Board;

void boardAI(Game* game);

void aiChooseMove(Board* board, int player);
void aiGetSteps(Board* board, int player);
void aiDoStep(Board* board, int player);

bool aiFindVertMatch(Board* board, int player);
bool aiFindHorizMatch(Board* board, int player);
void aiMoveBoardUp(Board* board, int player);
bool aiClearGarbage(Board* board, int player);
bool aiFlattenBoard(Board* board, int player);
void aiChain(Board* board, int player);

const char* aiGetMove(int player);