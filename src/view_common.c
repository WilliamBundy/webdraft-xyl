#include "view_points.h"
#include "gui.h"
#include "pokemon.h"
#include "drawtext.h"

#include "view_common.h"
#include "util.h"

#include "gui_tray.h"

int monTile_draw(float2 pos, GuiTray* tray, GuiTile* tile)
{
	Xform camera = Xzero; //tray ? tray->trx->trayCamera : Xzero;
	SDL_FRect src = getMonRect(tile->id - 1);
	SDL_FRect dst = pXformRect(pos - src.w/2, (float2){src.w, src.h} * 2, camera);
	SDL_RenderTexture(Game->renderer, monTexture, &src, &dst);

	if(tray && (tray->kind != TrayKind_PointTier && tray->kind != TrayKind_DraftBoard)) {
		char buf[16];
		SDL_snprintf(buf, 16, "%d", (int)(uint64_t)tile->userdata);
		drawFloat4AlphaCamera(0, 0.5, (float4){pos.x, pos.y, 16, 16}, camera);
		drawText(buf, pos, -1, camera);
	}

	return 0;
}

int monTile_add(float2 pos, GuiTray* tray, GuiTile* tile)
{
	if(!tray) return 0;

	if(tray->kind == TrayKind_PointTier) {
		tile->userdata = tray->userdata;
		PointEditorState* state = tray->trx->userdata;
		MonPointDatabase* pointdb = state->pointdb;
		int origcost = pointdb->pointCosts[tile->id - 1];
		pointdb->pointCosts[tile->id - 1] = (int64_t)tray->userdata;
		if(pointdb->pointCosts[tile->id - 1] != origcost) {
			state->pointsfile->wasEdited = true;
		}
	} else if (tray->kind == TrayKind_Source) {
		tile->userdata = tray->userdata;
		PointEditorState* state = tray->trx->userdata;
		MonPointDatabase* pointdb = state->pointdb;
		int origcost = pointdb->pointCosts[tile->id - 1];
		pointdb->pointCosts[tile->id - 1] = -1;
		if(pointdb->pointCosts[tile->id - 1] != origcost) {
			state->pointsfile->wasEdited = true;
		}

	}
	return 0;
}

int monTile_canDrop(float2 pos, GuiTray* tray, GuiTile* tile)
{
	if(tray->kind == TrayKind_DraftBoard) {
		uint64_t ptval = (uint64_t)tray->userdata; 
		DraftBoardState* state = tray->trx->userdata;
		MonPointDatabase* pointdb = state->pointdb;
		int origcost = pointdb->pointCosts[tile->id - 1];
		if(ptval != origcost) {
			return TILE_RETURN;
		}
	}
	return TILE_OK;
}

int playerTile_draw(float2 pos, GuiTray* tray, GuiTile* tile)
{
	float4 grab = tile->grabRegion;
	grab.xy += pos;
	drawFloat4(0x666666, grab);
	if(rect_contains(grab, Game->input->mpos)) {
		drawOutline(0xffbb33, grab, 1);
	}

	SDL_FRect src = {64, 0, 32, 32};
	SDL_FRect dst = pXformRect(grab.xy + (grab.zw - 32) * 0.5, 32, Xzero);
	SDL_RenderTexture(Game->renderer, Game->texture, &src, &dst);

	grab.x += 32;
	grab.z = 512;
	drawFloat4(0x222222, grab);
	drawOutline(0x333333, grab, 1);


	Gui->pen = grab.xy;
	PlayerTabUi* ui = tile->userdata;
	uiHbox();
	uiImg(0, Align_Left, monTexture, getMonRect(
		(tile->id + 
			murmur3(
				ui->ownerbox.buffer, 
				ui->ownerbox.numChars, 
				137513791)) % 251), 64);
	Gui->pen += Gui->padding;

	uiVbox();
	uiHbox();
	Gui->row = 24;
	Gui->column = 40;
	uiLabel(0, AlignCenterRight, "Name:", -1);
	Gui->column = -1;
	uiTextbox(0, 160, &ui->ownerbox);
	uiIncrement((float2){16, 0});
	Gui->column = 80;
	uiIncrement((float2){64, 0});

	uiPop();
	uiPop();
	uiPop();

	//drawText(tile->title, grab.xy + 8, -1, Xzero);
	return 0;
}

#include "pokemon.h"
void drawStatBlock(GuiContext* gui, MonDef* mon, float barWidth, int pts)
{
	gui_vbox(gui);
	gui_hbox(gui);
	gui_img(gui, Gui_Highlighted | Gui_Darkened, Align_Left, monTexture, getMonRect(mon->number), barWidth);
	float4 imgbox;
	imgbox.xy = gui->lastPen;
	imgbox.zw = gui->lastSize;
	drawOutlineCamera(gui->defaultStyle.colors[Color_Accent], imgbox, 1, gui->camera);
	gui_vbox(gui);
	//gui_label(gui, 0, Align_Left, mon->name, -1);
	bool oneType  = (mon->types[1] == mon->types[0]);
	gui_label_fmt(gui, 0, Align_Left, "%s - %s%s%s", mon->name,
		Mon_TypeNames[mon->types[0]],
		(!oneType ? "/" : ""),
		(!oneType ? Mon_TypeNames[mon->types[1]] : ""));
	int sum = 0;
	for(int i = 0; i < Num_Stats; ++i) {
		gui_hbox(gui);
		gui_label_fmt(gui, Gui_Button_Mono, Align_Left, "%s %3d", Mon_StatHeaderTitles[i], mon->stats[i]);

		sum += mon->stats[i];
		uint32_t color = 0;
		if(mon->stats[i] >= 140) {
			color = 0x6bedd3;
		} else if(mon->stats[i] >= 120) {
			color = 0x44f022;
		} else if(mon->stats[i] >= 100) {
			color = 0x83eb31;
		} else if(mon->stats[i] >= 90) {
			color = 0xbce84d;
		} else if(mon->stats[i] >= 75) {
			color = 0xd9e84d;
		} else if(mon->stats[i] >= 50) {
			color = 0xc8cc45;
		} else {
			color = 0xeb4034;
		}

		float w = (mon->stats[i] / 255.0) * barWidth;
		float2 size = {w, globalDefaultFont.line};

		drawFloat4Camera(0x333333, (float4){gui->pen.x, gui->pen.y, barWidth, size.y}, gui->camera);
		drawFloat4Camera(color, (float4){gui->pen.x, gui->pen.y, size.x, size.y}, gui->camera);
		drawOutlineCamera(color, (float4){gui->pen.x, gui->pen.y, barWidth, size.y}, 1, gui->camera);
		uiIncrement(size);

		gui_popbox(gui);
	}
	gui_label_fmt(gui, Gui_Button_Mono, Align_Left, "BST:   %3d, %.1f st/pt", sum, (double)sum / (double)pts);


	gui_popbox(gui);
	gui_popbox(gui);

	gui_increment(gui, 16 * Gui_DirVec[Gui->direction]);

	/*
	MonAbility* abi = &globalMonData->abilities[mon->abilities[0]-1];
	gui_label_fmt(gui, 0, Align_Left, "%s - %s", abi->name, abi->desc);
	if(mon->abilities[1] != -1) {
		abi = &globalMonData->abilities[mon->abilities[1]-1];
		gui_label_fmt(gui, 0, Align_Left, "%s - %s", abi->name, abi->desc);
	}
	*/

	gui_popbox(gui);
}