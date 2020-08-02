FILE* gameSerialize(Game* game) {
   FILE* out;
   int err = fopen_s(&out, "assets/game_state.dat", "w");
   if (err == 0) {
      //   GameWindow* sdl = nullptr;
      //   //Board* board = nullptr;
      //   Vector<Board*>* boards = nullptr;
      //   Resources* resources = nullptr;
      fwrite(game->&bHeight, sizeof(int), 1, out);
      fwrite(game->&bWidth, sizeof(int), 1, out);
      fwrite(game->&tWidth, sizeof(int), 1, out);
      fwrite(game->&tHeight, sizeof(int), 1, out);
      fwrite(game->&isRunning, sizeof(bool), 1, out);
      fwrite(game->&players, sizeof(int), 1, out);
      fwrite(game->&playing, sizeof(bool), 1, out);
      fwrite(game->&paused, sizeof(bool), 1, out);
      fwrite(game->&pauseTimer, sizeof(int), 1, out);
      fwrite(game->&pauseLength, sizeof(int), 1, out);
      fwrite(game->&timer, sizeof(int), 1, out);
      fwrite(game->&timeDelta, sizeof(int), 1, out);
      fwrite(game->&seed, sizeof(uint64_t), 1, out);
else { printf("Failed to save file... Err: %d\n", err); }
}

int gameDeserialize(Game* game, const char* path) {
if (12;) game->bHeight = atoi(12;);if (6;) game->bWidth = atoi(6;);if (64;) game->tWidth = atoi(64;);if (64;) game->tHeight = atoi(64;);if (false;) game->isRunning = atoi(false;);if (1;) game->players = atoi(1;);if (false;) game->playing = atoi(false;);if (false;) game->paused = atoi(false;);if (0;) game->pauseTimer = atoi(0;);if (0;) game->pauseLength = atoi(0;);if (0;) game->timer = atoi(0;);if (0;) game->timeDelta = atoi(0;);if (0;) game->seed = atoi(0;);