#include "view_points.h"
#include "gui.h"
#include "pokemon.h"

#include "view_common.h"
#include "util.h"

#include "gui_tray.h"
#include "drawtext.h"


int pointFileSave(LoadedFileInfo* fileinfo, const char* path)
{
	#ifndef __EMSCRIPTEN__
	PointEditorState* state = fileinfo->userdata;
	if(state && state->pointsfile == fileinfo) {
		if(strncmp(path, fileinfo->filename, 4096) != 0) {
			state->pointsfile->filename = path; 
			state->pointsfile->backupFile = nullptr;
			state->gotFilename = true;
		}
	}

	if(!fileinfo->backupFile) {
		const char* backup = getBackupPath(fileinfo->filename, "points");
		fileinfo->backupFile = backup;
	}
	//SDL_Log("backup file: %s", fileinfo->backupFile);
	SDL_CopyFile(fileinfo->filename, fileinfo->backupFile);
	fileinfo->wasEdited = false;
	return pointdbSave(state->monData, state->pointdb, fileinfo->filename);
	#else
	return 0;
	#endif
}

int pointFileExportCsv(LoadedFileInfo* fileinfo, const char* path)
{
	PointEditorState* state = fileinfo->userdata;
	if(state) {
		SDL_IOStream* stream = SDL_IOFromFile(path, "wb");
		int ret = pointdbExportCSV(state->monData, state->pointdb, stream);
		SDL_CloseIO(stream);
		return ret;
	} else {
		return 0;
	}
}


int pointFileLoad(LoadedFileInfo* fileinfo, const char* path)
{
	#ifndef __EMSCRIPTEN__
	PointEditorState* state = fileinfo->userdata;
	if(state && state->pointsfile == fileinfo) {
		if(strncmp(path, fileinfo->filename, 4096) != 0) {
			fileinfo->filename = SDL_strdup(path);
			state->gotFilename = true;
		} else if(fileinfo->filename == path) {
			state->gotFilename = true;
		}
	}

	if(state) {
		// TODO okay well, this is kinda bad
		// the state owns the pointdb, but it's written as if the file 
		// should own it and that there can be floating files unassociated
		// with the state. that's bad, but right now it's not going to be 
		// fixed -- version 2.0 things for emerald/others lol
		int ret = pointdbLoad(state->monData, state->pointdb, path);
		Game->needUpdate = 3;
		state->pointsDirty = true;
		return ret;
	} else {
		return 0;
	}
	#else
	return 0;
	#endif
}

int pointFileImportCsv(LoadedFileInfo* fileinfo, const char* path)
{
	PointEditorState* state = fileinfo->userdata;
	if(state) {
		// TODO this is slow enough that it breaks synchronization!
		// which is sad! it can take more than one frame
		// the easiest "right" way to do this is to just save the info 
		// somewhere and load it in the state's update function
		// setting the need update flags after loading fixes it well enough
		int ret = pointdbImportCSV(state->monData, state->pointdb, path);
		Game->needUpdate = 3;
		state->pointsDirty = true;
		return ret;
	} else {
		return 0;
	}

}

void setupPointFile(PointEditorState* state, LoadedFileInfo* fileinfo, const char* filename)
{
	fileinfo->userdata = state;
	fileinfo->save = pointFileSave;
	fileinfo->load = pointFileLoad;
	fileinfo->import = pointFileImportCsv;
	fileinfo->export = pointFileExportCsv;
	fileinfo->extension = "wdpoints";
	fileinfo->exportExtension = "csv";
	if(!fileinfo->filename) {
		fileinfo->filename = filename;
		fileinfo->backupFile = filename;
	} else {
		pointFileLoad(fileinfo, fileinfo->filename);
	}

}

const char* sourceTrayNames[] = {
	"550+ BST",
	"500-549 BST",
	"450-499 BST",
	"400-449 BST",
	"300-399 BST",
	"0-299 BST",
	"Banned",
};

