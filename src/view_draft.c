#include "view_draft.h"
#include "gui.h"
#include "view_common.h"
#include "util.h"
#include "sprite.h"
#include "drawtext.h"

int draftFileSave(LoadedFileInfo* fileinfo, const char* path)
{
	DraftBoardState* state = fileinfo->userdata;
	if(state && state->draftFile == fileinfo) {
		if(strncmp(path, fileinfo->filename, 4096) != 0) {
			state->draftFile->filename = path; 
			state->draftFile->backupFile = nullptr;
			state->gotFilename = true;
		}
	}

	if(!fileinfo->backupFile) {
		const char* backup = getBackupPath(fileinfo->filename, "draft");
		fileinfo->backupFile = backup;
	}

	SDL_Log("saving %s", fileinfo->filename);
	SDL_CopyFile(fileinfo->filename, fileinfo->backupFile);
	fileinfo->wasEdited = false;
	return draftSave(state->monData, state->board, state->playerdb, state->pointdb, path);
}

int draftFileLoad(LoadedFileInfo* fileinfo, const char* path) 
{
	DraftBoardState* state = fileinfo->userdata;
	if(state) {
		if(state->draftFile == fileinfo) {
			fileinfo->filename = path;
			state->gotFilename = true;
		}

		if(!state->playerdb) {
			state->playerdb = calloc(1, sizeof(PlayerDatabase));
		}

		if(!state->pointdb) {
			state->pointdb = calloc(1, sizeof(MonPointDatabase));
		}

		SDL_Log("loading %s", path);
		//int ret = playerdbLoad(state->playerdb, path);
		int ret = draftLoad(state->monData, state->board, state->playerdb, state->pointdb, path);
		//globalFileContext->players.filename = nullptr;
		//globalFileContext->points.filename = nullptr;
		state->draftDirty = true;
		Game->needUpdate = 3;
		return ret;
	} else {
		return 0;
	}

}

void setupDraftFile(DraftBoardState* state, LoadedFileInfo* fileinfo, const char* filename)
{
	fileinfo->userdata = state;
	fileinfo->save = draftFileSave;
	fileinfo->load = draftFileLoad;
	fileinfo->extension = "wddraft";
	if(fileinfo->filename) {
		fileinfoLoad(fileinfo);
	} else {
		fileinfo->filename = filename;
	}
}

void draftState_create(GameState* base, GameContext* game)
{
	DraftBoardState* state = (void*)base;
	state->resetMode = 0;
	state->monData = globalMonData;

	state->draftFile = &globalFileContext->draft;

	MonData* md = state->monData;
	state->board = md->draftboard;
	state->playerdb = md->playerdb;
	state->pointdb = md->pointdb;
	setupDraftFile(state, state->draftFile, "Untitled.wddraft");

	state->playerBoxWidth = 640 + 96 + 32;

	state->trx = createTrayContext(
		md->pointdb->maxTiers + md->playerdb->maxPlayers + 256,
		2048);
	GuiTrayContext* trx = state->trx;
	trx->userdata = state;
	for(int i = 0; i < md->numMons; ++i) {
		GuiTile* tile = &trx->tiles[i];
		monTile_init(tile, md, i+1);
	}
	trx->numTiles = md->numMons;
	pannerEnable(&trx->panners[0]);
	pannerEnable(&trx->panners[1]);
	trx->panners[1].panX = false;
	trx->panners[1].panY = true;


	state->pointTrays = calloc(md->pointdb->maxTiers, sizeof(GuiTray*));
	for(int i = 0; i < md->pointdb->maxTiers; ++i) {
		char* title = calloc(1, 64);
		SDL_snprintf(title, 64, "%d Point%s", i, i == 1 ? "" : "s");
		GuiTray* tray = createTray(trx, title, (int2){3, 12}, 64);
		tray->userdata = (void*)(uint64_t)i;
		tray->kind = TrayKind_DraftBoard;
		tray->flags |= Tray_Disabled 
		| Tray_ShowResizeControls 
		| Tray_ShowGutter
		| Tray_ShowOverlays 
		| Tray_KeepSorted
		| Tray_ReqExclusionBox;
		state->pointTrays[i] = tray;
	}

	state->playerTrays = calloc(md->playerdb->maxPlayers, sizeof(GuiTray*));
	for(int i = 0; i < md->playerdb->maxPlayers; ++i) {
		GuiTray* tray = createTray(trx, nullptr, (int2){10, 1}, 64);
		tray->cameraIndex = 1;
		tray->userdata = (void*)(uint64_t)i;
		tray->kind = TrayKind_DraftPicks;
		tray->flags |= Tray_Disabled 
		| Tray_HideHeader
		| Tray_ShowGutter
		| Tray_ShowOverlays 
		| Tray_ReqInclusionBox;
		state->playerTrays[i] = tray;
	}

	{
		state->randomTray = createTray(trx, nullptr, (int2){8, 1}, 64);
		GuiTray* tray = state->randomTray;
		tray->kind = TrayKind_DraftPicks;
		tray->flags |= 
		 Tray_HideHeader
		| Tray_ShowGutter
		| Tray_ShowOverlays 
		| Tray_ReqExclusionBox;
		tray->cameraIndex = 2;
	}
}

