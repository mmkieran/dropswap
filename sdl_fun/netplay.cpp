
#include "netplay.h"

extern Game* game;  //I dunno how I feel about this

int fletcher32_checksum(short* data, size_t len) {
   int sum1 = 0xffff, sum2 = 0xffff;

   while (len) {
      size_t tlen = len > 360 ? 360 : len;
      len -= tlen;
      do {
         sum1 += *data++;
         sum2 += sum1;
      } while (--tlen);
      sum1 = (sum1 & 0xffff) + (sum1 >> 16);
      sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   }

   /* Second reduction step to reduce sums to 16 bits */
   sum1 = (sum1 & 0xffff) + (sum1 >> 16);
   sum2 = (sum2 & 0xffff) + (sum2 >> 16);
   return sum2 << 16 | sum1;
}

int ggpoCheckSum(Game* game) {
   std::vector <Byte> stream = gameSave(game);
   int len = stream.size();
   if (len == 0) { return 0; }
   int checksum = fletcher32_checksum((short*)stream.data(), len / 2);
   return checksum;
}

void SetConnectState(GGPOPlayerHandle handle, PlayerConnectState state) {
   for (int i = 0; i < GAME_MAX_PLAYERS; i++) {
      if (game->net->connections[i].handle == handle) {
         game->net->connections[i].connect_progress = 0;
         game->net->connections[i].state = state;
         break;
      }
   }
}

void SetDisconnectTimeout(GGPOPlayerHandle handle, int when, int timeout) {
   for (int i = 0; i < GAME_MAX_PLAYERS; i++) {
      if (game->net->connections[i].handle == handle) {
         game->net->connections[i].disconnect_start = when;
         game->net->connections[i].disconnect_timeout = timeout;
         game->net->connections[i].state = Disconnecting;
         break;
      }
   }
}

void SetConnectState(PlayerConnectState state) {
   for (int i = 0; i < GAME_MAX_PLAYERS; i++) {
      game->net->connections[i].state = state;
   }
}

