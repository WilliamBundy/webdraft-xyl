#pragma once
#include "gui.h"

#include "view_points.h"
#include "view_draft.h"
#include "view_players.h"

enum
{
	TrayKind_Unknown,
	TrayKind_PointTier,
	TrayKind_Source,
	TrayKind_Player,
	TrayKind_DraftBoard,
	TrayKind_DraftPicks,
	TrayKind_Team
};

static inline 
void common_setHeaderPen(int rows)
{
	Gui->pen = 8;
	for(int i = 0; i < rows; ++i) {
		uiHbox();
		uiButton(0, "0");
		uiPop();
	}
}

static inline
void common_drawTabs() 
{
	Gui->pen = 8;
	uiHbox();

	// annoying but not terrible
	const char* stateTitles[] = {
		PLAYEREDIT_TITLE,
		DRAFTVIEW_TITLE,
		POINTVIEW_TITLE,
		//"Calc",
		//"Overview",
		//"Teambuild",
		//"Options",
	};

	const char* stateNames[] = {
		PLAYEREDIT_NAME,
		DRAFTVIEW_NAME,
		POINTVIEW_NAME,
		//"Test",
		//"Test",
		//"Test",
		//"Test"
	};

	uint32_t state4cc[] = {
		PLAYEREDIT_4CC,
		DRAFTVIEW_4CC,
		POINTVIEW_4CC,
		//0,
		//0,
		//0,
	//	0
	};

	for(int i = 0; i < SDL_arraysize(state4cc); ++i) {
		int flags = Gui_Button_NoElevation;
		flags |= Game->state->id == state4cc[i] ? Gui_Highlighted : 0;
		if(uiButton(flags, stateTitles[i])) {
			gameSwitch(Game, stateNames[i]);
		}
	}
	uiPop();
}

int monTile_draw(float2 pos, GuiTray* tray, GuiTile* tile);
int monTile_add(float2 pos, GuiTray* tray, GuiTile* tile);
int monTile_canDrop(float2 pos, GuiTray* tray, GuiTile* tile);
static inline
void monTile_init(GuiTile* tile, MonData* data, int dexNumber)
{
	tile->id = dexNumber;
	tile->grabRegion.zw = 64;
	tile->title = data->mons[dexNumber-1].name;
	tile->draw = monTile_draw;
	tile->add = monTile_add;
	tile->canDrop = monTile_canDrop;
}


int playerTile_draw(float2 pos, GuiTray* tray, GuiTile* tile);
static inline
void playerTile_init(GuiTile* tile, int id, PlayerTabUi* ptui)
{
	tile->id = id;
	tile->grabRegion = (float4){0, 0, 32, 24+24+12};
	tile->draw = playerTile_draw;
	char* title = calloc(32, 1);
	SDL_snprintf(title, 32, "Team %d", id + 1);
	tile->title = title;
	tile->userdata = ptui;
}

void drawStatBlock(GuiContext* gui, MonDef* mon, float barWidth, int pts);
