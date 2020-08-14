
#include "netplay.h"
#include "game.h"

extern Game* game;  //I dunno how I feel about this

void SetConnectState(GGPOPlayerHandle handle, PlayerConnectState state) {
   for (int i = 0; i < game->players; i++) {
      if (game->net->connections[i].handle == handle) {
         game->net->connections[i].connect_progress = 0;
         game->net->connections[i].state = state;
         break;
      }
   }
}

void SetDisconnectTimeout(GGPOPlayerHandle handle, int when, int timeout) {
   for (int i = 0; i < game->players; i++) {
      if (game->net->connections[i].handle == handle) {
         game->net->connections[i].disconnect_start = when;
         game->net->connections[i].disconnect_timeout = timeout;
         game->net->connections[i].state = Disconnecting;
         break;
      }
   }
}

void SetConnectState(PlayerConnectState state) {
   for (int i = 0; i < game->players; i++) {
      game->net->connections[i].state = state;
   }
}

void UpdateConnectProgress(GGPOPlayerHandle handle, int progress) {
   for (int i = 0; i < game->players; i++) {
      if (game->net->connections[i].handle == handle) {
         game->net->connections[i].connect_progress = progress;
         break;
      }
   }
}

//Don't call it a callback!
bool __cdecl ds_begin_game_callback(const char*) {
   //we don't need to do anything here apparently
   return true;
}

bool __cdecl ds_advance_frame_callback(int) {

   int disconnect_flags;

   int input = 0;
   int inputs[GAME_PLAYERS];

   //Figure out the inputs and check for disconnects
   ggpo_synchronize_input(game->net->ggpo, (void*)game->inputs, sizeof(UserInput) * GAME_PLAYERS, &disconnect_flags);
   //ggpo_synchronize_input(game->net->ggpo, (void*)inputs, sizeof(int) * GAME_PLAYERS, &disconnect_flags);

   //Call function to advance frame
   gameAdvanceFrame(game);

   return true;
}

bool __cdecl ds_load_game_callback(unsigned char* buffer, int len) {

   if (len > 0) {
      unsigned char* start = buffer;
      gameLoad(game, start);

      return true;
   }
   else { return false; }
}

bool __cdecl ds_save_game_callback(unsigned char** buffer, int* len, int* checksum, int) {

   std::vector <Byte> stream = gameSave(game);
   if (stream.size() == 0) { return false; }
   *len = stream.size();

   *buffer = (unsigned char*)malloc(*len);
   if (buffer) {
      memcpy(*buffer, stream.data(), *len);
      return true;
   }
   else { return false; }

}

void __cdecl ds_free_buffer_callback(void* buffer) {
   if (buffer) { free(buffer); }
}

bool __cdecl ds_on_event_callback(GGPOEvent* info) {
   int progress;

   switch (info->code) {
   case GGPO_EVENTCODE_CONNECTED_TO_PEER:
       SetConnectState(info->u.connected.player, Synchronizing);
       break;
   case GGPO_EVENTCODE_SYNCHRONIZING_WITH_PEER:
       progress = 100 * info->u.synchronizing.count / info->u.synchronizing.total;
       UpdateConnectProgress(info->u.synchronizing.player, progress);
       break;
   case GGPO_EVENTCODE_SYNCHRONIZED_WITH_PEER:
       UpdateConnectProgress(info->u.synchronized.player, 100);
       break;
   case GGPO_EVENTCODE_RUNNING:
       SetConnectState(Running);
       break;
   case GGPO_EVENTCODE_CONNECTION_INTERRUPTED:
       SetDisconnectTimeout(info->u.connection_interrupted.player, game->kt.getTime(), info->u.connection_interrupted.disconnect_timeout);
       break;
   case GGPO_EVENTCODE_CONNECTION_RESUMED:
       SetConnectState(info->u.connection_resumed.player, Running);
       break;
   case GGPO_EVENTCODE_DISCONNECTED_FROM_PEER:
       SetConnectState(info->u.disconnected.player, Disconnected);
       break;
   case GGPO_EVENTCODE_TIMESYNC:
       sdlSleep(1000 * info->u.timesync.frames_ahead / 60);
       break;
   }

   return true;
}