void UpdateConnectProgress(GGPOPlayerHandle handle, int progress) {
   for (int i = 0; i < GAME_MAX_PLAYERS; i++) {
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

   //Figure out the inputs and check for disconnects
   ggpo_synchronize_input(game->net->ggpo, (void*)game->inputs, sizeof(UserInput) * GAME_PLAYERS, &disconnect_flags);

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

   *buffer = (unsigned char*) malloc(*len);
   if (buffer) {
      memcpy(*buffer, stream.data(), *len);
      *checksum = fletcher32_checksum((short*)*buffer, *len / 2);

      return true;
   }
   else { return false; }

}

void __cdecl ds_free_buffer_callback(void* buffer) {
   if (buffer) { 
      free(buffer); 
   }
}

bool __cdecl ds_on_event_callback(GGPOEvent* info) {
   int progress;
   GGPOErrorCode result = GGPO_OK;
   int disconnect_flags = 0;

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
       popupEnable(Popup_Disconnect);
       break;
   case GGPO_EVENTCODE_CONNECTION_RESUMED:
       SetConnectState(info->u.connection_resumed.player, Running);
       popupDisable(Popup_Disconnect);
       game->paused = false;
       break;
   case GGPO_EVENTCODE_DISCONNECTED_FROM_PEER:
       SetConnectState(info->u.disconnected.player, Disconnected);
       //popupEnable(Popup_Disconnect);
       break;
   case GGPO_EVENTCODE_TIMESYNC:
       //popupEnable(Popup_Waiting);
       sdlSleep(1000 * info->u.timesync.frames_ahead / 60);
       break;
   }
   int a = 0;

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

void ggpoCreateSession(Game* game, SessionInfo connects[], unsigned short participants) {
   GGPOErrorCode result;

   //Called at startup to setup GGPO session
   GGPOSessionCallbacks cb;
   cb.begin_game = ds_begin_game_callback;
   cb.advance_frame = ds_advance_frame_callback;  
   cb.load_game_state = ds_load_game_callback;
   cb.save_game_state = ds_save_game_callback;
   cb.free_buffer = ds_free_buffer_callback;
   cb.on_event = ds_on_event_callback;  
   cb.log_game_state = ds_log_game_state_callback;  //This is turned off right now

   int sessionPort = 7001;
   int hostNumber = -1;
   int myNumber = -1;
   int spectators = 0;
   for (int i = 0; i < participants; i++) {
      if (connects[i].host == true) { hostNumber = i; }  //Who is hosting
      if (connects[i].me == true) { myNumber = i; }  //who am i? who am i?
      if (connects[i].playerType == GGPO_PLAYERTYPE_SPECTATOR) { spectators++; }
   }

   game->net->myConnNum = myNumber;
   game->net->hostConnNum = hostNumber;

   sessionPort = connects[myNumber].localPort;  //Start the session using my port

   if (game->syncTest == true) {  //Set syncTest to true to do a single player sync test
      char name[] = "DropAndSwap";
      result = ggpo_start_synctest(&game->net->ggpo, &cb, name, GAME_PLAYERS, sizeof(UserInput), 1);
   }
   else if (connects[myNumber].playerType == GGPO_PLAYERTYPE_SPECTATOR){  //Spectating a GGPO Session
      result = ggpo_start_spectating(&game->net->ggpo, &cb, "DropAndSwap", participants - spectators, sizeof(UserInput), sessionPort, connects[hostNumber].ipAddress, connects[hostNumber].localPort);
      return;  //And we're done
   }
   else {  //Start a regular GGPO Session
      result = ggpo_start_session(&game->net->ggpo, &cb, "DropAndSwap", participants - spectators, sizeof(UserInput), sessionPort);
   }

   // Disconnect clients after 3000 ms and start our count-down timer for disconnects after 1000 ms
   ggpo_set_disconnect_timeout(game->net->ggpo, 30000);  //debug no disconnect for now
   ggpo_set_disconnect_notify_start(game->net->ggpo, 1000);

   if (hostNumber == myNumber) {  //I'm hosting and playing
      game->net->host = true;
      for (int i = 0; i < participants; i++) {  //Fill in GGPOPlayer struct
         if (i == hostNumber) { game->net->players[i].player_num = 1; }
         else if (connects[i].playerType == GGPO_PLAYERTYPE_REMOTE) { game->net->players[i].player_num = 2; }
         if (connects[i].playerType != GGPO_PLAYERTYPE_SPECTATOR) {  //Spectators don't have input size or player number
            game->net->players[i].size = sizeof(GGPOPlayer);
         }
         game->net->players[i].type = (GGPOPlayerType)connects[i].playerType;

         if (i != hostNumber) {  //Everybody else is remote
            strcpy(game->net->players[i].u.remote.ip_address, connects[i].ipAddress);
            game->net->players[i].u.remote.port = connects[i].localPort;
         }

         GGPOPlayerHandle handle;
         result = ggpo_add_player(game->net->ggpo, &game->net->players[i], &handle);  //Add a player to GGPO session
         if (result != GGPO_OK) { printf("Couldn't add player %d", i); }

         game->net->connections[i].handle = handle;
         game->net->connections[i].type = game->net->players[i].type;

         if (game->net->players[i].type == GGPO_PLAYERTYPE_LOCAL) {
            game->net->localPlayer = handle;
            ggpo_set_frame_delay(game->net->ggpo, handle, GAME_FRAME_DELAY);
         }
      }
   }

   else if (connects[myNumber].playerType == GGPO_PLAYERTYPE_LOCAL) {  //I'm only playing
      //Add host to session
      game->net->players[hostNumber].player_num = 1;
      game->net->players[hostNumber].size = sizeof(GGPOPlayer);
      game->net->players[hostNumber].type = (GGPOPlayerType)connects[hostNumber].playerType;

      strcpy(game->net->players[hostNumber].u.remote.ip_address, connects[hostNumber].ipAddress);
      game->net->players[hostNumber].u.remote.port = connects[hostNumber].localPort;

      GGPOPlayerHandle handle1;
      result = ggpo_add_player(game->net->ggpo, &game->net->players[hostNumber], &handle1);  //Add the host
      if (result != GGPO_OK) { printf("Couldn't add player %d", hostNumber); }

      game->net->connections[hostNumber].handle = handle1;
      game->net->connections[hostNumber].type = game->net->players[hostNumber].type;

      //Add me to the session
      game->net->host = false;
      game->net->players[myNumber].player_num = 2;
      game->net->players[myNumber].size = sizeof(GGPOPlayer);
      game->net->players[myNumber].type = (GGPOPlayerType)connects[myNumber].playerType;

      GGPOPlayerHandle handle2;
      result = ggpo_add_player(game->net->ggpo, &game->net->players[myNumber], &handle2);  //Add me to the session
      if (result != GGPO_OK) { printf("Couldn't add player %d", myNumber); }

      game->net->connections[myNumber].handle = handle2;
      game->net->connections[myNumber].type = game->net->players[myNumber].type;
      game->net->localPlayer = handle2;
      ggpo_set_frame_delay(game->net->ggpo, handle2, GAME_FRAME_DELAY);  //Set frame delay for the session
   }
}

void gameAdvanceFrame(Game* game) {
   for (int i = 0; i < game->boards.size(); i++) {  //Check for pauses
      gameCheckPause(game, game->inputs[i]);
   }
   gameUpdate(game); 

   ggpo_advance_frame(game->net->ggpo);  //Tell GGPO we moved ahead a frame
   game->frameCount++;
}

void gameRunFrame() {
   if (game->playing == false) { return; }
   if (game->net && game->net->ggpo) {

      GGPOErrorCode result = GGPO_OK;
      int disconnect_flags;

      if (game->net->localPlayer != GGPO_INVALID_HANDLE) {  //Add local inputs for valid players
         processInputs(game);

         if (game->syncTest == false) {
            if (game->net->localPlayer == 1) { 
               game->p1Input.timer = game->timer;
            }
         }
         result = ggpo_add_local_input(game->net->ggpo, game->net->localPlayer, &game->p1Input, sizeof(UserInput));
      }
      //If we got the local inputs successfully, merge in remote ones
      if (GGPO_SUCCEEDED(result)) {
         result = ggpo_synchronize_input(game->net->ggpo, (void*)game->inputs, sizeof(UserInput) * GAME_PLAYERS, &disconnect_flags);
         if (GGPO_SUCCEEDED(result)) {
            if (game->net->localPlayer != 1) {  
               if (game->syncTest == false) {
                  game->timer = game->inputs[0].timer;  //We want to use the timer from p1
               }
            }
            gameAdvanceFrame(game);  //Update the game 
         }
      }
   }
}

void ggpoClose(GGPOSession* ggpo) {
   if (ggpo) {
      ggpo_close_session(ggpo);
   }
}

const char* ggpoShowStatus(Game* game, int playerIndex) {
   const char* out = "";
   if (game->net) {
      switch (game->net->connections[playerIndex].state) {
      case 0:
         out = "Connecting";
         break;
      case 1:
         out = "Synchronizing";
         break;
      case 2:
         out = "Running";
         break;
      case 3:
         out = "Disconnected";
         break;
      case 4:
         out = "Disconnecting";
         break;
      default:
         out = "";
         break;
      }
   }
   return out;
}

int ggpoDisconnectPlayer(int player){
   GGPOErrorCode result = ggpo_disconnect_player(game->net->ggpo, player);
   if (GGPO_SUCCEEDED(result)) {
      return 1;
   }
   else return 0;
}

void ggpoEndSession(Game* game) {
   if (game->net) {
      for (int i = 0; i < GAME_PLAYERS; i++) {
         ggpoDisconnectPlayer(i + 1);

      }
      ggpoClose(game->net->ggpo);
      delete game->net;
      game->net = new NetPlay;
   }
}

void ggpoSendMessage(int msg) {

   GGPOErrorCode result = GGPO_OK;
   int disconnect_flags;

   game->p1Input.timer = msg;

   result = ggpo_add_local_input(game->net->ggpo, game->net->localPlayer, &game->p1Input, sizeof(UserInput));
   if (GGPO_SUCCEEDED(result)) {
      result = ggpo_synchronize_input(game->net->ggpo, (void*)game->inputs, sizeof(UserInput) * GAME_PLAYERS, &disconnect_flags);
      if (GGPO_SUCCEEDED(result)) {
         ggpo_advance_frame(game->net->ggpo);
      }
   }
}