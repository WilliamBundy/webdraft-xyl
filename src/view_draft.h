#pragma once
#include "wb_sdlgame.h"
#include "pokemon.h"

#include "filecontext.h"

#include "gui_tray.h"

#define DRAFTVIEW_NAME "DraftBoardState"
#define DRAFTVIEW_TITLE "Draft"
#define DRAFTVIEW_4CC WB_4CC("DRFV")

typedef struct DraftBoardState
{
	GameState base;

	MonData* monData;
	PlayerDatabase* playerdb;
	MonPointDatabase* pointdb;
	DraftBoard* board;
	LoadedFileInfo* draftFile;

	GuiTrayContext* trx;

	GuiTray** pointTrays, **playerTrays;
	GuiTray* randomTray;

	float playerBoxWidth;
	int lastMove;
	

	bool draftStarted;
	bool gotFilename;
	bool draftDirty;
	bool showRandomControls;
	bool smallTeams;
	int resetMode;
} DraftBoardState;

void draftState_create(GameState* base, GameContext* game);
void draftState_start(GameState* base, GameContext* game);
void draftState_update(GameState* base, GameContext* game);
void draftState_stop(GameState* base, GameContext* game);

static inline 
void draftState_register()
{
	GameState* base = gamestateCreate(
		DRAFTVIEW_NAME, 
		DRAFTVIEW_4CC,
		sizeof(DraftBoardState));
	gamestateSetProcs(base, 
		draftState_create, 
		draftState_start, 
		draftState_update, 
		nullptr, 
		nullptr, 
		draftState_stop);
	gameRegister(Game, base);
}