void updatePoints(DraftBoardState* state)
{
	MonPointDatabase* pointdb = state->pointdb;
	for(int i = 0; i <= pointdb->numTiers; ++i) {
		GuiTray* tray = state->pointTrays[i];
		uint64_t trayPt = (uint64_t)tray->userdata;
		for(int j = 0; j < tray->numSlots; ++j) {
			GuiTile* tile = tray->slots[j];
			uint64_t tilePt = pointdb->pointCosts[tile->id - 1];
			if(tilePt != trayPt) {
				trayGrab(tray, j);
				j--;
				if(tilePt != 0 && tilePt != -1) {
					trayAdd(state->pointTrays[tilePt], tile, -1);
					tile->origin = state->pointTrays[tilePt];
				}
			}
		}
	}

	for(int i = 0; i < state->trx->numTiles; ++i) {
		GuiTile* tile = &state->trx->tiles[i];
		int ptcost = state->monData->pointdb->pointCosts[tile->id-1];
		tile->userdata = (void*)(uint64_t)ptcost;
		if(tile->tray || ptcost == -1) continue;
		//SDL_Log("%s - %d", state->monData->mons[tile->id-1].name, (int)(uint64_t)tile->userdata);
		GuiTray* tray = state->pointTrays[ptcost];
		if(tray) {
			tile->userdata = (void*)(uint64_t)ptcost;
			trayAdd(tray, tile, -1);
			tile->origin = tray;
		}
	}

}

void updatePlayers(DraftBoardState* state)
{
	PlayerDatabase* playerdb = state->playerdb;
	for(int i = 0; i < playerdb->numPlayers; ++i) {
		state->playerTrays[i]->userdata = (void*)(uint64_t)i;
		PlayerTeam* team = &playerdb->players[i];
		for(int j = 0; j < team->numMons; ++j) {
			MonRef ref = team->mons[j];
			GuiTile* tile = &state->trx->tiles[ref.id];
			if(tile->tray != state->playerTrays[i]) {
				if(tile->tray) {
					int slot = trayFindTile(tile->tray, tile);
					if(slot != -1) {
						GuiTile* grabbed = trayGrab(tile->tray, slot);
						if(grabbed != tile) {
							SDL_Log("invalid code path?");
							SDL_assert(0);
						}
					}
				}
				trayAdd(state->playerTrays[i], tile, j);
			}
		}
	}
}

void draftState_reset(DraftBoardState* state)
{
	if(!state->playerdb || ! state->pointdb) {
		return;
	}

	for(int i = 0; i < state->playerdb->numPlayers; ++i) {
		state->playerdb->players[i].numMons = 0;
	}

	state->board->turnIndex = 0;
	state->draftStarted = false;
	state->gotFilename = false;
	state->draftFile->filename = "Untitled.wddraft";
	state->draftFile->backupFile = nullptr;
}

void draftState_start(GameState* base, GameContext* game)
{
	DraftBoardState* state = (void*)base;
	if(state->draftStarted && state->pointdb) updatePoints(state);
	if(state->draftStarted && state->playerdb) updatePlayers(state);
}

