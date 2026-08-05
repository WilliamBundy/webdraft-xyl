#include "gui_tray.h"
#include "util.h"
#include "drawtext.h"
#include "gui.h"

// TODO fix scaling!!! 

GuiTray* createTray(GuiTrayContext* trx, const char* title, int2 gridSize, float2 slotSize)
{
	if(trx->numTrays >= trx->maxTrays) {
		return nullptr;
	}

	GuiTray* tray = calloc(1, sizeof(GuiTray));
	tray->maxSlots = i32max(gridSize.x * gridSize.y + 16, 256);
	tray->slots = calloc(tray->maxSlots, sizeof(GuiTile*));
	tray->trx = trx;
	tray->title = title;

	tray->slotSize = slotSize;
	tray->gridSize = gridSize;
	tray->gutterSize = (float2){0, 16};

	tray->headerRegion.zw = tray->slotSize;
	tray->headerRegion.z *= tray->gridSize.x;
	tray->spacerSpot = -1;

	tray->hlColor = 0xAAAAAA;
	tray->bgColor[0] = 0x222222;
	tray->bgColor[1] = 0x333333;
	tray->headerColor = 0x666666;
	tray->accentColor = 0x33bbff;
	tray->flags |= Tray_RegionDirty | Tray_GutterBg;

	// TODO intercept deserialization here?

	tray->id = trx->nextTrayId++;
	trx->trays[trx->numTrays++] = tray;
	return tray;
}

float4 trayRegion(GuiTray* tray)
{
	if(tray->flags & Tray_RegionDirty) {
		tray->flags &= ~Tray_RegionDirty;
		tray->headerRegion.xy = tray->pos;
		float2 pos = tray->pos;
		if(~tray->flags & Tray_HideHeader) {
			pos.y += tray->headerRegion.w;
		}
		tray->bodyRegion.xy = pos;
		float2 slotSize = tray->slotSize;
		if(tray->flags & Tray_ShowGutter) {
			slotSize += tray->gutterSize;
		}
		tray->headerRegion.z = tray->gridSize.x * slotSize.x;
		tray->bodyRegion.zw = slotSize * int2_to_float(tray->gridSize);

		tray->lastRegion = tray->bodyRegion;
		tray->lastRegion.xy = tray->pos;

		if(~tray->flags & Tray_HideHeader) {
			tray->lastRegion.w += tray->headerRegion.w;
		}
	}
	return tray->lastRegion;
}

float4 trayHeaderRegion(GuiTray* tray)
{
	if(tray->flags & Tray_HideHeader) {
		return 0;
	}

	if(tray->flags & Tray_RegionDirty) {
		trayRegion(tray);
	}


	return tray->headerRegion;
}

float4 trayBodyRegion(GuiTray* tray) 
{
	if(tray->flags & Tray_RegionDirty) {
		trayRegion(tray);
	}
	return tray->bodyRegion;
}

int trayHover(GuiTray* tray, float2 p)
{
	Xform camera = tray->trx->trayCamera[tray->cameraIndex];
	float4 region = rXform(trayRegion(tray), camera);
	float4 body = rXform(trayBodyRegion(tray), camera);
	float4 header = rXform(trayHeaderRegion(tray), camera);
	if(rect_contains(region, p)) {
		if(rect_contains(header, p)) {
			tray->spacerSpot = -1;
		} else {
			p -= body.xy;
			float2 slotSize = tray->slotSize;
			if(tray->flags & Tray_ShowGutter) {
				slotSize += tray->gutterSize;
			}

			p /= slotSize * camera.scale;
			int2 ip = float2_to_int(p);
			tray->spacerSpot = ip.y * tray->gridSize.x + ip.x;
			if(ip.x >= tray->gridSize.x || ip.y >= tray->gridSize.y) {
				tray->spacerSpot = -1;
			} else if(ip.x < 0 || ip.y < 0) {
				tray->spacerSpot = -1;
			}
		}
	}
	return tray->spacerSpot;
}

void traySort(GuiTray* tray)
{
	for(int i = 1; i < tray->numSlots; ++i) {
		int j = i - 1;
		GuiTile* tile = tray->slots[i];

		while(j >= 0 && tray->slots[j]->id > tile->id) {
			tray->slots[j + 1] = tray->slots[j];
			j--;
		}
		tray->slots[j + 1] = tile;
	}
}

int trayAdd(GuiTray* tray, GuiTile* tile, int index)
{
	if(!tile) {
		return -1;
	}

	if(index == -1) {
		index = tray->numSlots;
	}

	if(index < 0 || index >= tray->maxSlots) {
		return -1;
	}
	if(tray->numSlots >= tray->maxSlots) {
		return -1;
	}

	if(index >= tray->numSlots) {
		index = tray->numSlots++;
	} else {
		for(int i = tray->numSlots++; i >= index; --i) {
			tray->slots[i+1] = tray->slots[i];
		}
	}

	tray->slots[index] = tile;

	if(tile->add) tile->add(0, tray, tile);
	if(tray->flags & Tray_KeepSorted) traySort(tray);
	tile->tray = tray;

	return 0;
}