bool __cdecl ds_log_game_state_callback(char* filename, unsigned char* buffer, int len) {

   //Game* game = new Game;
   //if (game) {
   //    game->boards = vectorCreate<Board*>(4, 10);
   //    if (game->boards) {
   //        gameLoad(game, buffer);

   //        FILE* log = nullptr;
   //        fopen_s(&log, filename, "w");
   //        if (log) {
   //            fprintf(log, "Current Game State\n");
   //            fprintf(log, "Players: %d\n", game->players);
   //            fprintf(log, "Game timer: %d\n", game->timer);
   //            fprintf(log, "Seed: %d\n", game->seed);
   //            fprintf(log, "Boards: %d\n", vectorSize(game->boards));
   //            for (int i = 1; i <= vectorSize(game->boards); i++) {
   //                Board* board = vectorGet(game->boards, i);
   //                fprintf(log, "Player: %d\n", board->player);
   //                fprintf(log, "RandomCalls: %d\n", board->randomCalls);
   //            }
   //        }

   //        fclose(log);

   //        for (int i = 1; i <= vectorSize(game->boards); i++) {
   //            Board* board = vectorGet(game->boards, i);
   //            boardDestroy(board);
   //        }
   //        vectorDestroy(game->boards);
   //    }
   //}
   //delete game;
   return true;
}

void ggpoInitPlayer(int playerCount, int pNumber, unsigned short localport, int remoteport) {

   GGPOErrorCode result;

  //Called at startup to setup GGPO session
   GGPOSessionCallbacks cb;
   cb.begin_game = ds_begin_game_callback;
   cb.advance_frame = ds_advance_frame_callback;  //todo not done
   cb.load_game_state = ds_load_game_callback;
   cb.save_game_state = ds_save_game_callback;
   cb.free_buffer = ds_free_buffer_callback;
   cb.on_event = ds_on_event_callback;
   cb.log_game_state = ds_log_game_state_callback;

   //Can add sync test here
   //char name[] = "Drop and Swap";
   //result = ggpo_start_synctest(&game->net->ggpo, &cb, name, 2, sizeof(UserInput), 1);

   result = ggpo_start_session(&game->net->ggpo, &cb, "Dropswap", playerCount, sizeof(UserInput), localport);
   //result = ggpo_start_session(&game->net->ggpo, &cb, "Dropswap", playerCount, sizeof(int), localport);

   // Disconnect clients after 5000 ms and start our count-down timer for disconnects after 1000 ms
   ggpo_set_disconnect_timeout(game->net->ggpo, 3000);
   ggpo_set_disconnect_notify_start(game->net->ggpo, 1000);

   //todo don't do this the dumb way
   int p1index = 0;
   int p2index = 1;
   if (pNumber == 2) {
      p1index = 1;
      p2index = 0;
   }

   game->net->players[p1index].type = GGPO_PLAYERTYPE_LOCAL;
   game->net->players[p1index].size = sizeof(GGPOPlayer);
   game->net->players[p1index].player_num = 1;

   game->net->players[p2index].type = GGPO_PLAYERTYPE_REMOTE;
   game->net->players[p2index].size = sizeof(GGPOPlayer);
   game->net->players[p2index].player_num = 2;

   const char* ip = "127.0.0.1";
   strcpy(game->net->players[p2index].u.remote.ip_address, ip);

   game->net->players[p2index].u.remote.port = remoteport;

   //loop to add Players
   for (int i = 0; i < playerCount; i++) {
      GGPOPlayerHandle handle;
      result = ggpo_add_player(game->net->ggpo, &game->net->players[i], &handle);
      game->net->connections[i].handle = handle;
      game->net->connections[i].type = game->net->players->type;
      if (game->net->players[i].type == GGPO_PLAYERTYPE_LOCAL) {
         game->net->localPlayer = handle;
         ggpo_set_frame_delay(game->net->ggpo, handle, GAME_FRAME_DELAY);
      }

   }

}

void ggpoInitSpectator() {

   //result = ggpo_start_spectating...
}

void gameAdvanceFrame(Game* game) {
   gameUpdate(game);  //todo come back and make this work

   //Tell GGPO we moved ahead a frame
   ggpo_advance_frame(game->net->ggpo);
}

void gameRunFrame() {

   GGPOErrorCode result = GGPO_OK;
   int disconnect_flags;

   //read local inputs
   if (game->net->localPlayer != GGPO_INVALID_HANDLE) {
      inputProcessKeyboard(game->net->game);
   }

   //debug
   int input = 0;
   int inputs[GAME_PLAYERS];
   //Can do sync test here

   result = ggpo_add_local_input(game->net->ggpo, game->net->localPlayer, &game->net->game->p1Input, sizeof(UserInput));
   //result = ggpo_add_local_input(game->net->ggpo, game->net->localPlayer, &input, sizeof(int));
   //If we got the local inputs successfully, merge in remote ones
   if (GGPO_SUCCEEDED(result)) {
      result = ggpo_synchronize_input(game->net->ggpo, (void*)inputs, sizeof(int) * GAME_PLAYERS, &disconnect_flags);
      if (GGPO_SUCCEEDED(result)) {
         gameAdvanceFrame(game->net->game);  //Update the game 
      }
   }

}
