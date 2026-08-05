#include "view_players.h"
#include "gui.h"
#include "view_common.h"

#include "sprite.h"
#include "util.h"

#include "drawtext.h"

int playerFileSave(LoadedFileInfo* fileinfo, const char* path) 
{
	PlayerEditState* state = fileinfo->userdata;

	if(state && state->playerFile == fileinfo) {
		if(strncmp(path, fileinfo->filename, 4096) != 0) {
			state->playerFile->filename = path; 
			state->playerFile->backupFile = nullptr;
			state->gotFilename = true;
		}
	}

	if(!fileinfo->backupFile) {
		const char* backup = getBackupPath(fileinfo->filename, "players");
		fileinfo->backupFile = backup;
	}
	SDL_Log("saving %s", fileinfo->filename);
	SDL_CopyFile(fileinfo->filename, fileinfo->backupFile);
	fileinfo->wasEdited = false;
	return playerdbSave(state->playerdb, fileinfo->filename);
}

int playerFileLoad(LoadedFileInfo* fileinfo, const char* path) 
{
	PlayerEditState* state = fileinfo->userdata;
	if(state) {
		if(state->playerFile == fileinfo) {
			fileinfo->filename = path;
			state->gotFilename = true;
		}

		SDL_Log("loading %s", path);
		int ret = playerdbLoad(state->playerdb, path);
		state->playersDirty = true;
		Game->needUpdate = 3;
		return ret;
	} else {
		return 0;
	}
}

void setupPlayerFile(PlayerEditState* state, LoadedFileInfo* fileinfo, const char* filename)
{
	fileinfo->userdata = state;

	fileinfo->save = playerFileSave;
	fileinfo->load = playerFileLoad;
	fileinfo->extension = "wdplayers";
	if(fileinfo->filename) {
		//playerFileLoad(fileinfo, fileinfo->filename);
		fileinfoLoad(fileinfo);
	} else {
		fileinfo->filename = filename;
	}
}

void playerState_create(GameState* base, GameContext* game)
{
	PlayerEditState* state = (void*)base;
	state->playerFile = &globalFileContext->players;

	state->data = globalMonData;
	state->playerdb = state->data->playerdb;
	setupPlayerFile(state, state->playerFile, "Unknown.wdplayers");
	if(state->playerdb->numPlayers == 0) {
		state->playerdb->numPlayers = 10;
		for(int i = 0; i < state->playerdb->maxPlayers; ++i) {
			state->playerdb->players[i].draftOrder = i;
		}
	}
	state->trx = createTrayContext(4, state->playerdb->maxPlayers);
	state->playerTabs = calloc(state->playerdb->maxPlayers, sizeof(PlayerTabUi));
	GuiTray* teamTray = createTray(
		state->trx, 
		"Players", 
		(int2){1, state->playerdb->numPlayers},
		(float2){32+640+96, 24+24+12+8});
	state->teamTray = teamTray;
	state->trx->panners[0].enabled = true;
	state->trx->panners[0].panX = false;
	state->trx->panners[0].panY = true;
	teamTray->userdata = state;
	teamTray->flags |= Tray_HideHeader | Tray_HideBg | Tray_ReqExclusionBox;

	PlayerTabUi* pt = state->playerTabs;
	PlayerDatabase* db = state->playerdb;
	for(int i = 0; i < state->trx->maxTiles; ++i) {
		GuiTile* tile = &state->trx->tiles[i];
		memcpy(&pt[i].team, &db->players[i], sizeof(PlayerTeam));
		gui_init_textbox(&pt[i].ownerbox, pt[i].team.owner.name, SDL_arraysize(pt[i].team.owner.name));
		gui_init_textbox(&pt[i].titlebox, pt[i].team.title.name, SDL_arraysize(pt[i].team.title.name));
		pt[i].order = i;
		pt[i].index = i;
		playerTile_init(tile, i, &pt[i]);
		trayAdd(teamTray, tile, -1);
	}


	//state->montrx = createTrayContext(state->playerdb->maxPlayers, state->data->numMons + 256);
	//for(int i = 0; )

	state->playersDirty = true;
}

void playerState_updateTrayFromDatabase(PlayerEditState* state)
{
	PlayerDatabase* playerdb = state->playerdb;
	state->playersDirty = false;
	PlayerTabUi* tabs = state->playerTabs;
	for(int i = 0; i < playerdb->numPlayers; ++i) {
		tabs[i].index = i;
		tabs[i].order = i;
		memcpy(&tabs[i].team, &playerdb->players[i], sizeof(PlayerTeam));
		tabs[i].team.draftOrder = i;
		tabs[i].ownerbox.numChars = SDL_strlen(tabs[i].team.owner.name);
		tabs[i].titlebox.numChars = SDL_strlen(tabs[i].team.title.name);

		GuiTile* tile = &state->trx->tiles[i];
		state->teamTray->slots[i] = tile;
		tile->userdata = &tabs[i];
	}
}

void playerState_start(GameState* base, GameContext* game)
{
	playerState_updateTrayFromDatabase((void*)base);
}

