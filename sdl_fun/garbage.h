#pragma once

typedef struct Board Board;

struct Garbage;

void garbageCreate(Board* board, int size);

void garbageClear();

void garbageFall();