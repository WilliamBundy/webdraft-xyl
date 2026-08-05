#pragma once
#include "wb_sdlgame.h"
#include "gui_tray.h"
#include "pokemon.h"
#include "gui.h"
#include "filecontext.h"

#define PLAYEREDIT_NAME "PlayerEditState"
#define PLAYEREDIT_TITLE "Players"
#define PLAYEREDIT_4CC WB_4CC("PLVW")

typedef struct PlayerEditState PlayerEditState;
typedef struct PlayerEdit_FileDialogUserdata
{
	int mode, flags;
	const char* extension;
	PlayerEditState* state;
} PlayerEdit_FileDialogUserdata;

typedef struct PlayerTabUi
{
	PlayerTeam team;
	GuiTextbox titlebox, ownerbox;
	int numPicks, toRemove;
	int order, index;
} PlayerTabUi;

typedef struct PlayerEditState
{
	GameState base;

	MonData* data;
	PlayerDatabase* playerdb;
	GuiTrayContext* trx;
	GuiTray* teamTray;
	int tabIndex, reserved;
	LoadedFileInfo* playerFile;

	PlayerTabUi* playerTabs;
	bool gotFilename;
	bool playersDirty;

	PlayerEdit_FileDialogUserdata fdu;

	int resetMode;

} PlayerEditState;

void playerState_create(GameState* base, GameContext* game);
void playerState_start(GameState* base, GameContext* game);
void playerState_update(GameState* base, GameContext* game);
void playerState_stop(GameState* base, GameContext* game);

static inline 
void playerState_register()
{
	GameState* base = gamestateCreate(
		PLAYEREDIT_NAME, 
		PLAYEREDIT_4CC,
		sizeof(PlayerEditState));
	gamestateSetProcs(base, 
		playerState_create, 
		playerState_start, 
		playerState_update, 
		nullptr, 
		nullptr, 
		playerState_stop);
	gameRegister(Game, base);
}