GuiTile* trayGrab(GuiTray* tray, int index) 
{
	if(index >= tray->numSlots) {
		return nullptr;
	}

	if(index <= -1) {
		index = tray->numSlots - 1;
	}

	GuiTile* ret = tray->slots[index];
	tray->numSlots--;
	for(int i = index; i < tray->numSlots; ++i) {
		tray->slots[i] = tray->slots[i+1];
	}
	tray->slots[tray->numSlots] = nullptr;
	if(ret->remove) ret->remove(0, tray, ret);
	ret->tray = nullptr;
	return ret;
}

void trayDraw(GuiTray* tray)
{
	GuiTrayContext* trx = tray->trx;
	float padding = tray->trx->padding;
	Xform camera = tray->trx->trayCamera[tray->cameraIndex];
	//float4 region = rXform(trayRegion(tray), camera);
	float4 body = rXform(trayBodyRegion(tray), camera);
	float4 header = rXform(trayHeaderRegion(tray), camera);

	if(~tray->flags & Tray_HideHeader) {
		if(tray->drawHeader) {
			tray->drawHeader(tray);
		} else {
			drawFloat4(tray->headerColor, header);
			drawTextIn(tray->title, header.xy + padding, header.z - 1, 2.0f, camera);
		}

		if(tray->flags & Tray_ShowResizeControls) {
			gui_push_frame(Gui);
			Gui->pen = header.xy;
			Gui->pen.x += header.z - Gui->padding * 4;
			Gui->direction = Gui_HorizLeft;
			if(uiButton(0, "-")) {
				if(tray->gridSize.x > 1) {
					tray->gridSize.x--;
					tray->flags |= Tray_RegionDirty;
				}
			}
			if(uiButton(0, "+")) {
				if(tray->gridSize.x < 99) {
					tray->gridSize.x++;
					tray->flags |= Tray_RegionDirty;
				}
			}

			gui_restore_frame(Gui);
		}
	}

	int2 grid = tray->gridSize;
	float2 slotSize = tray->slotSize;
	if(tray->flags & Tray_ShowGutter) {
		slotSize += tray->gutterSize;
	}

	if(~tray->flags & Tray_HideBg) {
		if(tray->drawBackground) {
			tray->drawBackground(tray);
		} else {
			int parity = 0;
			if(tray->flags & Tray_AltBg) {
				parity = !parity;
			}
			float4 r;
			r.zw = slotSize * camera.scale;
			for(int i = 0; i < grid.y; ++i) {
				for(int j = 0; j < grid.x; ++j) {
					uint32_t color = tray->bgColor[((j + i) & 1) == parity];
					r.xy = body.xy + (float2){j, i} * r.zw;
					drawFloat4(color, r);
					if(tray->flags & Tray_GutterBg) {
						color = 0x222222;
						float4 gr = r;
						gr.y += tray->slotSize.y;
						gr.zw = tray->gutterSize;
						drawFloat4(color, gr);
					}
				}
			}
		}
	}

	float2 mpos = Game->input->mpos;
	// if the mouse is INSIDE the exclusion box or OUTSIDE the inclusion box 
	// tiles are NOT INTERACTIVE, so we invert that logic
	bool interactive = trayCanInteract(tray, mpos);

	for(int i = 0; i < grid.y; ++i) {
		for(int j = 0; j < grid.x; ++j) {
			int index = i * grid.x + j;
			if(index >= tray->numSlots) break;
			GuiTile* tile = tray->slots[index];
			if(!tile) continue;
			int2 ipos = {j, i}; 
			// TODO transform slotSize correctly
			float2 fpos = int2_to_float(ipos) * slotSize + body.xy;
			if(index == tray->spacerSpot) {
				float4 region;
				region.xy = fpos + padding;
				region.zw = slotSize;
				region.z /= 2;
				region.zw -= padding * 2;
				drawOutline(tray->accentColor, region, 1);

				fpos.x += slotSize.x / 2;
			}

			if(tile->draw) {
				tile->draw(fpos, tray, tile);
			}

			if(tray->flags & Tray_ShowOverlays) {
				if(tile->drawOverlay) {
					tile->drawOverlay(fpos, tray, tile);
				} else {
					if(tile->title) {
						float2 lpos = fpos;
						lpos.y += slotSize.y - globalDefaultFont.line;
						drawTextIn3(tile->title, lpos, slotSize.x-1, 1, 1.0, camera);
					}
				}
			}

			// yes it's bad form to do this here but whatever
			if(interactive) {
				float4 grab = tile->grabRegion; // TODO Xform grab region
				grab.xy += fpos;
				if(!trx->grabbedTile && rect_contains(grab, mpos)) {
					drawOutline(tray->accentColor, grab, 1);
					if(mbtnJustDown(SDL_BUTTON_LEFT)) {
						int index = trayHover(tray, mpos);
						trx->grabbedTile = trayGrab(tray, index);
						trx->grabbedPoint = (mpos - fpos) / camera.scale;
						trx->grabOrigin = tray;
						trx->grabbedIndex = index;
						trx->lastTile = trx->grabbedTile;
					}
				}
			}
		}
	}

	if(tray->spacerSpot != -1) {

		int j = tray->spacerSpot % tray->gridSize.x;
		int i = tray->spacerSpot / tray->gridSize.x;
		int2 ipos = {j, i}; 
		// TODO transform slotsize correctly
		float2 fpos = int2_to_float(ipos) * slotSize + body.xy;
		float4 region;
		region.xy = fpos + padding;
		region.zw = slotSize;
		region.z /= 2;
		region.zw -= padding * 2;
		drawOutline(tray->accentColor, region, 1);

	}

	tray->spacerSpot = -1;
}