void pointState_assignTiles(PointEditorState* state)
{
	state->pointsDirty = false;
	GuiTrayContext* trx = state->trx;
	MonData* md = state->monData;
	MonPointDatabase* pointdb = state->pointdb;

	for(int i = 0; i < trx->numTrays; ++i) {
		GuiTray* tray = trx->trays[i];
		if(!tray) continue;
		tray->numSlots = 0;
		tray->flags |= Tray_Disabled;
	}

	for(int i = 0; i <= pointdb->numTiers; ++i) {
		trx->trays[i]->flags &= ~Tray_Disabled;
	}

	for(int i = 0; i < md->numMons; ++i) {
		int pt = pointdb->pointCosts[i];
		GuiTile* tile = &trx->tiles[i];
		bool doReassign = false;
		if(pt >= 0 && pt < pointdb->maxTiers) {
			doReassign = trx->trays[pt]->flags & Tray_Disabled;
		}
		if(doReassign || pt <= -1 || pt >= pointdb->maxTiers) {
			GuiTray** sourceTrays = trx->trays + state->sourceTrayIndex;
			MonDef* mon = &md->mons[i];
			int bst = 0;
			for(int stat = 0; stat < Num_Stats; ++stat) {
				bst += mon->stats[stat];
			}
			if(bst >= 550) {
				trayAdd(sourceTrays[0], tile, -1);
			} else if(bst >= 500) {
				trayAdd(sourceTrays[1], tile, -1);
			} else if(bst >= 450) {
				trayAdd(sourceTrays[2], tile, -1);
			} else if(bst >= 400) {
				trayAdd(sourceTrays[3], tile, -1);
			} else if(bst >= 300) {
				trayAdd(sourceTrays[4], tile, -1);
			} else {
				trayAdd(sourceTrays[5], tile, -1);
			}
		} else {
			GuiTray* tray = trx->trays[pt];
			trayAdd(tray, tile, -1);
			if(tray->numSlots * 2 > tray->gridSize.x * tray->gridSize.y) {
				tray->gridSize.x += 2;
			}
		}

	}
}

void pointState_create(GameState* base, GameContext* game)
{
	PointEditorState* state = (void*)base;
	state->pointsfile = &globalFileContext->points;
	state->pointsfile->userdata = state;


	state->monData = globalMonData;
	MonData* md = state->monData;
	state->pointdb = md->pointdb;


	state->trx = createTrayContext(
		md->pointdb->maxTiers + SDL_arraysize(sourceTrayNames) + 1, 
		2048);

	GuiTrayContext* trx = state->trx;
	trx->userdata = state;

	pannerEnable(&trx->panners[0]);
	for(int i = 0; i < md->numMons; ++i) {
		GuiTile* tile = &trx->tiles[i];
		monTile_init(tile, md, i+1);
	}

	for(int i = 0; i < md->pointdb->maxTiers; ++i) {
		char* title = calloc(1, 64);
		SDL_snprintf(title, 64, "%d Point%s", i, i == 1 ? "" : "s");
		GuiTray* tray = createTray(trx, title, (int2){3, 12}, 64);
		tray->userdata = (void*)(uint64_t)i;
		tray->kind = TrayKind_PointTier;
		tray->flags |= Tray_Disabled 
		| Tray_KeepSorted
		| Tray_ShowResizeControls 
		| Tray_ShowGutter
		| Tray_ShowOverlays;
	}

	state->sourceTrayIndex = trx->numTrays;
	for(int i = 0; i < SDL_arraysize(sourceTrayNames); ++i) {
		GuiTray* tray = createTray(trx, sourceTrayNames[i], (int2){8, 16}, 64);
		tray->kind = TrayKind_Source;
		tray->cameraIndex = 1;
		tray->flags |= Tray_Disabled 
		| Tray_ShowResizeControls 
		| Tray_ShowGutter
		| Tray_ShowOverlays;
	}

	setupPointFile(state, state->pointsfile, "Untitled.wdpoints");
	pointState_assignTiles(state);
}

void pointState_start(GameState* base, GameContext* game)
{
	// gets called every time we switch to this state
	PointEditorState* state = (void*)base;
	(void)state;

	SDL_Log("point state: start!");
	state->pointsDirty = true;
}

