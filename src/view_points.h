#pragma once
#include "wb_sdlgame.h"
#include "pokemon.h"
#include "filecontext.h"

#include "gui_tray.h"

#define POINTVIEW_NAME "PointEditorState"
#define POINTVIEW_4CC WB_4CC("PTVW")
#define POINTVIEW_TITLE "Points"

typedef struct PointEditorState PointEditorState;
typedef struct PointState_FileDialogueUserdata
{
	PointEditorState* state;
	int mode, kind;
	const char* extension;
} PointState_FileDialogueUserdata;

typedef struct PointEditorState
{
	GameState base;
	MonData* monData;
	MonPointDatabase* pointdb;
	LoadedFileInfo* pointsfile;
	PointState_FileDialogueUserdata fdu;

	//const char* filename;
	//int stopCount, startCount;

	GuiTrayContext* trx;
	int sourceTrayIndex;
	int pointsDirty;
	int gotFilename;
	int resetMode;
	int lastMove;
} PointEditorState;

void pointState_create(GameState* base, GameContext* game);
void pointState_start(GameState* base, GameContext* game);
void pointState_update(GameState* base, GameContext* game);
void pointState_stop(GameState* base, GameContext* game);

static inline
void pointState_register()
{
	GameState* base = gamestateCreate(
		POINTVIEW_NAME, 
		POINTVIEW_4CC,
		sizeof(PointEditorState));
	gamestateSetProcs(base, 
		pointState_create, 
		pointState_start, 
		pointState_update, 
		nullptr, 
		nullptr, 
		pointState_stop);

	gameRegister(Game, base);
}