GuiTrayContext* createTrayContext(int maxTrays, int maxTiles)
{
	GuiTrayContext* trx = calloc(1, sizeof(GuiTrayContext));
	trx->maxTrays = maxTrays;
	trx->maxTiles = maxTiles;
	trx->trays = calloc(maxTrays, sizeof(GuiTray*));
	trx->tiles = calloc(maxTiles, sizeof(GuiTile));
	for(int i = 0; i < SDL_arraysize(trx->trayCamera); ++i) {
		trx->trayCamera[i] = Xzero;
		trx->panners[i].panX = true;
	}
	trx->padding = 8;

	return trx;
}

bool trayContextDrop(GuiTrayContext* trx, GuiTray* tray, bool drop)
{
	int ret;
	GuiTile* tile = trx->grabbedTile;
	if(!tile) return true;

	if(!tray) {
		if(drop && (trx->grabOrigin || tile->origin)) {
			if(tile->origin) {
				trx->grabOrigin = tile->origin;
			}
			ret = trayAdd(trx->grabOrigin, tile, trx->grabbedIndex);
			trx->grabOrigin->flags |= Tray_GotDrop;
			trx->grabOrigin->spacerSpot = -1;
			trx->grabbedTile = nullptr;
			return true;
			if(ret == -1) {
				SDL_Log("lost tile index %d!", tile->id);
			}
		}
	} else {
		int hover = trayHover(tray, Game->input->mpos);
		if(drop) {
			int ret = trayAdd(tray, tile, hover);
			if(ret == -1) {
				if(trx->grabOrigin) {
					ret = trayAdd(trx->grabOrigin, tile, trx->grabbedIndex);
					trx->grabOrigin->flags |= Tray_GotDrop;
					trx->grabOrigin->spacerSpot = -1;
					trx->grabbedTile = nullptr;
				}
				if(ret == -1) {
					SDL_Log("lost tile index %d!", tile->id);
				}
				return true;
			} 
			trx->grabbedTile = nullptr;
			tray->spacerSpot = -1;
			tray->flags |= Tray_GotDrop;
			return true;
		}
	}
	return false;
}

int trayContextUpdate(GuiTrayContext* trx)
{
	for(int i = 0; i < trx->numTrays; ++i) {
		GuiTray* tray = trx->trays[i];
		tray->flags &= ~Tray_GotDrop;
	}
	if(trx->grabbedTile) {
		trx->lastTile = trx->grabbedTile;
		float2 mpos = Game->input->mpos;
		GuiTile* tile = trx->grabbedTile;
		if(tile->draw) {
			tile->draw(mpos - trx->grabbedPoint, nullptr, tile);
		}

		// this is a little crappy, since we're iterating over everything
		// all the time we have a grabbed tile, rather than just setting the
		// right stuff
		for(int i = 0; i < trx->numTrays; ++i) {
			GuiTray* tray = trx->trays[i];
			Xform camera = trx->trayCamera[tray->cameraIndex];
			if(tray->flags & Tray_Disabled) continue;
			if(!trayCanInteract(tray, mpos)) continue;

			// TODO need a callback to check whether can drop a particular 
			// tile in a given tray beyond interaction boxes

			float4 region = rXform(trayRegion(tray), camera);
			if(rect_contains(region, mpos)) {
				int ret = TILE_OK;
				if(tile->canDrop) {
					ret = tile->canDrop(mpos, tray, tile);
				}
				bool dropped = trayContextDrop(trx, tray, ret == TILE_OK && mbtnJustUp(SDL_BUTTON_LEFT));
				if(dropped) {
					return 1;
				}
			}
		}

		if(mbtnUp(SDL_BUTTON_LEFT)) {
			// if the button's not down, try returning it to whence it came 
			trayContextDrop(trx, nullptr, true);
			return 1;
		}
	} else {
		float2 mpos = Game->input->mpos;
		const int numPanners = SDL_arraysize(trx->panners);
		if(mbtnJustDown(SDL_BUTTON_LEFT)) {
			for(int i = numPanners-1; i >= 0; i--) {
				if(pannerGrab(&trx->panners[i], mpos, &trx->trayCamera[i])) break;
			}
		}

		for(int i = numPanners-1; i >= 0; i--) {
			pannerUpdate(&trx->panners[i], mpos, &trx->trayCamera[i]);
		}

		if(mbtnUp(SDL_BUTTON_LEFT)) {
			for(int i = numPanners-1; i >= 0; i--) {
				trx->panners[i].grabbed = false;
			}
		}
	}
	return 0;
}
