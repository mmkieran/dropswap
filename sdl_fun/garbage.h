#pragma once

typedef struct Board Board;

struct Garbage;

Garbage* garbageCreate(Board* board, int width, int layers);
void garbageDestroy(Garbage* garbage);

void garbageClear();

void garbageFall(Board* board, float velocity);

void garbageDraw(Board* board);