void pointState_update(GameState* base, GameContext* game)
{
	PointEditorState* state = (void*)base;
	(void)state;

	GuiTrayContext* trx = state->trx;
	//MonData* data = state->monData;
	MonPointDatabase* pointdb = state->pointdb;

	gui_update(Gui);
	common_drawTabs();

	if(state->pointsDirty) {
		pointState_assignTiles(state);
	}

	uiHbox();
	int lastTiers = pointdb->numTiers;
	if(uiButton(0, "-")) {
		pointdb->numTiers--;
		if(pointdb->numTiers < 1) {
			pointdb->numTiers = 1;
		}

	}
	uiLabelFmt(Gui_Highlighted, Align_Center, "%3d Tiers", pointdb->numTiers);
	if(uiButton(0, "+")) {
		pointdb->numTiers++;
		if(pointdb->numTiers >= pointdb->maxTiers-1) {
			pointdb->numTiers = pointdb->maxTiers-1;
		}
	}

	if(pointdb->numTiers != lastTiers) {
		pointState_assignTiles(state);
	}
	uiIncrement(32);

	#ifndef __EMSCRIPTEN__
	if(uiButton(0, "Save")) {
		if(!state->gotFilename) {
			openFileDialog(FileDialog_Save, state->pointsfile, 0);
		} else {
			fileinfoSave(state->pointsfile);
		}
	}
	if(uiButton(0, "Save As")) {
		openFileDialog(FileDialog_Save, state->pointsfile, 0);
	}
	if(uiButton(0, "Load")) {
		openFileDialog(FileDialog_Load, state->pointsfile, 0);
	}
	if(uiButton(0, "Import CSV")) {
		openFileDialog(FileDialog_Import, state->pointsfile, 0);
	}
	if(uiButton(0, "Export CSV")) {
		openFileDialog(FileDialog_Export, state->pointsfile, 0);
	}
	#else
	/*
	if(uiButton(0, "Import CSV from clipboard")) {
		char* csv = SDL_GetClipboardText();
		if(!csv) {

		}
		pointdbImportCSVFromText(state->monData, state->pointdb, csv, strlen(csv));
		SDL_free(csv);
	}
	if(uiButton(0, "Export CSV to clipboard")) {
		SDL_IOStream* stream = SDL_IOFromDynamicMem();
		int ret = pointdbExportCSV(state->monData, state->pointdb, stream);
		SDL_SeekIO(stream, 0, SDL_IO_SEEK_SET);
		size_t sz;
		char* data = SDL_LoadFile_IO(stream, &sz, true);
		SDL_SetClipboardText(data);
		SDL_free(data);

	}
	*/
	#endif

	uiIncrement(32);

	if(state->gotFilename) {
		uiLabelFmt(Gui_Highlighted, Align_Center, "Editing %s", 
			state->pointsfile->filename
			 + getFilenameStart(
			 	state->pointsfile->filename, 
			 	SDL_strlen(state->pointsfile->filename)));
	} else {
		uiLabelFmt(0, Align_Center, "No file loaded");
	}

	if(state->resetMode == 0 && uiButton(0, "Reset Points")) {
		state->resetMode = 1;
	} else if(state->resetMode == 1 && uiButton(0, "Are you sure?")) {
		state->resetMode = 0;
		state->gotFilename = false;
		state->pointsfile->filename = nullptr;
		state->pointsfile->backupFile = nullptr;
		pointdb->numTiers = 0;
		for(int i = 0; i < state->monData->numMons; ++i) {
			pointdb->pointCosts[i] = -1;
			monTile_init(&trx->tiles[i], state->monData, i+1);
		}
		pointState_assignTiles(state);
	}

	uiIncrement(32);
	uiLabel(Gui_Highlighted, Align_Center, "Hold SPACE to access unsorted Pokemon", -1);

	uiPop();


	float2 trayPen = Gui->pen;
	trayPen.y += 16;
	float4 sourceTrayBox = 0;
	if(keyDown(SDL_SCANCODE_SPACE)) {
		sourceTrayBox.x = 0;
		sourceTrayBox.y = trayPen.y + Game->windowSize.y / 3;
		sourceTrayBox.zw = Game->windowSize;
	} else if(keyJustUp(SDL_SCANCODE_SPACE)) {
		for(int i = 0; i <= pointdb->numTiers; ++i) {
			int index = pointdb->numTiers - i;
			GuiTray* tray = trx->trays[index];
			tray->flags &= ~Tray_ReqExclusionBox;
		}
	}

	int nextParity = 0;
	for(int i = 0; i <= pointdb->numTiers; ++i) {
		int index = pointdb->numTiers - i;
		GuiTray* tray = trx->trays[index];
		traySetPos(tray, trayPen);
		tray->flags &= ~Tray_Disabled;
		if(keyDown(SDL_SCANCODE_SPACE)) {
			tray->flags |= Tray_ReqExclusionBox;
			tray->exclusionBox = sourceTrayBox;
		}
		if(nextParity) {
			tray->flags |= Tray_AltBg;
		} else {
			tray->flags &= ~Tray_AltBg;
		}
		nextParity ^= tray->gridSize.x & 1;

		trayDraw(tray);
		trayPen.x += trayRegion(tray).z + 16;
	}

	trayPen.x = 0;
	trayPen.y += Game->windowSize.y / 3.0f;


	float4 playerBox = {0, 0, 8, game->windowSize.y};

	if(keyDown(SDL_SCANCODE_SPACE)) {
		pannerDisable(&trx->panners[0]);
		pannerEnable(&trx->panners[1]);

		playerBox.w = trayPen.y;

		for(int i = 0; i < SDL_arraysize(sourceTrayNames); ++i) {
			int index = state->sourceTrayIndex + i;
			GuiTray* tray = trx->trays[index];
			tray->flags &= ~Tray_Disabled;
			traySetPos(tray, trayPen);
			if(nextParity) {
				tray->flags |= Tray_AltBg;
			} else {
				tray->flags &= ~Tray_AltBg;
			}
			nextParity = tray->gridSize.x & 1;

			trayDraw(tray);
			trayPen.x += trayRegion(tray).z + 16;
		}
	} else if(keyJustUp(SDL_SCANCODE_SPACE)) {
		pannerDisable(&trx->panners[1]);
		pannerEnable(&trx->panners[0]);
		for(int i = 0; i < SDL_arraysize(sourceTrayNames); ++i) {
			int index = state->sourceTrayIndex + i;
			GuiTray* tray = trx->trays[index];
			tray->flags |= Tray_Disabled;
		}
	}

	if(trx->lastTile) {
		Gui->pen = playerBox.zw;
		Gui->pen.y -= (globalDefaultFont.line + Gui->padding) * 8;
		MonDef* mon = &globalMonData->mons[trx->lastTile->id - 1];

		drawStatBlock(Gui, mon, 128, (uint64_t)trx->lastTile->userdata);

		
		// Don't bother with a movelist for now, we don't have the data for them
		/*
		MonData* md = globalMonData;
			//uiSpacer(128 + 8 * 11);
		Gui->pen = playerBox.zw;
		Gui->pen.y -= (globalDefaultFont.line + Gui->padding) * 10;
		Gui->pen.x += 128 * 2 + 8 * 14;
		uiHbox();
		uiVbox();
		float savedPadding = Gui->padding;
		Gui->padding = 0;
		for(int i = 0; i < 100; ++i) {
			if(mon->moves[i] == 0xFFFF) {
				break;
			}

				//uiLabel(0, Align_Left, md->moves[mon->moves[i]].name, -1);
			if(uiButton(Gui_Button_NoElevation, md->moves[mon->moves[i]].name)) {
				state->lastMove = mon->moves[i];
			}
			if((i+1) % 10 == 0) {
				uiPop();
				uiVbox();
			}

		}
		Gui->padding = savedPadding;
		uiPop();
		uiPop();
		if(state->lastMove != -1) {

			MoveDef* move = &md->moves[state->lastMove];
			Gui->pen = playerBox.zw;
			Gui->pen.y -= (globalDefaultFont.line + Gui->padding) * 13;
			uiVbox();

			uiLabelFmt(0, Align_Left, "%s - %s", move->name, Mon_TypeNames[move->type]);
			uiLabelFmt(Gui_Button_Mono, Align_Left, 
				"pow:%d, acc:%d%%, priority:%d, effect:%d%%, pp:%d", 
				move->power, move->accuracy, move->priority, move->chance, move->pp);
			uiLabel(0, Align_Left, move->desc, -1);

			uiPop();
		}
		
		*/
	} else {
		//state->lastMove = -1;
	}




	trayContextUpdate(trx);
}

void pointState_stop(GameState* base, GameContext* game)
{
	PointEditorState* state = (void*)base;
	(void)state;
}