void draftStart(DraftBoardState* state)
{
	if(state->draftStarted) {
		return;
	}
	if(!state->playerdb || ! state->pointdb) {
		return;
	}

	state->draftStarted = true;

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
		tile->userdata = (void*)(uint64_t)pt;
		bool inHiddenTier = false;
		if(pt >= 0 && pt < pointdb->maxTiers) {
			inHiddenTier = trx->trays[pt]->flags & Tray_Disabled;
		} 

		if(!inHiddenTier && pt >= 0 && pt < pointdb->maxTiers) {
			GuiTray* tray = trx->trays[pt];
			tile->origin = tray;
			trayAdd(tray, tile, -1);
			if(tray->numSlots * 2 > tray->gridSize.x * tray->gridSize.y) {
				tray->gridSize.x += 2;
			}
		}
	}

	updatePlayers(state);
}

int getTeamTurn(int draftTurn, int numTeams)
{
	int loop = draftTurn % (numTeams * 2);
	if(loop >= numTeams) {
		loop = numTeams - 1 - (loop - numTeams);
	}
	return loop;
}

int getRoundTurn(int draftTurn, int numTeams)
{
	return draftTurn / numTeams;
}

void draftState_update(GameState* base, GameContext* game)
{
	DraftBoardState* state = (void*)base;

	gui_update(Gui);
	//common_drawTabs();	

	if(state->draftDirty || !state->draftStarted) {
		state->draftDirty = false;
		draftStart(state);
	}


	{
		float2 pen = Gui->pen;
		uiHbox();
		uiSpacer(32);
		uiPop();
		uiSpacer(32);

		MonPointDatabase* pointdb = state->pointdb;
		GuiTrayContext* trx = state->trx;

		state->playerBoxWidth = f32clamp(state->playerBoxWidth, 96, Game->windowSize.x + 96);
		{
			static bool grabbed = false;	
			static float2 grabPos = 0;
			static float origWidth = 0;
			float4 grabRect = {
				state->playerBoxWidth-16+8, 
				Gui->pen.y + (Game->windowSize.y - Gui->pen.y - 128) / 2 - 16, 
				16, 128};

			drawFloat4(0xAAAAAA, grabRect);
			if(game->input->mbtn[SDL_BUTTON_LEFT] == KEY_JUST_PRESSED) {
				if(rect_contains(grabRect, game->input->mpos)) {
					grabbed = true;
					grabPos = game->input->mpos;
					origWidth = state->playerBoxWidth;
				}
			}

			if(grabbed) {
				if(game->input->mbtn[SDL_BUTTON_LEFT] <= KEY_RELEASED) {
					grabbed = false;
				}

				state->playerBoxWidth = origWidth + game->input->mpos.x - grabPos.x;
			}
		}
		float4 playerBox = {0, Gui->pen.y, state->playerBoxWidth, Game->windowSize.y};



		trx->panners[0].screenRegion = (float4){
			playerBox.z + 32, 0, Game->windowSize.x, Game->windowSize.y};


		float2 trayPen = Gui->pen;
		trayPen.y += 16;
		trayPen.x += playerBox.z + 8;
		int nextParity = 0;

		SDL_Rect clipRect = {playerBox.z + 16, playerBox.y, game->windowSize.x, playerBox.w};
		SDL_SetRenderClipRect(game->renderer, &clipRect);
		if(state->showRandomControls) {
			state->randomTray->flags &= ~Tray_Disabled;
			state->randomTray->pos = trayPen;
			state->randomTray->exclusionBox = playerBox;
			trayDraw(state->randomTray);
			float2 storedpen = Gui->pen;


			//uiVbox();
			Gui->pen = 0;
			Gui->pen.x = trayPen.x + state->randomTray->gridSize.x * state->randomTray->slotSize.x + 16;
			Gui->pen.y = trayPen.y;
			if(uiButton(0, "Clear")) {
				while(state->randomTray->numSlots > 0) {
					trx->grabbedTile = trayGrab(state->randomTray, -1);
					trx->grabOrigin = nullptr;
					trayContextDrop(trx, nullptr, true);
				}
			}

			//uiPop();

			Gui->pen = storedpen;
			trayPen.y += 96;
		}


		for(int i = 0; i <= pointdb->numTiers; ++i) {
			GuiTray* tray = state->pointTrays[pointdb->numTiers - i];
			traySetPos(tray, trayPen);
			tray->flags &= ~Tray_Disabled;
			if(nextParity) {
				tray->flags |= Tray_AltBg;
			} else {
				tray->flags &= ~Tray_AltBg;
			}
			nextParity ^= tray->gridSize.x & 1;
			tray->exclusionBox = playerBox;
			tray->exclusionBox.z += 32;
			trayDraw(tray);
			if(state->showRandomControls) {
				float2 storedpen = Gui->pen;
				Xform storedcam = Gui->camera;
				Gui->pen = trayPen + Gui->padding + (float2){0, globalDefaultFont.line * 2 + Gui->padding};
				Gui->camera = trx->trayCamera[0];
				Gui->mpos = pXformInv(Game->input->mpos, Gui->camera);
				int flags = Gui_Button_NoElevation;
				float4 plbox = playerBox; //bleh
				plbox.z += 16;
				if(rect_contains(plbox, Game->input->mpos)) {
					flags |= Gui_Disabled;
				}
				if(uiButton(flags, "Pick Randomly")) {
					if(tray->numSlots > 0) {
						uint64_t seed = SDL_GetTicks() + 18518282137;
						splitmix64(&seed);
						int index = splitmix64(&seed) % tray->numSlots;

						GuiTile* tile = trayGrab(tray, index);
						trayAdd(state->randomTray, tile, -1);

					}
				}
				Gui->pen = storedpen;
				Gui->camera = storedcam;
				Gui->mpos = pXformInv(Game->input->mpos, Gui->camera);
			}
			trayPen.x += trayRegion(tray).z + 16;

		}



		// mask off left side
		//drawFloat4(0, playerBox);
		trx->panners[1].screenRegion = playerBox;

		PlayerDatabase* playerdb = state->playerdb;
		trayPen = Gui->pen;
		trayPen.x = 16;
		trayPen.y += 16;


		clipRect = (SDL_Rect){0, playerBox.y, playerBox.z - 16, playerBox.w};
		SDL_SetRenderClipRect(game->renderer, &clipRect);
		for(int i = 0; i < playerdb->numPlayers; ++i) {
			//if(playerdb->players[i].draftOrder != j) continue;
			GuiTray* tray = state->playerTrays[i];
			float height = tray->slotSize.y + tray->gutterSize.y;
			float4 header = {trayPen.x, trayPen.y, 96, height};

			int pointsUsed = 0;
			for(int i = 0; i < tray->numSlots; ++i) {

				pointsUsed += pointdb->pointCosts[tray->slots[i]->id - 1];
			}

			uint32_t color = 0x444444;

			if(getTeamTurn(state->board->turnIndex, playerdb->numPlayers) == i) {
				color = 0x44aa22;
			}

			if(pointsUsed > state->board->startingPoints) {
				color = 0xCC0000;
			}

			drawFloat4Camera(color, header, trx->trayCamera[1]);

			PlayerTeam* team = &playerdb->players[i];

			wbsf_Scale = 2.0f;
			if(team->owner.name[0] == 0) {
				char buf[32];
				SDL_snprintf(buf, 32, "Team %d", i + 1);
				drawText(buf, header.xy + 8, 96 - 8, trx->trayCamera[1]);
			} else {
				drawText(team->owner.name, header.xy + 8, 96 - 8, trx->trayCamera[1]);
			}


			char buf[32];
			SDL_snprintf(buf, 32, "%d,%d", state->board->startingPoints - pointsUsed, tray->gridSize.x - tray->numSlots);
			drawText(buf, header.xy + 8 + (float2){0, 32}, 96 - 8, trx->trayCamera[1]);
			wbsf_Scale = 1.0f;
			traySetPos(tray, trayPen + (float2){96, 0});
			tray->flags |= Tray_ReqInclusionBox;
			tray->inclusionBox = playerBox;
			trayDraw(tray);



			tray->flags &= ~Tray_Disabled;
			if(i & 1) {
				tray->flags |= Tray_AltBg;
			}
			trayPen.y += height + 16;
		}

		SDL_SetRenderClipRect(game->renderer, nullptr);

		//drawFloat4(0, (float4){0, 0, Game->windowSize.x, Gui->pen.y});
		//drawOutline(0xFFFF00, playerBox, 1);


		Gui->pen = pen;
		uiHbox();
		uiLabelFmt(Gui_Highlighted, Align_Center, "Round %2d", getRoundTurn(state->board->turnIndex, playerdb->numPlayers) + 1);
		if(uiButton(0, "-")) {
			state->board->turnIndex--;
		}
		uiLabelFmt(Gui_Highlighted, Align_Center, "Turn %3d", state->board->turnIndex + 1);
		if(uiButton(0, "+")) {
			state->board->turnIndex++;
		}
		uiSpacer(16);
		if(uiButton(0, "-5")) {
			state->board->startingPoints -= 5;
		}
		if(uiButton(0, "-")) {
			state->board->startingPoints -= 1;
		}
		uiLabelFmt(Gui_Highlighted, Align_Center, "Point Total %3d", state->board->startingPoints);
		if(uiButton(0, "+")) {
			state->board->startingPoints += 1;
		}
		if(uiButton(0, "+5")) {
			state->board->startingPoints += 5;
		}

		uiSpacer(16);
		if(uiButton(0, "-1")) {
			if(state->playerTrays[0]->gridSize.x > 1) {
				for(int i = 0; i < playerdb->numPlayers; ++i) {
					state->playerTrays[i]->gridSize.x--;
				}
			}

		}
		uiLabelFmt(Gui_Highlighted, Align_Center, "Num Mons: %2d", state->playerTrays[0]->gridSize.x);
		if(uiButton(0, "+1")) {
			if(state->playerTrays[0]->gridSize.x < 20) {
				for(int i = 0; i < playerdb->numPlayers; ++i) {
					state->playerTrays[i]->gridSize.x++;
				}
			}
		}

		uiSpacer(32);

		
		#ifndef __EMSCRIPTEN__
		if(uiButton(0, "Save")) {
			if(!state->gotFilename) {
				openFileDialog(FileDialog_Save, state->draftFile, 0);
			} else {
				fileinfoSave(state->draftFile);
			}
		}
		if(uiButton(0, "Save As")) {
			openFileDialog(FileDialog_Save, state->draftFile, 0);
		}
		if(uiButton(0, "Load")) {
			openFileDialog(FileDialog_Load, state->draftFile, 0);
		}
		#endif

		uiSpacer(32);
		if(state->resetMode == 1 && uiButton(0, "Are you sure?")) {
			state->resetMode = 0;
			draftState_reset(state);
		}

		if(state->resetMode == 0 && uiButton(0, "Reset Draft")) {
			state->resetMode = 1;
		}

		#ifndef __EMSCRIPTEN__	
		if(state->gotFilename) {
			uiSpacer(32);
			uiLabelFmt(Gui_Highlighted, Align_Center, "Editing %s", state->draftFile->filename);
		}
		#endif

		uiCheckbox(0, &state->showRandomControls, "Enable Randomness Controls");

		uiPop();

		uiSpacer(32);

		int wasDropped = trayContextUpdate(trx);
		if(wasDropped) {
			for(int i = 0; i < playerdb->numPlayers; ++i) {
				GuiTray* tray = state->playerTrays[i];
				int gotDrop = tray->flags & Tray_GotDrop;
				if(gotDrop && getTeamTurn(state->board->turnIndex, playerdb->numPlayers) == i) {
					state->board->turnIndex++;
				}

				PlayerTeam* team = &playerdb->players[i];
				for(int j = 0; j < tray->numSlots; ++j) {
					team->mons[j].id = (uint16_t)tray->slots[j]->id - 1;
				}
				team->numMons = tray->numSlots;
			}
		}

		if(trx->lastTile) {
			Gui->pen = playerBox.zw;
			Gui->pen.y -= (globalDefaultFont.line + Gui->padding) * 10;
			MonDef* mon = &globalMonData->mons[trx->lastTile->id - 1];

			drawStatBlock(Gui, mon, 128, (uint64_t)trx->lastTile->userdata);

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
		} else {
			state->lastMove = -1;
		}


	}

	gui_update(Gui);
	common_drawTabs();	
}

void draftState_stop(GameState* base, GameContext* game)
{

}


