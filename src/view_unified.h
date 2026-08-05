#pragma once
#include "wb_sdlgame.h"
#include "pokemon.h"
#include "filecontext.h"
#include "gui_tray.h"


#define UNIFIEDVIEW_NAME "UnifiedDraftState"
#define UNIFIEDVIEW_TITLE "Unified"
#define UNIFIEDVIEW_4CC WB_4CC("UNIF")

typedef struct UnifiedDraftState
{
	GameState base;

} UnifiedDraftState;

void unifiedState_create(GameState* base, GameContext* game);
void unifiedState_start(GameState* base, GameContext* game);
void unifiedState_update(GameState* base, GameContext* game);
void unifiedState_stop(GameState* base, GameContext* game);

static inline 
void unifiedState_register()
{
	GameState* base = gamestateCreate(
		UNIFIEDVIEW_NAME, 
		UNIFIEDVIEW_4CC,
		sizeof(UnifiedDraftState));
	gamestateSetProcs(base, 
		unifiedState_create, 
		unifiedState_start, 
		unifiedState_update, 
		nullptr, 
		nullptr, 
		unifiedState_stop);
	gameRegister(Game, base);
}