void playerState_updateDatabaseFromTray(PlayerEditState* state)
{
	PlayerDatabase* playerdb = state->playerdb;
	for(int i = 0; i < state->teamTray->numSlots; ++i) {
		GuiTile* tile = state->teamTray->slots[i];
		PlayerTabUi* tab = tile->userdata;

		tab->order = i;
		tab->index = i;
		tab->team.draftOrder = i;
		memcpy(&playerdb->players[i], &tab->team, sizeof(PlayerTeam));
	}

}

void playerState_update(GameState* base, GameContext* game)
{
	PlayerEditState* state = (void*)base;
	gui_update(Gui);
	Gui->pen = 0;
	common_setHeaderPen(2);

	float headerY = Gui->pen.y;

	PlayerDatabase* playerdb = state->playerdb;

	state->teamTray->pos = Gui->pen;
	state->teamTray->numSlots = playerdb->numPlayers - ((state->trx->grabbedTile ? 1 : 0));
	state->teamTray->gridSize.y = playerdb->numPlayers;
	state->teamTray->flags |= Tray_RegionDirty;

	if(keyJustDown(SDL_SCANCODE_TAB)) {
		if(Gui->focusedTextbox) {
			state->tabIndex = (state->tabIndex + 1) % (playerdb->numPlayers);
		}
		int box = state->tabIndex;

		GuiTile* tile = state->teamTray->slots[box];
		PlayerTabUi* tab = tile->userdata;
		Gui->focusedTextbox = &tab->ownerbox;
		SDL_StartTextInput(Game->window);
	}

	if(state->playersDirty) {
		playerState_updateTrayFromDatabase(state);
	}

	trayDraw(state->teamTray);

	// draw header (on top of boxes)

	float4 headerRegion;
	headerRegion.xy = -1;
	headerRegion.z = Game->windowSize.x + 2;
	headerRegion.w = headerY + 1;
	drawFloat4(0, headerRegion);
	state->teamTray->exclusionBox = headerRegion;

	common_drawTabs();
	bool wasChanged = false;

	uiHbox();
	if(uiButton(0, "-")) {
		playerdb->numPlayers--;
		wasChanged = true;
		if(playerdb->numPlayers < 1) {
			playerdb->numPlayers = 1;
		}
	}

	uiLabelFmt(Gui_Highlighted, Align_Center, "%2d Players", playerdb->numPlayers);
	if(uiButton(0, "+")) {
		wasChanged = true;
		// TODO maybe change how trays handle being resized 
		// this is a little messy
		state->teamTray->slots[playerdb->numPlayers] = &state->trx->tiles[playerdb->numPlayers];
		playerdb->numPlayers++;
		if(playerdb->numPlayers >= playerdb->maxPlayers-1) {
			playerdb->numPlayers = playerdb->maxPlayers-1;
		}
	}

	#ifndef __EMSCRIPTEN__
	if(uiButton(0, "Save")) {
		playerState_updateDatabaseFromTray(state);
		if(!state->gotFilename) {
			openFileDialog(FileDialog_Save, state->playerFile, 0);
		} else {
			fileinfoSave(state->playerFile);
		}
	}
	if(uiButton(0, "Save As")) {
		playerState_updateDatabaseFromTray(state);
		openFileDialog(FileDialog_Save, state->playerFile, 0);
	}
	if(uiButton(0, "Load")) {
		openFileDialog(FileDialog_Load, state->playerFile, 0);
	}
	#endif

	uiSpacer(32);
	if(uiButton(0, "Shuffle Teams")) {
		wasChanged = true;
		rng_state rng;
		init_rng(&rng, SDL_GetTicks());
		//PlayerTabUi* pt = state->playerTabs;
		PlayerDatabase* db = state->playerdb;
		for(int iters = 0; iters < 4; ++iters) {
			for(int i = 0; i < db->numPlayers; ++i) {
				int swap = i32abs(i32rand(&rng)) % db->numPlayers;
				if(i == swap && swap > db->maxPlayers) {
					i--;
					continue;
				}

				GuiTile* at = state->teamTray->slots[swap];
				state->teamTray->slots[swap] = state->teamTray->slots[i];
				state->teamTray->slots[i] = at;
			}
		}

	}

	if(state->resetMode == 1 && uiButton(0, "Are you sure?")) {
		state->resetMode = 0;
		memset(state->playerdb->players, 0, sizeof(PlayerTeam) * state->playerdb->maxPlayers);
		for(int i = 0; i < state->playerdb->maxPlayers; ++i) {
			memset(&state->playerTabs[i].team, 0, sizeof(PlayerTeam));
			state->playerTabs[i].ownerbox.numChars = 0;
			state->playerTabs[i].titlebox.numChars = 0;
		}
		state->playerdb->numPlayers = 1;
		state->playerFile->filename = "Untitled.wdplayers";
		state->playerFile->backupFile = nullptr;
		state->gotFilename = false;
		state->playersDirty = true;
	}
	if(state->resetMode == 0 && uiButton(0, "Reset Players")) {
		state->resetMode = 1;
	}

	if(state->gotFilename) {
		uiSpacer(32);
		uiLabelFmt(0, Align_Center, "Editing %s", state->playerFile->filename);
	}

	uiPop();

	wasChanged |= trayContextUpdate(state->trx);
	if(wasChanged) {
		playerState_updateDatabaseFromTray(state);
	}

}

void playerState_stop(GameState* base, GameContext* game)
{
	playerState_updateDatabaseFromTray((PlayerEditState*)base);
}
