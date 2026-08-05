#pragma once
#include "wb_gamemath.h"
#include "sprite.h"

typedef struct GuiTray GuiTray;
typedef struct GuiTile GuiTile;
typedef struct GuiTrayContext GuiTrayContext;
typedef int (*GuiTileProc)(float2 pos, GuiTray* tray, GuiTile* tile);

#define TILE_OK 0
#define TILE_RETURN 1

typedef struct GuiTile
{
	int kind;
	int id;
	int counter;
	int wasInFilter;
	float4 grabRegion;
	float4 activeRegion;
	float2 size;
	void* userdata;
	const char* title;
	GuiTray* tray;
	GuiTray* origin;

	GuiTileProc create, serialize;
	GuiTileProc add, remove, canDrop;
	GuiTileProc draw, drawOverlay, update;
} GuiTile;

enum 
{
	Tray_Disabled = 1<<0,
	Tray_HideHeader = 1<<1,
	Tray_ShowGutter = 1<<2,
	Tray_ShowOverlays = 1<<3,
	Tray_HideBg = 1<<4,
	Tray_RegionDirty = 1<<5,
	Tray_KeepSorted = 1<<6,
	Tray_ReqInclusionBox = 1<<7,
	Tray_ReqExclusionBox = 1<<8,
	Tray_ShowResizeControls = 1<<9,
	Tray_AltBg = 1<<10,
	Tray_GutterBg = 1<<11,
	Tray_GotDrop = 1<<12
};

typedef void (*GuiTrayProc)(GuiTray* tray);

typedef struct GuiTray
{
	const char* title;
	GuiTile** slots;
	GuiTrayContext* trx;

	float2 pos;
	float2 slotSize;
	float2 gutterSize;
	int2 gridSize;

	float4 lastRegion;
	float4 bodyRegion;
	float4 headerRegion;
	float4 inclusionBox, exclusionBox;

	GuiTrayProc drawHeader, drawBackground;

	int numSlots, maxSlots;
	int spacerSpot;
	int flags;
	int id, kind;
	void* userdata;
	uint32_t hlColor, bgColor[2], headerColor;
	uint32_t accentColor;
	int cameraIndex;
} GuiTray;

static inline
int trayFindTile(GuiTray* tray, GuiTile* tile)
{
	for(int i = 0; i < tray->numSlots; ++i) {
		if(tile == tray->slots[i]) {
			return i;
		}
	}
	return -1;
}

static inline
void traySetPos(GuiTray* tray, float2 p)
{
	if(tray->pos.x != p.x || tray->pos.y != p.y) {
		tray->pos = p;
		tray->flags |= Tray_RegionDirty;
	}
}
static inline
void traySetPosXY(GuiTray* tray, float x, float y)
{
	traySetPos(tray, (float2){x, y});
}



static inline
bool trayCanInteract(GuiTray* tray, float2 p)
{
	if(tray->flags & Tray_Disabled) {
		return false;
	}

	bool excluded = (tray->flags & Tray_ReqExclusionBox) && rect_contains(tray->exclusionBox, p);
	bool included = (tray->flags & Tray_ReqInclusionBox) && !rect_contains(tray->inclusionBox, p);

	return !excluded && !included;
}

typedef struct GuiPanner
{
	float2 lastPos;
	float2 grab;
	float4 screenRegion;
	union {
		char bytes[8];
		struct {
			char grabbed;
			char enabled;
			char panX, panY;
		};
	};
} GuiPanner;

static inline
bool pannerGrab(GuiPanner* panner, float2 point, Xform* camera) 
{
	if(!panner->enabled) return false;
	float4 r = panner->screenRegion;
	bool region0 = r.x == 0 && r.y == 0 && r.z == 0 && r.w == 0;
	if(region0 || rect_contains(r, point)) {
		panner->grab = point;
		panner->lastPos = camera->pos;
		panner->grabbed = true;
		return true;
	}
	return false;
}


static inline
void pannerUpdate(GuiPanner* panner, float2 newPoint, Xform* camera)
{
	if(!panner->enabled) return;
	if(panner->grabbed) {
		float2 grab = panner->grab;
		float2 last = panner->lastPos;

		if(panner->panX) {
			camera->pos.x = (grab.x - newPoint.x) / camera->scale + last.x;
		}
		if(panner->panY) {
			camera->pos.y = (grab.y - newPoint.y) / camera->scale + last.y;
		}
	}
}

static inline
void pannerDisable(GuiPanner* panner)
{
	panner->enabled = false;
	panner->grabbed = false;
}



static inline
void pannerEnable(GuiPanner* panner)
{
	panner->enabled = true;
}


typedef struct GuiTrayContext
{
	GuiTray** trays;
	int numTrays, maxTrays;

	GuiTile* tiles;
	int numTiles, maxTiles;

	Xform trayCamera[4];
	GuiPanner panners[4];

	float2 grabbedPoint;
	GuiTile* grabbedTile;
	GuiTile* lastTile;
	GuiTray* grabOrigin;
	void* userdata;
	int grabbedIndex, nextTrayId;

	char tileFilter[64];
	int filterMode;
	float padding;
} GuiTrayContext;


GuiTray* createTray(GuiTrayContext* trx, const char* title, int2 gridSize, float2 slotSize);
float4 trayRegion(GuiTray* tray);
float4 trayHeaderRegion(GuiTray* tray);
float4 trayBodyRegion(GuiTray* tray);
int trayHover(GuiTray* tray, float2 p);
void traySort(GuiTray* tray);
int trayAdd(GuiTray* tray, GuiTile* tile, int index);
GuiTile* trayGrab(GuiTray* tray, int index);
void trayDraw(GuiTray* tray);
GuiTrayContext* createTrayContext(int maxTrays, int maxTiles);
bool trayContextDrop(GuiTrayContext* trx, GuiTray* tray, bool drop);
int trayContextUpdate(GuiTrayContext* trx);
