#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include "wb_gamemath.h"

#define WB_SDLGAME_IMPLEMENTATION
#include "wb_sdlgame.h"

#define WB_SPRITEFONT_IMPLEMENTATION
#include "wb_spritefont.h"

#include "wb_dict32.h"

#include "sprite.h"
#include "sprite.c"


#include "util.h"
#include "drawtext.h"
wbsf_Spritefont globalDefaultFont;
#include "gui.h"
#include "pokemon.h"
#include "gui_tray.h"

#include "filecontext.h"

#include "view_common.h"
#include "view_points.h"
#include "view_players.h"

#include "gui.c"
#include "pokemon.c"
#include "filecontext.c"


#include "gui_tray.c"
#include "view_common.c"
#include "view_points.c"
#include "view_players.c"
#include "view_draft.c"


uint8_t graphicsPng[] = {
	#embed "assets/graphics.png"
};

GuiContext guictx;
GuiContext* Gui = &guictx;

//MonData* monData;
MonData* globalMonData;

GuiTrayContext* trx;
int tileDraw(float2 pos, GuiTray* tray, GuiTile* tile)
{
	Xform camera = Xzero; //tray ? tray->trx->trayCamera : Xzero;
	SDL_FRect src = getMonRect(tile->id - 1);
	SDL_FRect dst = pXformRect(pos, (float2){src.w, src.h}, camera);
	SDL_RenderTexture(Game->renderer, monTexture, &src, &dst);
	return 0;
}

void testCreate(GameState* state, GameContext* game)
{
	SDL_Log("test created");
	MonData* monData = globalMonData;
	trx = createTrayContext(256, 2048);
	for(int i = 0; i < monData->numMons; ++i) {
		trx->tiles[i].title = monData->mons[i].name; 
		trx->tiles[i].id = monData->mons[i].number;
		trx->tiles[i].grabRegion = (float4){0, 0, 64, 64};
		trx->tiles[i].draw = tileDraw;
	}

	float2 slotSize = {64, 64};
	createTray(trx, "Picks", (int2){3, 8}, slotSize);
	createTray(trx, "Pool", (int2){3, 8}, slotSize);
	createTray(trx, "Bans", (int2){3, 8}, slotSize);

	int q = 0;
	for(int i = 0; i < trx->numTrays; ++i) {
		trx->trays[i]->flags |= Tray_ShowOverlays | Tray_ShowGutter;
		for(int j = 0; j < 10; ++j) {
			trayAdd(trx->trays[i], &trx->tiles[q++], -1);
		}
	}
}

void testStart(GameState* state, GameContext* game)
{
}

int needUpdate = 2;
void testUpdate(GameState* state, GameContext* game)
{
	gui_update(Gui);
	common_drawTabs();


	float xoff = 64;
	for(int i = 0; i < trx->numTrays; ++i) {
		//trx->trays[i]->pos = (float2){xoff, 100};
		traySetPosXY(trx->trays[i], xoff, 100);
		trayDraw(trx->trays[i]);
		xoff += trayRegion(trx->trays[i]).z + 32;
	}

	trayContextUpdate(trx);



	/*
	uiButton(0, "Hello, World!");
	const char* message = "Hello, World!";
	uiLabelFmt(0, Align_Left, "%f - %f", wbsf_stringWidth(message, strlen(message), &globalDefaultFont, 1.0), 
		sizeText(message, -1, nullptr).x);
	static GuiTextbox tb;
	static char buffer[256];
	tb.buffer = buffer;
	tb.maxChars = 255;
	uiTextbox(0, 256, &tb);
	*/

}

char defaultElPoints[] = {
	0
};

void globalInit()
{
	Game->surface = SDL_LoadPNG_IO(SDL_IOFromMem(graphicsPng, SDL_arraysize(graphicsPng)), true);
	Game->texture = SDL_CreateTextureFromSurface(Game->renderer, Game->surface);
	SDL_SetTextureBlendMode(Game->texture, SDL_BLENDMODE_BLEND);
	setupDefaultFont();

	globalMonData = calloc(1, sizeof(MonData));
	pokemon_init(globalMonData);
	//pointdbImportCSVFromText(globalMonData, globalMonData->pointdb, defaultElPoints, sizeof(defaultElPoints));
	gui_init(Gui);


	int totals[40] = {0};
	for(int i = 0; i < globalMonData->numMons; ++i) {
		totals[globalMonData->pointdb->pointCosts[i]]++;
	}

	int sum = 0;
	for(int i = 0; i < 20; ++i){
		printf("point cost %d: %d\n", i, totals[i]);
		if(i > 1) {
			sum += totals[i];
		}
	}

	printf("%d usable mons\n", sum);

	setupGlobalFileContext();
}

void statesInit()
{
	pointState_register();
	playerState_register();
	draftState_register();

	GameState* testState = gamestateCreate("Test", 0, sizeof(GameState));
	gamestateSetProcs(testState, testCreate, testStart, testUpdate, nullptr, nullptr, nullptr);
	gameRegister(Game, testState);



}


SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
	SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "waitevent");
	bool ret = SDL_Init(SDL_INIT_VIDEO);
	if(!ret) {
		SDL_Log("Error: could not initialize SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	GameSettings settings = {1280, 720, "webdraft", false, false};
	Game = gameCreate(settings);
	SDL_SetDefaultTextureScaleMode(Game->renderer, SDL_SCALEMODE_NEAREST);
	globalInit();
	statesInit();
	// TODO stupid hack because we have bad coupling between data and view

	gameStart(Game, PLAYEREDIT_NAME);
	gameStart(Game, POINTVIEW_NAME);
	gameStart(Game, DRAFTVIEW_NAME);
	Game->needUpdate = 3;

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	gamePreUpdate(Game);
	gameUpdate(Game);
	gamePostUpdate(Game);
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	if(event->type == SDL_EVENT_QUIT) {
		int ret = checkOpenedFilesOnClose(globalFileContext);
		if(ret == SDL_APP_CONTINUE) {
			return SDL_APP_CONTINUE;
		}
	}

	if(event->type == SDL_EVENT_KEY_DOWN) gui_handle_key_down(*event, Gui);
	if(event->type == SDL_EVENT_TEXT_INPUT) gui_handle_text_input(*event, Gui);
	return gameHandleEvent(Game, event);
}

typedef struct StatTotals
{
	int stats[6];
} StatTotals;

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	(void)appstate;
	(void)result;
	// TODO write backups to prefs path
}
