/*
TODO list
+ separate the draft board from the points list
+ Team review tab, show extra mons, group by tier

+ Picks left

*/
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

SDL_FRect f4tofr(float4 f4)
{
	return (SDL_FRect){f4[0], f4[1], f4[2], f4[3]};
}

char toLower(char a)
{
	return a >= 'A' && a <= 'Z' ? (a - 'A' + 'a') : a;
}


/*
int stringDistance(const char* a, const char* b)
{
	int blen = SDL_strlen(b);
	int alen = SDL_strlen(a);

	if(SDL_strncmp(a, b, 78) == 0) return 0;
	int len = i32min(alen, blen);
	alen = len;
	blen = len;

	int tempA[80];
	int tempB[80];

	for(int i = 0; i < blen; ++i) {
		tempA[i] = i;
	}

	for(int i = 0; i < alen; ++i) {
		tempB[0] = i + 1;
		for(int j = 0; j < blen; ++j) {
			int cost = !(toLower(a[i]) == toLower(b[j]));
			tempB[j+1] = i32min(tempB[j]+1, i32min(tempA[j+1] + 1, tempA[j] + cost));
		}

		for(int j = 0; j < blen+1; ++j) {
			tempA[j] = tempB[j];
		}
	}

	return tempB[blen];
}
*/

int stringDistance2(const char* a, const char* b)
{
	int blen = SDL_strlen(b);
	int alen = SDL_strlen(a);

	if(SDL_strncmp(a, b, 78) == 0) return 0;
	int len = i32min(alen, blen);

	int count = 0;
	for(int i = 0; i < len; ++i) {
		if(toLower(a[i]) != toLower(b[i])) {
			count++;
		}
	}

	return count;
}




SDL_Texture* monTexture;

#define GLYPH_W 8
#define GLYPH_H 16

void saveDraft(const char* draftname, const char* teamsname);

enum 
{
	Type_None=-1,
	Type_Normal,
	Type_Fighting,
	Type_Flying,
	Type_Poison,
	Type_Ground,
	Type_Rock,
	Type_Bird,
	Type_Bug,
	Type_Ghost,
	Type_Steel,
	Type_Fire,
	Type_Water,
	Type_Grass,
	Type_Electric,
	Type_Psychic,
	Type_Ice,
	Type_Dragon,
	Type_Dark,
	Num_Types
};

enum
{
	Category_Status,
	Category_Physical,
	Category_Special
};

int TypeCategory[] = {
	[Type_Normal] = Category_Physical, 
	[Type_Fire] = Category_Special,
	[Type_Water] = Category_Special,
	[Type_Electric] = Category_Special,
	[Type_Grass] = Category_Special,
	[Type_Ice] = Category_Special,
	[Type_Fighting] = Category_Physical,
	[Type_Poison] =  Category_Physical,
	[Type_Ground] = Category_Physical,
	[Type_Flying] = Category_Physical,
	[Type_Psychic] =  Category_Special,
	[Type_Bug] =  Category_Physical,
	[Type_Rock] =  Category_Physical,
	[Type_Ghost] =  Category_Special,
	[Type_Dragon] = Category_Special,
	[Type_Steel] =  Category_Physical,
	[Type_Dark] =  Category_Physical
};

int TypeResists[Num_Types][Num_Types] = {
	[Type_Normal] = {Type_None}, 
	[Type_Fire] = {Type_Fire, Type_Grass, Type_Bug, Type_None}, 
	[Type_Water] = {Type_Fire, Type_Water, Type_Ice, Type_None}, 
	[Type_Electric] = {Type_Electric, Type_Flying, Type_None}, 
	[Type_Grass] = {Type_Water, Type_Electric, Type_Grass, Type_Ground, Type_None}, 
	[Type_Ice] = {Type_Ice,  Type_None}, 
	[Type_Fighting] = {Type_Bug, Type_Rock, Type_None}, 
	[Type_Poison] = {Type_Grass, Type_Fighting, Type_Poison, Type_Bug, Type_None}, 
	[Type_Ground] = {Type_Poison, Type_Rock, Type_None}, 
	[Type_Flying] = {Type_Grass, Type_Fighting, Type_Bug, Type_None}, 
	[Type_Psychic] = {Type_Fighting, Type_Psychic, Type_None}, 
	[Type_Bug] = {Type_Grass, Type_Fighting, Type_Ground, Type_None}, 
	[Type_Rock] = {Type_Normal, Type_Fire, Type_Poison, Type_Flying, Type_None}, 
	[Type_Ghost] = {Type_Poison, Type_Bug, Type_None}, 
	[Type_Dragon] = {Type_Fire, Type_Water, Type_Electric, Type_Grass, Type_None}, 
	[Type_Steel] = {Type_Normal, Type_Grass, Type_Ice, Type_Flying, Type_Psychic, Type_Bug, Type_Rock, Type_Bug, Type_Dragon, Type_Steel, Type_Dark, Type_Ghost, Type_None}, 
	[Type_Dark] = {Type_Dark, Type_Ghost, Type_None}, 
};

int TypeWeaknesses[Num_Types][Num_Types] = {
	[Type_Normal] = {Type_Fighting, Type_None}, 
	[Type_Fire] = {Type_Water, Type_Ground, Type_Rock, Type_None}, 
	[Type_Water] = {Type_Electric, Type_Grass, Type_None}, 
	[Type_Electric] = {Type_Ground, Type_None}, 
	[Type_Grass] = {Type_Fire, Type_Ice, Type_Poison, Type_Flying, Type_Bug, Type_None}, 
	[Type_Ice] = {Type_Fire, Type_Fighting, Type_Rock, Type_None}, 
	[Type_Fighting] = {Type_Flying, Type_Psychic, Type_None}, 
	[Type_Poison] = {Type_Ground, Type_Psychic, Type_None}, 
	[Type_Ground] = {Type_Water, Type_Grass, Type_Ice, Type_None}, 
	[Type_Flying] = {Type_Electric, Type_Ice, Type_Rock, Type_None}, 
	[Type_Psychic] = {Type_Bug, Type_Ghost, Type_None}, 
	[Type_Bug] = {Type_Fire, Type_Flying, Type_Rock, Type_None}, 
	[Type_Rock] = {Type_Water, Type_Grass, Type_Fighting, Type_Ground, Type_None}, 
	[Type_Ghost] = {Type_Ghost, Type_None}, 
	[Type_Dragon] = {Type_Ice, Type_Dragon, Type_None}, 
	[Type_Steel] = {Type_Fire, Type_Fighting, Type_Ground, Type_None}, 
	[Type_Dark] = {Type_Fighting, Type_Bug, Type_None}, 
};

int TypeImmunities[Num_Types][Num_Types] = {
	[Type_Normal] = {Type_Ghost, Type_None}, 
	[Type_Fire] = {Type_None}, 
	[Type_Water] = {Type_None}, 
	[Type_Electric] = {Type_None}, 
	[Type_Grass] = {Type_None}, 
	[Type_Ice] = {Type_None}, 
	[Type_Fighting] = {Type_None}, 
	[Type_Poison] = {Type_None}, 
	[Type_Ground] = {Type_Electric, Type_None}, 
	[Type_Flying] = {Type_Ground, Type_None}, 
	[Type_Psychic] = {Type_None}, 
	[Type_Bug] = {Type_None}, 
	[Type_Rock] = {Type_None}, 
	[Type_Ghost] = {Type_Normal, Type_None}, 
	[Type_Dragon] = {Type_None}, 
	[Type_Steel] = {Type_Poison, Type_None}, 
	[Type_Dark] = {Type_Psychic, Type_None}, 
};

uint8_t moveTableFile[] = {
#embed "cl-movedata.tabl"
};
uint8_t monTableFile[] = {
#embed "cl-mondata.tabl"
};

uint8_t baseOrigPointsFile[] = {
#embed "base_orig_points.txt"
};

uint8_t crystalMonsPng[] = {
	#embed "crystal-mons.png"
};
uint8_t graphicsPng[] = {
	#embed "graphics.png"
};

#define MV_SPIKES 200
#define MV_RAPID_SPIN 157
#define MV_EXPLOSION 59
#define MV_SELFDESTRUCT 179
#define MV_BATON_PASS 11
#define MV_HEAL_BELL 88
#define MV_PURSUIT 153
#define MV_LIGHT_SCREEN 108
#define MV_REFLECT 161
#define MV_ROAR 165
#define MV_WHIRLWIND 246
#define MV_THUNDER_WAVE 233
#define MV_STUN_SPORE 210
#define MV_SING 182
#define MV_HYPNOSIS 96
#define MV_SLEEP_POWDER 188
#define MV_SPORE 204
#define MV_LOVELY_KISS 110
#define MV_RECOVER 160
#define MV_MOONLIGHT 129
#define MV_MORNING_SUN 130

int2 highlightedMoves[] = {
	{MV_SPIKES, 1},
	{MV_RAPID_SPIN, 0},
	{MV_EXPLOSION, 2},
	{MV_SELFDESTRUCT, 2},
	{MV_BATON_PASS, 3},
	{MV_HEAL_BELL, 4},
	{MV_PURSUIT, 5},
	{MV_LIGHT_SCREEN, 6},
	{MV_REFLECT, 7},
	{MV_ROAR, 8},
	{MV_WHIRLWIND, 8},
	{MV_THUNDER_WAVE, 9},
	{MV_STUN_SPORE, 9},
	{MV_SING, 10},
	{MV_HYPNOSIS, 10},
	{MV_SLEEP_POWDER, 10},
	{MV_SPORE, 10},
	{MV_LOVELY_KISS, 10},
	{MV_RECOVER, 11},
	{MV_MOONLIGHT, 11},
	{MV_MORNING_SUN, 11},
};

typedef struct MoveDef
{
	char name[16];
	char effect[16];
	char desc[64];
	uint8_t accuracy;
	uint8_t power;
	int8_t priority;
	uint8_t type;
	uint8_t chance;
	uint8_t pp;
	uint8_t number;
	uint8_t reserved;
} MoveDef;

typedef struct MonDef
{
	char name[16];
	int preevo;
	int number;
	uint8_t types[2]; 
	uint8_t stats[6];
	uint16_t moves[100];
} MonDef;
int* evoTree;

typedef struct MonHasMove
{
	int id;
	int numMoves;
	uint8_t moves[sizeof(highlightedMoves)/sizeof(highlightedMoves[0])];
} MonHasMove;
MonHasMove* monHasMoves;

uint8_t* monWasLoaded;

void setupMonHasMoves(MonDef* mons, int numMons)
{
	monHasMoves = calloc(numMons, sizeof(MonHasMove));
	for(int i = 0; i < numMons; ++i) {
		MonDef* mon = &mons[i];
		monHasMoves[i].id = i;
		for(int j = 0; j < 100; ++j) {
			for(int k = 0; k < SDL_arraysize(highlightedMoves); ++k) {
				if(mon->moves[j] == highlightedMoves[k].x) {
					bool skip = false;
					for(int l = 0; l < monHasMoves[i].numMoves; ++l) {
						if(highlightedMoves[monHasMoves[i].moves[l]].y == highlightedMoves[k].y) {
							skip = true;
							break;
						}
					}

					if(skip) continue;

					monHasMoves[i].moves[monHasMoves[i].numMoves++] = k;
				}
			}
		}
	}
}

typedef struct TableHeader
{
	uint32_t magic, version, kind, count;
} TableHeader;

void setRgb(uint32_t rgb, float scale)
{
	SDL_SetTextureColorMod(Game->texture, 
		(uint8_t)(((float)((rgb >> 16) & 0xFF) / 255.0 * scale) * 255), 
		(uint8_t)(((float)((rgb >> 8) & 0xFF) / 255.0 * scale) * 255), 
		(uint8_t)(((float)(rgb & 0xFF) / 255.0 * scale) * 255));
}

void drawRect(uint32_t rgb, SDL_FRect fr, Xform camera)
{
	setRgb(rgb, 1.0);
	fr = rXformRect(fr, camera);
	SDL_RenderTexture(
		Game->renderer, 
		Game->texture, 
		&(SDL_FRect){1,1,1,1},
		&fr);
	setRgb(0xFFFFFF, 1.0f);
}

void drawRectAlpha(uint32_t rgb, float alpha, SDL_FRect fr, Xform camera)
{
	setRgb(rgb, 1.0);
	SDL_SetTextureAlphaMod(Game->texture, (uint8_t)(alpha * 255.0));
	fr = rXformRect(fr, camera);
	SDL_RenderTexture(
		Game->renderer, 
		Game->texture, 
		&(SDL_FRect){1,1,1,1},
		&fr);
	setRgb(0xFFFFFF, 1.0f);
	SDL_SetTextureAlphaMod(Game->texture, 255);
}

bool keyDown(int key)
{
	return Game->input->keys[key] >= KEY_PRESSED;
}

bool keyJustDown(int key)
{
	return Game->input->keys[key] == KEY_JUST_PRESSED;
}

bool keyUp(int key)
{
	return Game->input->keys[key] == KEY_RELEASED;
}

bool keyJustUp(int key)
{
	return Game->input->keys[key] == KEY_JUST_RELEASED;
}


bool mbtnDown(int btn)
{
	return Game->input->mbtn[btn] >= KEY_PRESSED;
}

bool mbtnUp(int btn)
{
	return Game->input->mbtn[btn] <= KEY_RELEASED;
}

bool mbtnJustDown(int btn)
{
	return Game->input->mbtn[btn] == KEY_JUST_PRESSED;
}


bool mbtnJustUp(int btn)
{
	return Game->input->mbtn[btn] == KEY_JUST_RELEASED;
}

wbsf_Spritefont font;

float4 drawText(const char* text, float2 p, float maxw, Xform camera)
{
	return wbsf_drawText(
		Game->renderer, 
		&font, 
		text, 
		p, 
		-1, -1,
		maxw, 
		Game->texture, 
		camera.pos, 
		camera.scale, 
		true, true);
}

float4 drawText2(const char* text, float2 p, size_t len, float maxw, Xform camera)
{
	return wbsf_drawText(
		Game->renderer, 
		&font, 
		text, 
		p, 
		len, 
		-1,
		maxw, 
		Game->texture, 
		camera.pos, 
		camera.scale, 
		true, true);
}

bool frect_contains(SDL_FRect fr, float2 p)
{
	return p.x > fr.x && p.y > fr.y && p.x < (fr.x + fr.w) && p.y < (fr.y + fr.h);
}

float2 uiLabel(const char* label, float2 p, bool hl, Xform camera)
{
	return drawText(label, p, -1, camera).zw;
}

bool uiButton(const char* label, float2 p, bool hl, Xform camera)
{
	size_t numChars = SDL_strlen(label);

	float2 sp = p;	

	SDL_FRect fr = pXformRect(p, (float2){(numChars+2)* GLYPH_W, GLYPH_H}, camera);

	float2 mpos = Game->input->mpos;
	//mpos = pXformInv(mpos, camera);
	bool res = false;
	const char* ends[] = {"<", ">"};
	bool endHl = hl;
	if(frect_contains(fr, mpos)) {
		ends[0] = "[";
		ends[1] = "]";
		if(mbtnDown(SDL_BUTTON_LEFT)) endHl = true;
		if(mbtnJustUp(SDL_BUTTON_LEFT)) {
			endHl = true;
			res = true;
		}
	}
	if(endHl) {
		drawRect(0x333333, fr, Xzero);

	}
	drawText2(ends[0], sp, 1, -1, camera);
	sp.x += GLYPH_W;
	drawText2(label, sp, numChars, -1, camera);
	sp.x += numChars * GLYPH_W;
	drawText2(ends[1], sp, 1, -1, camera);

	return res;
}


typedef struct UiTextbox
{
	char* str;
	int len, maxchars;
	bool hasfocus;
} UiTextbox;
UiTextbox* globalTextbox;

int uiHandleTextInput(SDL_Event event)
{
	UiTextbox* tb = globalTextbox;
	if(!tb) return 0;

	if(tb->maxchars == 0) {
		tb->maxchars = 254;
	}

	size_t len = SDL_strlen(event.text.text);
	if(len + tb->len > tb->maxchars) {
		len = tb->maxchars - tb->len;
	}

	SDL_memcpy(tb->str + tb->len, event.text.text, len);
	tb->len += len;
	return 1;
}

int uiHandleKeyDown(SDL_Event event)
{
	if(event.key.scancode == SDL_SCANCODE_TAB) return 0;
	if(event.key.scancode == SDL_SCANCODE_RETURN) return 0;
	if((event.key.mod & SDL_KMOD_CTRL) && 
		event.key.key != SDLK_V &&
		event.key.key != SDLK_C &&
		event.key.scancode != SDL_SCANCODE_BACKSPACE) return 0;
	if(event.type == SDL_EVENT_KEY_UP) return 0;

	UiTextbox* tb = globalTextbox;
	if(!tb) return 0;
	if(tb->maxchars == 0) {
		tb->maxchars = 254;
	}

	if(event.key.scancode == SDL_SCANCODE_ESCAPE) {
		tb->hasfocus = 0;
		globalTextbox = nullptr;
		return 1;
	}

	if(event.key.scancode == SDL_SCANCODE_BACKSPACE) {
		if(event.key.mod & SDL_KMOD_CTRL) {
			while(tb->len > 0 && tb->str[tb->len - 1] != ' ') {
				tb->str[--tb->len] = 0;
			}
			if(tb->len > 0 && tb->str[tb->len - 1] == ' ') {
				tb->str[--tb->len] = 0;
			}
		} else {
			if(tb->len > 0)
				tb->str[--tb->len] = 0;
		}
	}

	if(event.key.key == SDLK_V && (event.key.mod & SDL_KMOD_CTRL)) {
		char* clip = SDL_GetClipboardText();
		size_t len = SDL_strlen(clip);
		if(len + tb->len > tb->maxchars) {
			len = tb->maxchars - tb->len;
		}

		SDL_memcpy(tb->str + tb->len, clip, len);
		tb->len += len;

		SDL_free(clip);
	}
	if(event.key.key == SDLK_C && (event.key.mod & SDL_KMOD_CTRL)) {
		SDL_SetClipboardText(tb->str);
	}
	return 1;
}

float2 uiTextbox(const char* title, float2 p, int2 size, UiTextbox* tb, Xform camera)
{
	int2 itotalsize = (size + 2);
	float2 totalsize = {itotalsize.x, itotalsize.y};

	uint32_t color = 0x333333;
	if(tb->hasfocus) color = 0x666666;
	drawRect(color, (SDL_FRect){p.x - 4, p.y - 4, size.x * GLYPH_W + 8, size.y * GLYPH_H + 8}, camera);

	SDL_FRect fr = pXformRect(p, totalsize * (float2){GLYPH_W, GLYPH_H}, camera);
	float2 mpos = Game->input->mpos;
	//mpos = pXformInv(mpos, camera);
	bool inside = frect_contains(fr, mpos);
	if(mbtnJustUp(SDL_BUTTON_LEFT)) {
		if(!tb->hasfocus && inside) {
			tb->hasfocus = true;
			if(globalTextbox) globalTextbox->hasfocus = false;
			globalTextbox = tb;
			SDL_StartTextInput(Game->window);
		} else if(tb->hasfocus && !inside) {
			tb->hasfocus = true;
			globalTextbox = nullptr;
			SDL_StopTextInput(Game->window);
		}
	}

	if(tb->hasfocus && globalTextbox != tb) {
		globalTextbox = tb;
		SDL_StartTextInput(Game->window);
	}

	float xoff = 0;

	if(title) {
		xoff = drawText(title, (float2){p.x, p.y}, -1, camera).z;
	}

	float2 lastc = drawText2(tb->str, p + (float2){xoff, 0}, tb->len, size.x * GLYPH_W - 1, camera).xy;
	double d;
	(void)d;
	if(tb->hasfocus && SDL_modf(Game->totalGameTime, &d) < 0.5) {
		if(tb->len != tb->maxchars)  {
			SDL_FRect src = {1, 1, 1, 1}; //extGlyphSrc(EXT_Cursor);
			if(lastc.x >= size.x * GLYPH_W) {
				lastc.x = 0;
				lastc.y += GLYPH_H;
			}
			SDL_FRect dst = pXformRect(lastc + p + (float2){xoff, 0}, (float2){GLYPH_W * 0.25, GLYPH_H}, camera);
			SDL_RenderTexture(Game->renderer, Game->texture, &src, &dst);
		}
	}

	/*
	if(tb->hasfocus) {
		char buf[64];
		size_t len = SDL_snprintf(buf, 64, "%d/%d", tb->len, tb->maxchars);
		drawText(buf, p + totalsize - GLYPH_W - (float2){len, 0} * GLYPH_W + (float2){0, GLYPH_H}, -1, camera);
	}
	*/

	return totalsize;
}

typedef struct UiTrayEntry
{
	int kind;
	int id;
	SDL_Texture* texture;
	SDL_FRect src;
	void* userdata;
	int a, b;
	int reserved, wasInFilter;
} UiTrayEntry;

float2 grabbedPoint;
UiTrayEntry* grabbedEntry;
UiTrayEntry* lastEntry;
int grabbedIndex;


typedef struct UiTray
{
	float2 pos;
	const char* title;
	UiTrayEntry** slots;
	int numSlots, maxSlots;
	int2 size;
	int spacerSpot;
	int value;
	float2 slotSize;
	int style, visible;
	uint32_t highlightColor, bgColor;
} UiTray;
UiTray* grabOrigin;

typedef struct PlayerTeam
{
	const char* name;
	UiTray* tray;
	int points;
	int icon;
} PlayerTeam;

int teamPointTotal = 77;

//const char* teams[16];
int numTeams = 0;

PlayerTeam pteams[16];


int numMons;
MonDef* mons;
MonDef* monsBySpeed;
int* monIndex;

int numMoves;
MoveDef* moves;

SDL_FRect getMonRect(int mon)
{
	int row = monTexture->w / 64;
	int y = mon / row;
	int x = mon % row;
	return (SDL_FRect){x * 64, y * 64, 64, 64};
}

int** trayArrays;
int** trayEtrA, **trayEtrB;
int* trayArraySizes;
dict32* trayDict;
int** baseArrays;
int* baseSizes;
int** basePoints;
dict32* baseDict;
bool loadedFromFile = false;
bool editedPoints = false;
int loadedTrays = 0;

const char* sourceNames[] = {
	"Single Stage",
	"Final Form",
	"Middle Evo",
	"Base Form",
	"Banned",
	"???1","???2","???3"
};
UiTrayEntry* entries;
int loadTrays(char* text, size_t len, dict32* trayData, int** arrays, int** etrAarrays, int** etrBarrays, int* sizes);
int loadTeams(char* text, size_t len, PlayerTeam* teams, int maxTeams);
void trayUpdateValues(UiTray* tray);
void createTrays();

bool countWasLoaded = true;

void monSortStat(MonDef* mons, int numMons, int stat)
{
	for(int i = 1; i < numMons; ++i) {
		int j = i - 1;
		MonDef mon = mons[i];

		while(j >= 0 && mons[j].stats[stat] < mon.stats[stat]) {
			mons[j + 1] = mons[j];
			j--;
		}
		mons[j + 1] = mon;
	}
}



void testStart(GameState* state, GameContext* game)
{
	monTexture = SDL_CreateTextureFromSurface(Game->renderer, 
		SDL_LoadPNG_IO(SDL_IOFromMem(crystalMonsPng, SDL_arraysize(crystalMonsPng)), true)
	);//SDL_LoadPNG("crystal-mons.png"));
	TableHeader* monheader = (void*)monTableFile;
	numMons = monheader->count;
	mons = (void*)(monheader + 1);
	monsBySpeed = calloc(numMons, sizeof(MonDef));
	memcpy(monsBySpeed, mons, numMons * sizeof(MonDef));
	for(int i = 0; i < numMons; ++i) {
		monsBySpeed[i].preevo = i;
	}
	// 5 is speed
	monSortStat(monsBySpeed, numMons, 5);


	TableHeader* moveheader = (void*)moveTableFile;
	numMoves = moveheader->count;
	moves = (void*)(moveheader + 1);

	monWasLoaded = calloc(numMons, 1);

	wbsf_spritefontInit(
		&font, 
		(int4){128, 0, 8, 16}, 
		16, 16, 
		Game->surface->pixels, 
		Game->surface->w, Game->surface->h);

	setupMonHasMoves(mons, numMons);
	evoTree = calloc(numMons, sizeof(int));
	for(int i = 0; i < numMons; ++i) {
		if(mons[i].preevo == -1) continue;
		evoTree[mons[i].preevo]++;
	}

	trayArrays = calloc(200, sizeof(int*));
	trayEtrA = calloc(200, sizeof(int*));
	trayEtrB = calloc(200, sizeof(int*));
	trayArraySizes = calloc(200, sizeof(int));
	for(int i = 0; i < 200; ++i) {
		trayArrays[i] = calloc(1024, sizeof(int));
		trayEtrA[i] = calloc(1024, sizeof(int));
		trayEtrB[i] = calloc(1024, sizeof(int));
	}

	dict32* dict = dict32Create(1024);
	trayDict = dict;
	int trayIndex = 0;
	for(int i = 0; i < SDL_arraysize(sourceNames); ++i) {
		dict32Add(&dict, sourceNames[i], trayIndex++);
	}

	for(int i = 0; i < 100; ++i) {
		char buf[16];
		SDL_snprintf(buf, 16, "%d", i);
		dict32Add(&dict, buf, trayIndex++);
	}
	loadedTrays = trayIndex;

	entries = calloc(1024 + numMons, sizeof(UiTrayEntry));
	for(int i = 0; i < numMons; ++i) {
		SDL_FRect src = getMonRect(i);
		UiTrayEntry* entry = &entries[i];
		entry->id = i;
		entry->src = src;
		entry->texture = monTexture;
	}

	size_t size;
	char* data;

	size = 0;
	data = SDL_LoadFile("teams.txt", &size);
	if(size > 0 && data) {
		numTeams = loadTeams(data, size, pteams, 16);
		SDL_SaveFile("backup_teams.txt", data, size);
	}

	size = SDL_arraysize(baseOrigPointsFile);
	data = (char*)baseOrigPointsFile;

	int backupLoaded = loadedTrays;
	loadedTrays = 0;
	baseDict = dict32Create(256);
	for(int i = 0; i < 20; ++i) {
		char buf[16];
		SDL_snprintf(buf, 16, "%d", i);
		dict32Add(&baseDict, buf, loadedTrays++);
	}
	baseArrays = calloc(64, sizeof(int*));
	baseSizes = calloc(64, sizeof(int));
	basePoints = calloc(64, sizeof(int*));
	for(int i = 0; i < 64; ++i) {
		baseArrays[i] = calloc(256, sizeof(int));
		basePoints[i] = calloc(256, sizeof(int));
	}
	if(data) {
		countWasLoaded = false;
		int numLoaded = loadTrays(data, size, baseDict, baseArrays, nullptr, basePoints, baseSizes);
		countWasLoaded = true;
		SDL_Log("(source) loaded %d/%d", numLoaded, numMons);
	}
	loadedTrays = backupLoaded;

	size = 0;
	data = SDL_LoadFile("draft.txt", &size);
	loadedFromFile = true;
	if(size > 0 && data) {
		int numLoaded = loadTrays(data, size, dict, trayArrays, trayEtrA, trayEtrB, trayArraySizes);
		SDL_Log("(draft) loaded %d/%d", numLoaded, numMons);
		createTrays();
		SDL_SaveFile("backup_draft.txt", data, size);
	} else {
		size = 0;
		data = SDL_LoadFile("points.txt", &size);
		if(size > 0 && data) {
			int numLoaded = loadTrays(data, size, dict, trayArrays, trayEtrA, trayEtrB, trayArraySizes);
			SDL_Log("(points) loaded %d/%d", numLoaded, numMons);
			createTrays();
			SDL_SaveFile("backup_points.txt", data, size);
		} else {
			size = SDL_arraysize(baseOrigPointsFile);
			data = (char*)baseOrigPointsFile;
			if(data) {
				int numLoaded = loadTrays(data, size, dict, trayArrays, trayEtrA, trayEtrB, trayArraySizes);
				SDL_Log("(backup) loaded %d/%d", numLoaded, numMons);
				(void)numLoaded;
				createTrays();
				// known good, replace backups
				SDL_SaveFile("points.txt", data, size);
				SDL_SaveFile("backup_points.txt", data, size);
			}
		}
	}
}

float4 drawbox;
uint32_t dbcolor;
float dbalpha;
bool drawboxenabled;

void setDrawbox(float4 box, uint32_t color, float alpha)
{
	drawboxenabled = true;

	drawbox =box;
	dbcolor = color;
	dbalpha = alpha;
}

void drawDrawbox()
{
	if(drawboxenabled) {
		drawboxenabled = false;
		drawRectAlpha(dbcolor, dbalpha, f4tofr(drawbox), Xzero);
	}
}



void addTeam(const char* name)
{
	if(numTeams >= SDL_arraysize(pteams)) {
		return;
	}
	pteams[numTeams] = (PlayerTeam) {
		.name = name,
		.points = teamPointTotal,
		.icon = 0
	};
	numTeams++;

	//ieams[numTeams++] = name;
}

int trayAdd(UiTray* tray, UiTrayEntry* entry, int index);
UiTray* createTray(const char* title, int2 size, float2 slotSize) 
{
	UiTray* tray = SDL_calloc(1, sizeof(UiTray));
	tray->maxSlots = i32max(size.x * size.y, 1024);
	tray->slots = SDL_calloc(tray->maxSlots, sizeof(UiTrayEntry*));
	tray->title = title;
	tray->size = size;
	tray->slotSize = slotSize;
	tray->spacerSpot = -1;
	tray->value = -1;
	tray->visible = true;

	if(loadedFromFile) {
		uint32_t index = 0;
		int ret = dict32Get(trayDict, title, &index);
		if(ret == DICT32_OK) {
			int* array = trayArrays[index];
			int* As = trayEtrA[index];
			int* Bs = trayEtrA[index];
			int size = trayArraySizes[index];
			for(int i = 0; i < size; ++i) {
				entries[array[i] - 1].a = As[i];
				entries[array[i] - 1].b = Bs[i];
				trayAdd(tray, &entries[array[i] - 1], -1);
			}
		}
	}
	return tray;
}

bool uiTrayKeepSorted = false;
bool uiTrayDrawNames = true;
bool uiTrayDrawBadges = true;
bool uiTrayGutter = true;


float4 trayRegion(UiTray* tray)
{
	return (float4){
		tray->pos.x, tray->pos.y, 
		tray->size.x * tray->slotSize.x,
		(tray->size.y + (tray->style == 0 ? 1 : 0)) * ((uiTrayGutter ? 16 : 0) + tray->slotSize.y)};
}

float4 trayHeaderRegion(UiTray* tray)
{
	if(tray->style == 0) {
		return (float4){
			tray->pos.x, tray->pos.y, 
			tray->size.x * tray->slotSize.x,
			tray->slotSize.y};
	} else {
		return 0;
	}
}

int trayHover(UiTray* tray, float2 p, Xform camera)
{
	float4 region = rXform(trayRegion(tray), camera);
	float4 header = rXform(trayHeaderRegion(tray), camera);
	if(rect_contains(region, p)) {
		if(rect_contains(header, p)) {
			tray->spacerSpot = -1;
		} else {
			p -= region.xy;
			if(tray->style == 0) {
				p.y -= header.w;
			} 
			float2 slotSize = tray->slotSize;
			if(uiTrayGutter) {
				slotSize.y += 16;
			}
			p /= slotSize * camera.scale;
			int2 ip = float2_to_int(p);
			tray->spacerSpot = ip.y * tray->size.x + ip.x;
		}
	}
	return tray->spacerSpot;
}


void traySort(UiTray* tray)
{
	for(int i = 1; i < tray->numSlots; ++i) {
		int j = i - 1;
		UiTrayEntry* entry = tray->slots[i];

		while(j >= 0 && tray->slots[j]->id > entry->id) {
			tray->slots[j + 1] = tray->slots[j];
			j--;
		}
		tray->slots[j + 1] = entry;
	}
}



int trayAdd(UiTray* tray, UiTrayEntry* entry, int index)
{
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
		tray->slots[tray->numSlots] = entry;
		tray->numSlots++;
		if(tray->value != -1) entry->b = tray->value;
		if(uiTrayKeepSorted) traySort(tray);
		return 0;
	}

	for(int i = tray->numSlots++; i >= index; --i) {
		tray->slots[i+1] = tray->slots[i];
	}

	tray->slots[index] = entry;
	if(tray->value != -1) entry->b = tray->value;

	if(uiTrayKeepSorted) {
		traySort(tray);
	}

	return 0;
}

void trayUpdateValues(UiTray* tray)
{
	if(tray->value == -1) return;
	for(int i = 0; i < tray->numSlots; ++i) {
		tray->slots[i]->b = tray->value;
	}
}

UiTrayEntry* trayGrab(UiTray* tray, int index) 
{
	if(index < 0 || index >= tray->maxSlots) {
		return nullptr;
	}

	UiTrayEntry* ret = tray->slots[index];
	tray->numSlots--;
	for(int i = index; i < tray->numSlots; ++i) {
		tray->slots[i] = tray->slots[i+1];
	}
	return ret;
}

enum {
	Filtermode_Name,
	Filtermode_Move
};
int filterMode = 0;
char filterBuf[64];
UiTextbox filterbox = {filterBuf, 0, 63, false};

bool checkFilter(const char* test, bool exact)
{
	if(filterbox.len > 0) {
		if(exact) {
			return stringDistance2(test, filterbox.str) == 0;
		} else {
			return stringDistance2(test, filterbox.str) <= 1;
		}
	} else {
		return true;
	}

}

bool uiTrayDrawCount = true;

void trayDraw(UiTray* tray, Xform camera, float4 interactionBox, bool usebox)
{
	float4 region = trayRegion(tray);
	float4 header = trayHeaderRegion(tray);
	region.y += header.w;
	region.w -= header.w;

	if(tray->spacerSpot != -1 && tray->highlightColor > 0 ) {
		drawRect(tray->highlightColor, f4tofr(region), camera);
	} else {
		drawRect(0x111111, f4tofr(region), camera);
	}
	if(tray->style == 0) {
		drawRect(0x444444, f4tofr(header), camera);
		if(strlen(tray->title) < 4) {
			wbsf_Scale = 2.0f;
		}
		drawText(tray->title, tray->pos + 4, -1, camera);
		wbsf_Scale = 1.0f;

		char buf[64];
		SDL_snprintf(buf, 64, "%d", tray->numSlots);
		drawText(buf, (float2){tray->pos.x + 4, tray->pos.y + 8 + GLYPH_H * 2}, -1, camera);
	}

	float2 mpos = Game->input->mpos;//pXformInv(Game->input->mpos, camera);
	float2 p = 0;
	int2 size = tray->size;
	int count = i32min(tray->numSlots, tray->size.x * tray->size.y);
	bool interactive = true;
	if(usebox && !rect_contains(interactionBox, mpos)) {
		interactive = false;
	}

	for(int i = 0; i < count; ++i) {
		UiTrayEntry* entry = tray->slots[i];
		MonDef* mon = &mons[entry->id];

		bool inFilter = true;
		if(filterbox.len > 0) {
			if(filterMode == Filtermode_Name) {
				inFilter = checkFilter(mon->name, false);
			} else if(filterMode == Filtermode_Move) {
				inFilter = false;
				for(int j = 0; j < 100; ++j) {
					if(mon->moves[j] == 0xFFFF) {
						break;
					}
					if(checkFilter(moves[mon->moves[j]].name, true)) {
						inFilter = true;
						break;
					}
				}
			}
		}

		entry->wasInFilter = inFilter;


		if(i == tray->spacerSpot) {
			p.x += tray->slotSize.x / 2;
		}
		SDL_FRect dst = pXformRect(region.xy + p, tray->slotSize, camera);

		if(((i/tray->size.x) + (i%tray->size.x)) % 2 == 1) {
			// draw a background on every other square
			drawRect(0x333333, dst, Xzero);
		}
		if(interactive && frect_contains(dst, mpos)) {
			if(!grabbedEntry && mbtnJustDown(SDL_BUTTON_LEFT)) {
				// grab entry
				int index = trayHover(tray, mpos, camera);
				grabbedEntry = trayGrab(tray, index);
				grabbedPoint = (mpos - (float2){dst.x, dst.y}) / camera.scale;
				grabOrigin = tray;
				grabbedIndex = i;
				lastEntry = grabbedEntry;
			}

			SDL_FRect bgsrc = getMonRect(255);
			SDL_RenderTexture(
				Game->renderer, 
				monTexture, 
				&bgsrc,
				&dst);
		}
		if(!inFilter) {
			SDL_SetTextureAlphaMod(entry->texture, 32);
		}
		SDL_RenderTexture(Game->renderer, entry->texture, &entry->src, &dst);
		if(!inFilter) {
			SDL_SetTextureAlphaMod(entry->texture, 255);
		}
		if(p.x + tray->slotSize.x >= tray->size.x * tray->slotSize.x - 1) {
			p.y += tray->slotSize.y;
			if(uiTrayGutter) {
				p.y += 16;
			}
			p.x = 0;
		} else {
			p.x += tray->slotSize.x;
		}
	}

	for(int i = count; i < (size.x * size.y); ++i) {
		SDL_FRect dst = pXformRect(region.xy + p, tray->slotSize, camera);
		if(((i/tray->size.x) + (i%tray->size.x)) % 2 == 1) {
			// draw a background on every other square
			drawRect(0x222222, dst, Xzero);
		}
		if(p.x + tray->slotSize.x >= tray->size.x * tray->slotSize.x - 1) {
			p.y += tray->slotSize.y;
			if(uiTrayGutter) {
				p.y += 16;
			}
			p.x = 0;
		} else {
			p.x += tray->slotSize.x;
		}
	}

	if(uiTrayDrawNames || uiTrayDrawBadges) {
		p = 0;
		for(int i = 0; i < count; ++i) {
			UiTrayEntry* entry = tray->slots[i];
			bool inFilter = entry->wasInFilter;
			if(i == tray->spacerSpot) {
				p.x += tray->slotSize.x / 2;
			}
			float2 lp = pXform(region.xy + p + (float2){0, tray->slotSize.y}, camera);
			lp.y -= GLYPH_H;
			if(uiTrayGutter) {
				lp.y += 16;
			}
			SDL_FRect bg = {lp.x, lp.y, GLYPH_W * strlen(mons[entry->id].name), GLYPH_H};
			if(inFilter && uiTrayDrawNames) {
				drawRect(0, bg, Xzero);
				drawText(mons[entry->id].name, lp, -1, Xzero);
			}
			if(inFilter && uiTrayDrawBadges) {
				if(!uiTrayDrawNames) {
					lp.y += 16;
				}
				float2 xp = 0;
				for(int j = 0; j < monHasMoves[entry->id].numMoves; ++j) {
					int badgeid = highlightedMoves[monHasMoves[entry->id].moves[j]].y;
					int badgex = badgeid & 3;
					int badgey = badgeid / 4;
					int2 pos = {badgex, badgey};
					pos *= 16;
					pos.y += 16 + 64 + 48;
					SDL_FRect dst = {xp.x + lp.x, xp.y + lp.y - 16, 16, 16};
					SDL_FRect src = {pos.x, pos.y, 16, 16};
					SDL_RenderTexture(Game->renderer, Game->texture, &src, &dst);
					xp.x += 16;
					if(xp.x >= (tray->slotSize.x * camera.scale) - 1) {
						xp.y -= 16;
						xp.x = 0;
					}
				}
			}

			if(p.x + tray->slotSize.x >= tray->size.x * tray->slotSize.x - 1) {
				p.y += tray->slotSize.y;
				if(uiTrayGutter) {
					p.y += 16;
				}
				p.x = 0;
			} else {
				p.x += tray->slotSize.x;
			}
		}
	}
	tray->spacerSpot = -1;
}

int uimode = 2;

UiTray* sourceTrays[8];
UiTray* trays[32];
int numTrays, numSourceTrays;


void addTray(UiTray* tray)
{
	if(numTrays >= SDL_arraysize(trays)) {
		return;
	}
	trays[numTrays++] = tray;
}

void addSourceTray(UiTray* tray)
{
	if(numSourceTrays >= SDL_arraysize(sourceTrays)) {
		return;
	}
	sourceTrays[numSourceTrays++] = tray;
}


int numTiers = 15;

void createTrays() 
{
	char buf[32];
	for(int i = 0; i < 5; ++i) {
		int2 size = {10, 16};
		if(i == 4) {
			size = (int2){4, 16};
		}
		UiTray* tray = createTray(sourceNames[i], size, 64);
		addSourceTray(tray);
	}

	for(int i = 0; i < numTiers; ++i) {
		SDL_snprintf(buf, 16, "%d", numTiers - i);

		int w = 2;
		if(numTiers - i <= 2) w *= 2;

		uint32_t index = 0;
		int ret = dict32Get(trayDict, buf, &index);
		if(ret == DICT32_OK) {
			int size = trayArraySizes[index];
			if(size <= 6) w = 1;
		}

		UiTray* tray = createTray(SDL_strndup(buf, 16), (int2){w, 12}, 64) ;
		tray->value = numTiers - i;
		trayUpdateValues(tray);
		addTray(tray);
	}

	for(int i = 0; i < numMons; ++i) {
		if(monWasLoaded[i]) continue;
		UiTrayEntry* entry = &entries[i];

		for(int j = 0; j < numTiers; ++j) {
			uint32_t index = 0;
			int ret = dict32Get(baseDict, trays[j]->title, &index);
			if(ret == DICT32_OK) {
				for(int k = 0; k < baseSizes[index]; ++k) {
					int mon = baseArrays[index][k]-1;
					if(mon == i && trays[j]->value == basePoints[index][k]) {
						monWasLoaded[i] = true;
						trayAdd(trays[j], entry, -1);
						break;
					}
				}
			}
			if(monWasLoaded[i]) break;
		}

		if(monWasLoaded[i]) continue;

		if(mons[i].preevo != -1 && evoTree[i] > 0) {
							// middle
			trayAdd(sourceTrays[2], entry, -1);
		} else if(mons[i].preevo != -1 && evoTree[i] == 0) {
			trayAdd(sourceTrays[1], entry, -1);
		} else if(mons[i].preevo == -1 && evoTree[i] == 0) {
			trayAdd(sourceTrays[0], entry, -1);
		} else if(mons[i].preevo == -1 && evoTree[i] > 0) {
			trayAdd(sourceTrays[3], entry, -1);
		} else {
			SDL_Log("can't place %s", mons[i].name);
		}
	}
}

bool use2xScale = false;

Xform camera = Xzero;
Xform sourceCam = Xzero;
bool sourcePan = false;
float2 cameraGrab, lastCamera;
bool cameraPan = false;
Xform ctrlCam = Xzero;

int draftTurn = 0;

bool wasGutters = true;


int getTeamTurn()
{
	int loop = draftTurn % (numTeams * 2);
	if(loop >= numTeams) {
		loop = numTeams - 1 - (loop - numTeams);
	}
	return loop;
}

void drawStatBlock(UiTrayEntry* etr, float2 fpanel)
{
	if(!etr) {
		return;
	}
	MonDef* mon = &mons[etr->id];
	SDL_FRect dst = pXformRect(fpanel.xy, 128, Xzero);
	SDL_FRect bgsrc = getMonRect(etr->id);
	SDL_RenderTexture(
		Game->renderer, 
		monTexture, 
		&bgsrc,
		&dst);
	fpanel.x += 128 + 8;

	drawText(mon->name, fpanel, -1, Xzero);
	const char* statNames[] = {
		"HP:   ",
		"Atk:  ",
		"Def:  ",
		"SpAtk:",
		"SpDef:",
		"Speed:"
	};

	float2 stp = fpanel.xy;
	stp.y += GLYPH_H;
	float right = 0;
	for(int i = 0; i < 6; ++i) {
		char buf[32]; 
		int w = snprintf(buf, 32, "%s %3d", statNames[i], mon->stats[i]);
		drawText(buf, stp, -1, Xzero);
		right = GLYPH_W * w + 1;
		drawRect(0x333333, pXformRect(stp + (float2){right, 2}, (float2){128, GLYPH_H - 4}, Xzero), Xzero); 

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

		drawRect(color, pXformRect(stp + (float2){right, 2}, (float2){128 * (mon->stats[i] / 255.0), GLYPH_H - 4}, Xzero), Xzero); 
		stp.y += GLYPH_H;
	}
}

int lastMove = -1;

void teamSelect(float2 p, int* teamOut)
{
	float titlew = 96;
	for(int i = 0; i < numTeams; ++i) {
		uint32_t color = 0x333333;

		if(pteams[i].tray) {
			pteams[i].tray->pos = p + (float2){titlew, 0};
			trayDraw(pteams[i].tray, camera, 0, true);
		}

		float4 region = {p.x, p.y, titlew + pteams[i].tray->slotSize.x * 10, pteams[i].tray->slotSize.y};

		if(rect_contains(region, Game->input->mpos)) {
			color = 0x44aa22;
			if(mbtnJustUp(SDL_BUTTON_LEFT)) {
				if(teamOut) *teamOut = i;
			}
		}

		drawRect(color, (SDL_FRect){p.x, p.y, titlew, 64}, camera);
		drawText(pteams[i].name, p + 4, titlew - 8, camera);


		p.y += 64 + 16; 

	}
}

typedef struct DamageResult
{
	int2 hprange;
	float2 percrange;
	int2 hitrange;
} DamageResult;

enum 
{
	Dmg_None = 1<<0,
	Dmg_Rain = 1<<1,
	Dmg_Sun = 1<<2,
	Dmg_ItemBoost20 = 1<<3,
	Dmg_ItemBoost10 = 1<<4,
	Dmg_Crit = 1<<5,
	Dmg_DoubleDamage = 1<<6,
	Dmg_Reflect = 1<<7,
	Dmg_LightScreen = 1<<8,
	Dmg_Explosion = 1<<9
};

bool checkType(int* types, int t)
{
	for(int i = 0; i < Num_Types; ++i) {
		if(types[i] == -1) break;
		if(types[i] == t) {
			return true;
		}
	}
	return false;
}

int maxStatExp[] = {
	65536,
	65536,
	65536,
	65536,
	65536,
	65536,
};

int maxDvs[] = {
	15,15,15,15,15,15
};

#include <math.h>

typedef struct BattleMon
{
	MonDef* def;
	int level;
	int stats[6];
	int boosts[6];
	int statxp[6];
	int dvs[6];
} BattleMon;

void calcStats(BattleMon* mon)
{
	for(int i = 0; i < 6; ++i) {

		int base = mon->def->stats[i] + mon->dvs[i];
		base *= 2;
		base += (int)(sqrtf(mon->statxp[i]) / 4.0);
		base *= mon->level;
		base /= 100;
		mon->stats[i] = base;
		mon->stats[i] += 5;
	}
	mon->stats[0] += mon->level + 5;
}

enum 
{
	Stat_HP,
	Stat_Atk,
	Stat_Def,
	Stat_SpAtk,
	Stat_SpDef,
	Stat_Speed,
};


DamageResult calcDamage(BattleMon* mon1, BattleMon* mon2, MoveDef* move, int flags)
{
	int category = TypeCategory[move->type];
	if(category == Category_Status || move->power == 0) {
		return (DamageResult){0, 0};
	}

	if(mon1->stats[0] == 0) {
		calcStats(mon1);
	}

	if(mon2->stats[0] == 0) {
		calcStats(mon2);
	}

	int atk = category == Category_Physical ? mon1->stats[Stat_Atk] : mon1->stats[Stat_SpAtk];
	int def = category == Category_Physical ? mon2->stats[Stat_Def] : mon2->stats[Stat_SpDef];

	int typeNum = 1;
	int typeDenom = 1;

	if(checkType(TypeWeaknesses[mon2->def->types[0]], move->type)) {
		typeNum *= 2;
	}
	if(checkType(TypeResists[mon2->def->types[0]], move->type)) {
		typeDenom *= 2;
	}
	if(checkType(TypeImmunities[mon2->def->types[0]], move->type)) {
		typeNum = 0;
	}

	if(mon2->def->types[0] != mon2->def->types[1]) {
		if(checkType(TypeWeaknesses[mon2->def->types[1]], move->type)) {
			typeNum *= 2;
		}
		if(checkType(TypeResists[mon2->def->types[1]], move->type)) {
			typeDenom *= 2;
		}
		if(checkType(TypeImmunities[mon2->def->types[1]], move->type)) {
			typeNum = 0;
		}
	}

	int base = (2 * mon1->level) / 5 + 2;
	base *= move->power;

	if(flags & Dmg_Explosion) {
		def /= 2;
	}

	if(flags & Dmg_LightScreen) {
		if(TypeCategory[move->type] == Category_Special) {
			def *= 2;
		}
	}
	if(flags & Dmg_Reflect) {
		if(TypeCategory[move->type] == Category_Physical) {
			def *= 2;
		}
	}

	base *= atk;
	base /= def;
	base /= 50;

	if(flags & Dmg_ItemBoost20) {
		base = (int)(base * 1.2);
	} else if(flags & Dmg_ItemBoost10) {
		base *= (int)(base * 1.1);
	}

	if(flags & Dmg_Crit) {
		base *= 2;
	}

	base += 2;

	if(flags & Dmg_Rain) {
		if(move->type == Type_Water) {
			base = (int)(base * 1.5);
		} else if(move->type == Type_Fire) {
			base = (int)(base * 0.5);
		} else if(strncmp(move->name, "Solarbeam", 16) == 0) {
			base = (int)(base * 0.5);
		}
	} else if(flags & Dmg_Sun) {
		if(move->type == Type_Water) {
			base = (int)(base * 0.5);
		} else if(move->type == Type_Fire) {
			base = (int)(base * 1.5);
		}
	}

	// stab
	if(mon1->def->types[0] == move->type) {
		base = (int)(base * 1.5);
	}

	if(mon1->def->types[0] != mon1->def->types[1]) {
		if(mon1->def->types[1] == move->type) {
			base = (int)(base * 1.5);
		}
	}

	base *= typeNum;
	base /= typeDenom;

	int low = base * 217 / 255;
	int high = base;

	DamageResult res;
	res.hprange.x = low;
	res.hprange.y = high;
	res.percrange.x = (float)low / mon2->stats[Stat_HP];
	res.percrange.y = (float)high / mon2->stats[Stat_HP];

	res.hitrange.x = res.percrange.x > 0 ? (int)(1.0 / res.percrange.x) + 1 : 999;
	res.hitrange.y = res.percrange.y > 0 ? (int)(1.0 / res.percrange.y) + 1 : 999;

	return res;
}

int needUpdate = 2;
void testUpdate(GameState* state, GameContext* game)
{
	if(needUpdate < 0) return;
	SDL_RenderClear(Game->renderer);
	needUpdate--;
	float2 buttonp = 0;
	if(uiButton("Config", buttonp, false, Xzero)) {
		camera = Xzero;
		cameraPan = false;
		sourcePan = false;
		uimode = 2;
	}
	buttonp.x += sizeof("Config  ") * GLYPH_W;
	if(uiButton("Draft", buttonp, false, Xzero)) {
		camera = Xzero;
		if(use2xScale) {
			sourceCam = Xdouble;
		} else {
			sourceCam = Xzero;
		}
		cameraPan = false;
		sourcePan = false;
		uimode = 0;

		for(int i = 0; i < numTeams; ++i) {
			PlayerTeam* team = &pteams[i];
			if(!team->tray) {
				team->tray = createTray(team->name, (int2){10, 1}, 64);
				team->tray->style = 1; // no header;
				team->tray->highlightColor = 0x555555;
			}
		}

		for(int i = 0; i < numTrays; ++i) {
			traySort(trays[i]);
		}
	}
	buttonp.x += sizeof("DRAFT  ") * GLYPH_W;
	if(uiButton("Points", buttonp, false, Xzero)) {
		camera = Xzero;
		sourceCam = Xzero;
		cameraPan = false;
		sourcePan = false;
		uimode = 1;
	}
	buttonp.x += sizeof("POINTS  ") * GLYPH_W;

	if(uiButton("Stat Tiers", buttonp, false, Xzero)) {
		camera = Xzero;
		sourceCam = Xzero;
		cameraPan = false;
		sourcePan = false;
		uimode = 4;
	}
	buttonp.x += sizeof("Stat Tiers  ") * GLYPH_W;

	if(uiButton("Dmg Calc", buttonp, false, Xzero)) {
		camera = Xzero;
		sourceCam = Xzero;
		cameraPan = false;
		sourcePan = false;
		uimode = 5;
		for(int i = 0; i < numTeams; ++i) {
			PlayerTeam* team = &pteams[i];
			if(!team->tray) {
				team->tray = createTray(team->name, (int2){10, 1}, 64);
				team->tray->style = 1; // no header;
				team->tray->highlightColor = 0x555555;
			}
		}
		wasGutters = uiTrayGutter;
		uiTrayGutter = false;
	}
	buttonp.x += sizeof("Stat Tiers  ") * GLYPH_W;

	buttonp.x += sizeof("      ") * GLYPH_W;
	if(uiButton(uiTrayDrawNames ? "Toggle Names(^)":"Toggle Names(_)", buttonp, false, Xzero)) {
		uiTrayDrawNames = !uiTrayDrawNames;
	}
	buttonp.x += sizeof("Toggle Names(.)...") * GLYPH_W;
	if(uiButton(uiTrayGutter ? "Toggle Gutter(^)":"Toggle Gutter(_)", buttonp, false, Xzero)) {
		uiTrayGutter = !uiTrayGutter;
	}
	buttonp.x += sizeof("Toggle Gutter(.)...") * GLYPH_W;
	if(uiButton(uiTrayDrawBadges ? "Toggle Badges(^)":"Toggle Badges(_)", buttonp, false, Xzero)) {
		uiTrayDrawBadges = !uiTrayDrawBadges;
	}
	buttonp.x += sizeof("Toggle Badges(.)...") * GLYPH_W;

	if(uiButton(use2xScale ? "2X Scale(^)":"2X Scale(_)", buttonp, false, Xzero)) {
		use2xScale = !use2xScale;
		int diff = -1;
		if(use2xScale) {
			wasGutters = uiTrayGutter;
			uiTrayGutter = false;
			diff = 1;
		} else {
			uiTrayGutter = wasGutters;
		}
		for(int i = 0; i < numTrays; ++i) {
			trays[i]->size.x += diff;
		}

	}
	buttonp.x += sizeof("2X Scale...  ") * GLYPH_W;

	float2 p = {16, 32};

	// config interface
	// 1. set up teams
	// 2. configure point total
	if(uimode == 2) {
		float2 bbp = p;
		uiLabel("Point Total:", bbp, false, camera);
		bbp.x += GLYPH_W * sizeof("point total:  ");
		int lastPointTotal = teamPointTotal;
		if(uiButton("+5", bbp, false, camera)) {
			teamPointTotal += 5;
		}
		bbp.x += sizeof("..  ") * GLYPH_W;
		if(uiButton("+1", bbp, false, camera)) {
			teamPointTotal += 1;
		}
		bbp.x += sizeof("....  ") * GLYPH_W;

		char pbuf[64];
		snprintf(pbuf, 16, "%d", teamPointTotal);
		uiLabel(pbuf, bbp, false, camera);
		bbp.x += sizeof("..  ") * GLYPH_W;

		if(uiButton("-1", bbp, false, camera)) {
			teamPointTotal += -1;
		}
		bbp.x += sizeof("..  ") * GLYPH_W;
		if(uiButton("-5", bbp, false, camera)) {
			teamPointTotal += -5;
		}
		bbp.x += sizeof("..  ") * GLYPH_W;
		if(uiButton("Reset", bbp, false, camera)) {
			teamPointTotal = 75;
		}
		bbp.x += sizeof("..  ") * GLYPH_W;

		if(teamPointTotal != lastPointTotal) {
			for(int i = 0; i < numTeams; ++i) {
				pteams[i].points = teamPointTotal;
			}
		}

		p.y += GLYPH_H;
		if(uiButton("Shuffle Teams", p, false, camera)) {
			rng_state rng;
			init_rng(&rng, time(0));
			for(int iters = 0; iters < 4; ++iters) {
				for(int i = 0; i < numTeams; ++i) {
					int swap = i32abs(i32rand(&rng)) % numTeams;
					if(swap > SDL_arraysize(pteams)) {
						i--;
						continue;
					}

					PlayerTeam at = pteams[swap];
					pteams[swap] = pteams[i];
					pteams[i] = at;
				}
			}
		}
		p.y += 32;
		static char boxbuf[36] = {};
		static UiTextbox teambox = {boxbuf, 0, 32, false};
		uiTextbox("Name: ", p, (int2){32,1}, &teambox, camera);
		p.y += 24;
		if(uiButton("Add Team", p, false, camera)) {
			if(teambox.len > 0) {
				addTeam(SDL_strndup(teambox.str, 33));
				memset(teambox.str, 0, 36);
				teambox.len = 0;
			}
		}

		snprintf(pbuf, 64, "Num Teams: %d", numTeams);
		uiLabel(pbuf, p + (float2){sizeof("Add Team..  "), 0} * GLYPH_W,  false, camera);
		if(teambox.hasfocus && teambox.len > 0 && keyJustDown(SDL_SCANCODE_RETURN)) {
			addTeam(SDL_strndup(teambox.str, 33));
			memset(teambox.str, 0, 36);
			teambox.len = 0;
		}
		p.y += 32;

		for(int i = 0; i < numTeams; ++i) {
			float2 lp = p + (float2){0, 80 * i};
			drawRect(0x333333, (SDL_FRect){lp.x, lp.y, 128, 64}, camera);
			drawText(pteams[i].name, lp + 4, 128 - 8, camera);

			float2 bp = lp;
			bp.x += 128 + 16;
			bp.y += 64.0f / 2 - GLYPH_H / 2.0f;
			if(uiButton("Move Up", bp, false, camera)) {
				if(i > 0) {
					int swap = i - 1;
					PlayerTeam at = pteams[swap];
					pteams[swap] = pteams[i];
					pteams[i] = at;
					break;
				}
			}
			bp.x += sizeof("MOVE UP  ") * GLYPH_W;
			if(uiButton("Move Down", bp, false, camera)) {
				if(i < numTeams - 1) {
					int swap = i + 1;
					PlayerTeam at = pteams[swap];
					pteams[swap] = pteams[i];
					pteams[i] = at;
					break;
				}
			}
			bp.x += sizeof("MOVE Down  ") * GLYPH_W;
			if(uiButton("Rename", bp, false, camera)) {
				if(teambox.len > 0) {
					if(pteams[i].name) {
						SDL_free((void*)pteams[i].name);
					}
					pteams[i].name = (SDL_strndup(teambox.str, 33));
					memset(teambox.str, 0, 36);
					teambox.len = 0;
				}
			}
			bp.x += sizeof("Rename  ") * GLYPH_W;
			if(uiButton("Remove", bp, false, camera)) {
				for(int j = 0; j < pteams[i].tray->numSlots; ++j) {
					for(int k = 0; k < numTrays; ++k) {
						UiTray* tray = trays[k];
						if(tray->value == pteams[i].tray->slots[j]->b) {
							trayAdd(tray, pteams[i].tray->slots[j], -1);
							break;
						}
					}
				}

				free(pteams[i].tray->slots);
				free(pteams[i].tray);

				dict32Delete(trayDict, pteams[i].name);

				--numTeams;
				for(int j = i; j < numTeams; ++j)  {
					pteams[j] = pteams[j+1];
				}

				break;
			}
			bp.x += sizeof("Remove  ") * GLYPH_W;
		}
		p.y += 64 + 16;
	}

	// Draft interface
	if(uimode == 0) {
		float2 rpanel = 0;
		float titlew = 96;
		rpanel.x = 16 + 10 * 64 + (16 + titlew);
		rpanel.y = 128 + 16 + 16;
		float4 rpanel_r;
		rpanel_r.xy = rpanel;
		rpanel_r.zw = Game->windowSize - rpanel;

		if(use2xScale) {
			sourceCam.scale = 2;
			rpanel.y += 32;
		} else {
			sourceCam.scale = 1;
		}
		rpanel.xy /= sourceCam.scale;

		float2 xoff = 0;
		for(int i = 0; i < numTrays; ++i) {
			UiTray* tray = trays[i];
			tray->pos = rpanel + xoff;
			xoff.x += tray->size.x * tray->slotSize.x + 16 / sourceCam.scale;
			tray->style = sourceCam.scale == 2.0f ? 1 : 0;

			if(sourceCam.scale == 2.0) {
				float2 p = tray->pos + (float2){0, -GLYPH_H};
				drawRect(0x333333, pXformRect(p, (float2){tray->size.x * tray->slotSize.x, 32} , Xzero), sourceCam);
				drawText(tray->title, p, -1, sourceCam);
			}

			trayDraw(tray, sourceCam, rpanel_r, true);
		}

		rpanel.x *= sourceCam.scale;

		drawRect(0, (SDL_FRect){0, 32, rpanel.x, Game->windowSize.y-32}, Xzero);

		float2 fpanel = rpanel;
		fpanel.y = 32;
		uiTextbox("Filter: ", fpanel, (int2){20, 1}, &filterbox, Xzero);
		fpanel.y += 24;
		if(uiButton(filterMode == Filtermode_Name ? "* Filter by name": "  Filter by name", fpanel, false, Xzero)) {
			filterMode = Filtermode_Name;
		}
		fpanel.y += GLYPH_H;
		if(uiButton(filterMode == Filtermode_Move ? "* Filter by move": "  Filter by move", fpanel, false, Xzero)) {
			filterMode = Filtermode_Move;
		}
		fpanel.y += GLYPH_H;
		fpanel.y += GLYPH_H;
		if(uiButton("Clear Filter", fpanel, false, Xzero)) {
			filterbox.len = 0;
			memset(filterBuf, 0, sizeof(filterBuf));
		}

		fpanel.y = 32;
		fpanel.x += 22 * GLYPH_W;

		if(lastEntry) {
			MonDef* mon = &mons[lastEntry->id];
			SDL_FRect dst = pXformRect(fpanel.xy, 128, Xzero);
			SDL_FRect bgsrc = getMonRect(lastEntry->id);
			SDL_RenderTexture(
				Game->renderer, 
				monTexture, 
				&bgsrc,
				&dst);
			fpanel.x += 128 + 8;
			
			drawText(mon->name, fpanel, -1, Xzero);
			const char* statNames[] = {
				"HP:   ",
				"Atk:  ",
				"Def:  ",
				"SpAtk:",
				"SpDef:",
				"Speed:"
			};

			float2 stp = fpanel.xy;
			stp.y += GLYPH_H;
			float right = 0;
			for(int i = 0; i < 6; ++i) {
				char buf[32]; 
				int w = snprintf(buf, 32, "%s %3d", statNames[i], mon->stats[i]);
				drawText(buf, stp, -1, Xzero);
				right = GLYPH_W * w + 1;
				drawRect(0x333333, pXformRect(stp + (float2){right, 2}, (float2){128, GLYPH_H - 4}, Xzero), Xzero); 

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

				drawRect(color, pXformRect(stp + (float2){right, 2}, (float2){128 * (mon->stats[i] / 255.0), GLYPH_H - 4}, Xzero), Xzero); 
				stp.y += GLYPH_H;
			}

			stp.y = fpanel.y + 16;
			stp.x += 128 + right + 16;


			UiTrayEntry* entry = lastEntry;
			float2 xp = 0;
			float2 lp = stp;
			float xmax = 0;
			for(int j = 0; j < monHasMoves[entry->id].numMoves; ++j) {
				int2 movepair = highlightedMoves[monHasMoves[entry->id].moves[j]];
				int badgeid = movepair.y;
				int badgex = badgeid & 3;
				int badgey = badgeid / 4;
				int2 pos = {badgex, badgey};
				pos *= 16;
				pos.y += 16 + 64 + 48;
				SDL_FRect dst = {xp.x + lp.x, xp.y + lp.y, 32, 32};
				SDL_FRect src = {pos.x, pos.y, 16, 16};
				SDL_RenderTexture(Game->renderer, Game->texture, &src, &dst);

				float4 region = drawText(moves[movepair.x].name, lp + xp + (float2){32 + 8, 8}, -1, Xzero);
				xmax = f32max(region[2], xmax);

				xp.y += 32;
				if(xp.y >= (32 * 3) - 1) {
					xp.y = 0;
					xp.x += xmax + 32 + 16;
					xmax = 0;
				}

			}
		}

		float2 bp = {16, 32};
		if(uiButton("Next Turn", bp, false, camera)) {
			draftTurn++;
		}
		bp.x += sizeof("Next Turn...") * GLYPH_W;
		if(uiButton("Prev Turn", bp, false, camera)) {
			draftTurn--;
		}
		bp.x += sizeof("Next Turn...") * GLYPH_W;
		char turnbuf[32];
		SDL_snprintf(turnbuf, 32, "Turn Index: %d", draftTurn);
		uiLabel(turnbuf, bp, false, camera);


		for(int i = 0; i < numTeams; ++i) {
			p = (float2){16, 64 + 80 * i};
			uint32_t color = 0x333333;
			if(i == getTeamTurn()) {
				color = 0x44aa22;
			}
			int points = pteams[i].points;
			for(int j = 0; j < pteams[i].tray->numSlots; ++j) {
				points -= pteams[i].tray->slots[j]->b;
			}
			if(points < 0) {
				color = 0xFF0000;
			}
			drawRect(color, (SDL_FRect){p.x, p.y, titlew, 64}, camera);
			drawText(pteams[i].name, p + 4, titlew - 8, camera);


			char pointsbuf[32];
			snprintf(pointsbuf, 32, "%d/%d", points, 10 - pteams[i].tray->numSlots);
			wbsf_Scale = 2.0f;
			drawText(pointsbuf, p + 4 + (float2){0, GLYPH_H + 4}, -1, camera);
			wbsf_Scale = 1.0f;

			if(pteams[i].tray) {
				pteams[i].tray->pos = p + (float2){titlew, 0};
				trayDraw(pteams[i].tray, camera, 0, false);
			}
		}

		if(!grabbedEntry) {
			if(rect_contains(rpanel_r, Game->input->mpos) && mbtnJustDown(SDL_BUTTON_LEFT)) {
				sourcePan = true;
				lastCamera = sourceCam.pos;
				cameraGrab = Game->input->mpos;
			} else if(mbtnJustDown(SDL_BUTTON_LEFT)) {
				lastCamera = camera.pos;
				cameraPan = true;
				cameraGrab = Game->input->mpos;
			}
			if(cameraPan) {
				camera.pos.y = (cameraGrab.y - Game->input->mpos.y) / camera.scale + lastCamera.y;
			}
			if(sourcePan) {
				sourceCam.pos.x = (cameraGrab.x - Game->input->mpos.x)/sourceCam.scale + lastCamera.x;
			}
		}
	}

	// Point assignment
	if(uimode == 1) {

		const char* message = "Hold SPACE to access unassigned pokemon";
		float2 ppp = (float2){Game->windowSize.x - GLYPH_W * (strlen(message)+2), 16};
		uiLabel(message, ppp, false, Xzero);
		ppp.x -= sizeof("Reset All Pokemon...") * GLYPH_W;

		if(uiButton("Reset All Pokemon", ppp, false, Xzero)) {
			for(int i = 0; i < numTrays; ++i) {
				trays[i]->numSlots = 0;
			}
			for(int i = 0; i < numMons; ++i) {
				UiTrayEntry* entry = &entries[i];
				if(mons[i].preevo != -1 && evoTree[i] > 0) {
							// middle
					trayAdd(sourceTrays[2], entry, -1);
				} else if(mons[i].preevo != -1 && evoTree[i] == 0) {
					trayAdd(sourceTrays[1], entry, -1);
				} else if(mons[i].preevo == -1 && evoTree[i] == 0) {
					trayAdd(sourceTrays[0], entry, -1);
				} else if(mons[i].preevo == -1 && evoTree[i] > 0) {
					trayAdd(sourceTrays[3], entry, -1);
				} else {
					SDL_Log("can't place %s", mons[i].name);
				}
			}
		}

		if(numTrays == 0) {

			if(uiButton("-", p, false, camera)) {
				numTiers--;
			}
			if(uiButton("+", p + (float2){GLYPH_W*7, 0}, false, camera)) {
				numTiers++;
			}
			char buf[16];
			SDL_snprintf(buf, 16, "%d", numTiers);
			uiLabel(buf, p + (float2){GLYPH_W * (3 + 1), 0}, false, camera);

			if(uiButton("Create Tiers", p + (float2){GLYPH_W * 14, 0}, false, camera)) {
				createTrays();
			}
		}

		bool showSource = keyDown(SDL_SCANCODE_SPACE);


		float4 top_r = {0, 0, Game->windowSize.x, Game->windowSize.y / 3};

		float xoff = 0;
		for(int i = 0; i < numTrays; ++i) {
			UiTray* tray = trays[i];
			if(!tray->visible) continue;
			tray->pos = (float2){p.x + xoff, p.y};
			xoff += tray->size.x * tray->slotSize.x + 16;
			trayDraw(tray, camera, top_r, showSource);
		}

		if(showSource) {
			if(trays[0])
				p.y = Game->windowSize.y / 3;
			xoff = 0;
			for(int i = 0; i < numSourceTrays; ++i) {
				UiTray* tray = sourceTrays[i];
				if(!tray->visible) continue;
				tray->pos = (float2){p.x + xoff, p.y};
				xoff += tray->size.x * tray->slotSize.x + 16;
				trayDraw(tray, sourceCam, 0, false);
			}
		}
	
	}

	// Speed Tiers
	// or, stat-sorted?
	if(uimode == 4) {
		float4 rpanel = {Game->windowSize.x - 384, 32, 384, Game->windowSize.y - 32};
		float2 bp = {16, 32};
		static int stat = 5;
		const char* statNames[] = {
			"HP",
			"Atk",
			"Def",
			"SpAtk",
			"SpDef",
			"Speed"
		};
		int lastStat = stat;
		for(int i = 0; i < 6; ++i) {
			char buf[64];
			int len = snprintf(buf, 64, "%s(%s)", statNames[i], stat == i ? "^" : "_");
			if(uiButton(buf, bp, false, Xzero)) {
				stat = i;
			} 
			bp.x += (len+3) * GLYPH_W;
		}
		if(stat != lastStat) {
			monSortStat(monsBySpeed, numMons, stat);
		}
		int lastColumnStat = monsBySpeed[0].stats[stat];
		float2 p = {0, 0}, off = {16, 48 + 32};
		char buf[64];
		snprintf(buf, 64, "%d", lastColumnStat);
		wbsf_Scale = 2.0f;
		drawText(buf, p + off + (float2){0, -GLYPH_H*2}, -1, camera);
		for(int i = 0; i < numMons; ++i) {
			if(monsBySpeed[i].stats[stat] < lastColumnStat - 5) {
				p.x += 64 + 32;
				p.y = 0;
				lastColumnStat = monsBySpeed[i].stats[stat];
				char buf[64];
				snprintf(buf, 64, "%d", lastColumnStat);
				drawText(buf, p + off + (float2){0, -GLYPH_H*2}, -1, camera);
			}

			if(p.y + 64 > Game->windowSize.y - off.y) {
				p.x += 64;
				p.y = 0;
			}

			SDL_FRect dst = pXformRect(p + off, 64, camera);
			SDL_FRect bgsrc = getMonRect(monsBySpeed[i].preevo);
			SDL_RenderTexture(
				Game->renderer, 
				monTexture, 
				&bgsrc,
				&dst);

			if(!rect_contains(rpanel, Game->input->mpos) && frect_contains(dst, Game->input->mpos)) {
				bgsrc = getMonRect(255);
				SDL_RenderTexture(
					Game->renderer, 
					monTexture, 
					&bgsrc,
					&dst);
				if(mbtnDown(SDL_BUTTON_LEFT)) {
					lastEntry = &entries[monsBySpeed[i].preevo];
					lastMove = -1;
				}

			}

			p.y += 64;
		}
		wbsf_Scale = 1.0f;

		drawRect(0, f4tofr(rpanel), Xzero);

		drawStatBlock(lastEntry, rpanel.xy + (float2){16, 0});
		float2 mp = rpanel.xy + (float2){16, 128 + 64 + 16};
		float2 omp = mp;
		if(lastEntry) {
			MonDef* mon = &mons[lastEntry->id];
			for(int i = 0; i < 100; ++i) {
				if(mon->moves[i] == 0xFFFF) continue;
				char buf[256];
				snprintf(buf, 256, "%2d: %s", i, moves[mon->moves[i]].name);
				float4 region = drawText(buf, mp, -1, Xzero);
				region.xy = mp;
				mp.y += GLYPH_H;
				if(rect_contains(region, Game->input->mpos)) {
					drawRectAlpha(0xFFFFFF, 0.25, f4tofr(region), Xzero);
					if(mbtnDown(SDL_BUTTON_LEFT)) {
						lastMove = mon->moves[i];
					}
				}

				if(mp.y + GLYPH_H > Game->windowSize.y) {
					mp.y = omp.y;
					mp.x += 16 * GLYPH_W;
				}
			}

			/*
			typedef struct MoveDef
			{
				char name[16];
				char effect[16];
				uint8_t accuracy;
				uint8_t power;
				int8_t priority;
				uint8_t type;
				uint8_t chance;
				uint8_t pp;
				uint8_t reserved[2];
			} MoveDef;
			*/

			if(lastMove != -1) {
				mp = rpanel.xy + (float2){16, 128};
				MoveDef* move = &moves[lastMove];
				char buf[256];
				snprintf(buf, 256, "%s:\npow:%d / acc:%d%%\nprio:%d effect:%d%%\npp: %d",
					move->name,
					move->power,
					move->accuracy,
					move->priority,
					move->chance,
					move->pp);
				float4 region = drawText(buf, mp, 384-32, Xzero);
				drawText(move->desc, mp + (float2){region[2]+32, 0}, 384-32, Xzero);
			}
		}


		if(!rect_contains(rpanel, Game->input->mpos) &&  mbtnJustDown(SDL_BUTTON_LEFT)) {
			lastCamera = camera.pos;
			cameraPan = true;
			cameraGrab = Game->input->mpos;
		}

		if(cameraPan) {
			camera.pos.x = cameraGrab.x - Game->input->mpos.x + lastCamera.x;
		}

	}


	if(uimode == 5) {
		static int team1 = -1, team2 = -1;
		static UiTrayEntry* team1Entry, *team2Entry;
		static DamageResult cachedDamage1[100], cachedDamage2[100];
		static bool dmgInvalid1 = false, dmgInvalid2 = false;
		static int lastMove1 = -1, lastMove2 = -1;

		if(team1 == -1) {
			teamSelect((float2){16, 32}, &team1);
		} else {
			int teamId = team1;
			UiTrayEntry** lentry = &team1Entry;

			if(uiButton("Change Team", (float2){16, 32}, false, Xzero)) {
				team1 = -1;
			}

			float2 p = {16, 64};
			for(int i = 0; i < pteams[teamId].tray->numSlots; ++i) {
				UiTrayEntry* entry = pteams[teamId].tray->slots[i];
				SDL_FRect dst = pXformRect(p, 64, Xzero);
				SDL_FRect bgsrc = getMonRect(entry->id);
				SDL_RenderTexture(
					Game->renderer, 
					monTexture, 
					&bgsrc,
					&dst);
				p.y += 64;
				if(frect_contains(dst, Game->input->mpos)) {
					bgsrc = getMonRect(255);
					SDL_RenderTexture(
						Game->renderer, 
						monTexture, 
						&bgsrc,
						&dst);
					if(*lentry != entry && mbtnJustDown(SDL_BUTTON_LEFT)) {
						*lentry = entry;
						lastMove1 = -1;
						dmgInvalid1 = true;
						dmgInvalid2 = true;
					}
				}
			}

			if(*lentry) {
				if(dmgInvalid1 && team2Entry)  {
					dmgInvalid1 = false;
					// do damage calcs
					UiTrayEntry* entry = *lentry;
					MonDef* mon = &mons[entry->id];

					BattleMon bmon1 = {
						.def = mon,
						100,
						{},
						{},
						{65536, 65536, 65536, 65536, 65536, 65536},
						{15, 15, 15, 15, 15, 15}
					}; 

					BattleMon bmon2 = {
						.def = &mons[team2Entry->id],
						100,
						{},
						{},
						{65536, 65536, 65536, 65536, 65536, 65536},
						{15, 15, 15, 15, 15, 15}
					}; 


					for(int i = 0; i < 100; ++i) {
						if(mon->moves[i] == 0xFFFF) continue;
						cachedDamage1[i] = calcDamage(&bmon1, &bmon2, &moves[mon->moves[i]], 0);
					}


				}

				p = (float2){16 + 64, 32};
				float2 rpanel = p;
				UiTrayEntry* entry = *lentry;
				drawStatBlock(entry, rpanel.xy);
				MonDef* mon = &mons[entry->id];
				float2 mp = p + (float2){16, 128 + 80};
				float2 omp = mp;
				for(int i = 0; i < 100; ++i) {
					if(mon->moves[i] == 0xFFFF) continue;
					char buf[256];
					MoveDef* move = &moves[mon->moves[i]];
					if(move->power > 0 && cachedDamage1[i].hprange.x != 0) {
						snprintf(buf, 256, "%s (%d-%dhko)", moves[mon->moves[i]].name,
							cachedDamage1[i].hitrange.x, cachedDamage1[i].hitrange.y);
					} else {
						snprintf(buf, 256, "%s", moves[mon->moves[i]].name);
					}
					float4 region = drawText(buf, mp, -1, Xzero);
					region.xy = mp;
					mp.y += GLYPH_H;
					if(rect_contains(region, Game->input->mpos)) {
						drawRectAlpha(0xFFFFFF, 0.25, f4tofr(region), Xzero);
						if(mbtnDown(SDL_BUTTON_LEFT)) {
							lastMove1 = i;
						}
					}

					if(mp.y + GLYPH_H > Game->windowSize.y) {
						mp.y = omp.y;
						mp.x += 24 * GLYPH_W;
					}
				}

				if(lastMove1 != -1) {
					mp = rpanel.xy + (float2){16, 128};
					MoveDef* move = &moves[mon->moves[lastMove1]];
					char buf[256];

					snprintf(buf, 256, "%s:\npow:%d / acc:%d%%\nprio:%d effect:%d%%\npp: %d",
						move->name,
						move->power,
						move->accuracy,
						move->priority,
						move->chance,
						move->pp);
					float4 region = drawText(buf, mp, 384-32, Xzero);
					float4 region2 = drawText(move->desc, mp + (float2){region[2]+32, 0}, 384-32, Xzero);

					DamageResult dr = cachedDamage1[lastMove1];
					snprintf(buf, 256, "%.2f%%-%.2f%%\n%d-%dhp\n%d-%dhko", 
						dr.percrange[0] * 100, dr.percrange[1] * 100,
						dr.hprange[0], dr.hprange[1],
						dr.hitrange[0], dr.hitrange[1]);
					drawText(buf, mp + (float2){region[2]+32 + region2[2]+16, 0}, 384-32, Xzero);
				}
			}
		}

		float ww = Game->windowSize.x;
		if(team2 == -1) {
			teamSelect((float2){Game->windowSize.x - 10 * 64 - 96 - 16, 32}, &team2);
		} else {
			int teamId = team2;
			UiTrayEntry** lentry = &team2Entry;
			if(uiButton("Change Team", (float2){ww - sizeof("Change Team") * (GLYPH_W + 4), 32}, false, Xzero)) {
				team2 = -1;
			}

			float2 p = {ww - 16 - 64, 64};
			for(int i = 0; i < pteams[teamId].tray->numSlots; ++i) {
				UiTrayEntry* entry = pteams[teamId].tray->slots[i];
				SDL_FRect dst = pXformRect(p, 64, Xzero);
				SDL_FRect bgsrc = getMonRect(entry->id);
				SDL_RenderTexture(
					Game->renderer, 
					monTexture, 
					&bgsrc,
					&dst);
				p.y += 64;
				if(frect_contains(dst, Game->input->mpos)) {
					bgsrc = getMonRect(255);
					SDL_RenderTexture(
						Game->renderer, 
						monTexture, 
						&bgsrc,
						&dst);
					if(*lentry != entry && mbtnJustDown(SDL_BUTTON_LEFT)) {
						lastMove2 = -1;
						*lentry = entry;
						dmgInvalid2 = true;
						dmgInvalid1 = true;
					}
				}
			}

			// TODO fix big code duplication :(
			if(*lentry) {
				if(dmgInvalid2 && team1Entry)  {
					dmgInvalid2 = false;
					// do damage calcs
					UiTrayEntry* entry = *lentry;
					MonDef* mon = &mons[entry->id];

					BattleMon bmon1 = {
						.def = mon,
						100,
						{},
						{},
						{65536, 65536, 65536, 65536, 65536, 65536},
						{15, 15, 15, 15, 15, 15}
					}; 

					BattleMon bmon2 = {
						.def = &mons[team1Entry->id],
						100,
						{},
						{},
						{65536, 65536, 65536, 65536, 65536, 65536},
						{15, 15, 15, 15, 15, 15}
					}; 


					for(int i = 0; i < 100; ++i) {
						if(mon->moves[i] == 0xFFFF) continue;
						cachedDamage2[i] = calcDamage(&bmon1, &bmon2, &moves[mon->moves[i]], 0);
					}
				}

				p = (float2){ww - 640 + 64, 32};
				float2 rpanel = p;
				UiTrayEntry* entry = *lentry;
				drawStatBlock(entry, rpanel.xy);
				MonDef* mon = &mons[entry->id];
				float2 mp = p + (float2){16, 128 + 80};
				float2 omp = mp;
				for(int i = 0; i < 100; ++i) {
					if(mon->moves[i] == 0xFFFF) continue;
					char buf[256];
					MoveDef* move = &moves[mon->moves[i]];
					if(move->power > 0 && cachedDamage2[i].hprange.x != 0) {
						snprintf(buf, 256, "%s (%d-%dhko)", moves[mon->moves[i]].name,
							cachedDamage2[i].hitrange.x, cachedDamage2[i].hitrange.y);
					} else {
						snprintf(buf, 256, "%s", moves[mon->moves[i]].name);
					}
					float4 region = drawText(buf, mp, -1, Xzero);
					region.xy = mp;
					mp.y += GLYPH_H;
					if(rect_contains(region, Game->input->mpos)) {
						drawRectAlpha(0xFFFFFF, 0.25, f4tofr(region), Xzero);
						if(mbtnDown(SDL_BUTTON_LEFT)) {
							lastMove2 = i;//mon->moves[i];
						}
					}

					if(mp.y + GLYPH_H > Game->windowSize.y) {
						mp.y = omp.y;
						mp.x += 24 * GLYPH_W;
					}
				}

				/*
				if(lastMove2 != -1) {
					mp = rpanel.xy + (float2){16, 128};
					MoveDef* move = &moves[lastMove2];
					char buf[256];

					snprintf(buf, 256, "%s:\npow:%d / acc:%d%%\nprio:%d effect:%d%%\npp: %d",
						move->name,
						move->power,
						move->accuracy,
						move->priority,
						move->chance,
						move->pp);
					float4 region = drawText(buf, mp, 384-32, Xzero);
					drawText(move->desc, mp + (float2){region[2]+32, 0}, 384-32, Xzero);
				}
				*/

				if(lastMove2 != -1) {
					mp = rpanel.xy + (float2){16, 128};
					MoveDef* move = &moves[mon->moves[lastMove2]];
					char buf[256];

					snprintf(buf, 256, "%s:\npow:%d / acc:%d%%\nprio:%d effect:%d%%\npp: %d",
						move->name,
						move->power,
						move->accuracy,
						move->priority,
						move->chance,
						move->pp);
					float4 region = drawText(buf, mp, 384-32, Xzero);
					float4 region2 = drawText(move->desc, mp + (float2){region[2]+32, 0}, 384-32, Xzero);

					DamageResult dr = cachedDamage2[lastMove2];
					snprintf(buf, 256, "%.2f%%-%.2f%%\n%d-%dhp\n%d-%dhko", 
						dr.percrange[0] * 100, dr.percrange[1] * 100,
						dr.hprange[0], dr.hprange[1],
						dr.hitrange[0], dr.hitrange[1]);
					drawText(buf, mp + (float2){region[2]+32 + region2[2]+16, 0}, 384-32, Xzero);
				}
			}
		}
	}

	bool madeChange = false;

	if(grabbedEntry) {
		UiTrayEntry* entry = grabbedEntry;
		float2 mpos = Game->input->mpos;
		SDL_FRect dst = pXformRect(mpos - grabbedPoint, (float2){entry->src.w, entry->src.h}, Xzero);

		drawRect(0x333333, dst, Xzero);
		SDL_RenderTexture(
			Game->renderer, 
			Game->texture, 
			&(SDL_FRect){0, 16, 64, 64}, 
			&dst);
		SDL_RenderTexture(
			Game->renderer, 
			entry->texture, 
			&entry->src, 
			&dst);

		if(grabbedEntry && uimode == 1 && keyDown(SDL_SCANCODE_SPACE)) {
			for(int i = 0; i < numSourceTrays; ++i) {
				UiTray* tray = sourceTrays[i];
				if(!tray->visible) continue;
				float4 region = rXform(trayRegion(tray), sourceCam);
				if(rect_contains(region, mpos)) {
					int hover = trayHover(tray, mpos, sourceCam);
					if(mbtnJustUp(SDL_BUTTON_LEFT)) {
						editedPoints = true;
						int ret = trayAdd(tray, grabbedEntry, hover);
						if(ret == -1) {
							trayAdd(grabOrigin, grabbedEntry, grabbedIndex);
						}
						grabbedEntry = nullptr;
						break;
					}
				}
			}
		}

		if(grabbedEntry) {
			Xform lcam = camera;
			do {
				if(uimode == 0) {
					lcam = sourceCam;
					// TODO cache rpanel info
					float2 rpanel = 0;
					float titlew = 96;
					rpanel.x = 16 + 10 * 64 + (16 + titlew);
					rpanel.y = 32 + 64 + 16;
					float4 rpanel_r;
					rpanel_r.xy = rpanel;
					rpanel_r.zw = Game->windowSize - rpanel;
					if(!rect_contains(rpanel_r, mpos)) {
						break;;
					}
				}

				for(int i = 0; i < numTrays; ++i) {
					UiTray* tray = trays[i];
					if(!tray->visible) continue;
					if(uimode == 0) {
						if(tray->value != -1 && tray->value != grabbedEntry->b) {
							continue;
						}
					}
					float4 region = rXform(trayRegion(tray), lcam);
					if(rect_contains(region, mpos)) {
						int hover = trayHover(tray, mpos, lcam);
						if(mbtnJustUp(SDL_BUTTON_LEFT)) {
							madeChange = true;
							if(uimode == 1) {
								editedPoints = true;
							}
							int ret = trayAdd(tray, grabbedEntry, hover);
							if(ret == -1) {
								trayAdd(grabOrigin, grabbedEntry, grabbedIndex);
							}
							grabbedEntry = nullptr;
							break;
						}
					}
				}
			} while(0);
		}

		if(uimode == 0) {
			Xform lcam = camera;
			for(int i = 0; i < numTeams; ++i) {
				UiTray* tray = pteams[i].tray;
				if(!tray->visible) continue;
				float4 region = rXform(trayRegion(tray), lcam);
				if(rect_contains(region, mpos)) {
					int hover = trayHover(tray, mpos, lcam);
					if(mbtnJustUp(SDL_BUTTON_LEFT)) {
						madeChange = true;
						int ret = trayAdd(tray, grabbedEntry, hover);
						if(ret == -1) {
							trayAdd(grabOrigin, grabbedEntry, grabbedIndex);
						} else {
							if(i == getTeamTurn()) {
								draftTurn++;
							}
						}
						grabbedEntry = nullptr;
						break;
					}
				}
			}
		}

		if(grabbedEntry && mbtnUp(SDL_BUTTON_LEFT)) {
			if(uimode == 0) {
				for(int i = 0; i < numTrays; ++i) {
					UiTray* tray = trays[i];
					if(tray->value == grabbedEntry->b) {
						trayAdd(tray, grabbedEntry, -1);
						grabbedEntry = nullptr;
						break;
					}
				}
			}

			if(grabbedEntry) {
				trayAdd(grabOrigin, grabbedEntry, grabbedIndex);
			}
			grabbedEntry = nullptr;
		}
	}

	if(uimode == 1) {
		if(!grabbedEntry && mbtnJustDown(SDL_BUTTON_LEFT)) {
			if(keyDown(SDL_SCANCODE_SPACE)) {
				sourcePan = true;
				lastCamera = sourceCam.pos;
			} else {
				lastCamera = camera.pos;
				cameraPan = true;
			}
			cameraGrab = Game->input->mpos;
		}

		if(cameraPan) {
			camera.pos.x = cameraGrab.x - Game->input->mpos.x + lastCamera.x;
		}
		if(sourcePan) {
			sourceCam.pos.x = cameraGrab.x - Game->input->mpos.x + lastCamera.x;
		}
	}

	if(uimode == 2) {
		if(!grabbedEntry && mbtnJustDown(SDL_BUTTON_LEFT)) {
			lastCamera = camera.pos;
			cameraPan = true;
			cameraGrab = Game->input->mpos;
		}

		if(cameraPan) {
			camera.pos.y = cameraGrab.y - Game->input->mpos.y + lastCamera.y;
		}
	}

	if(uimode == 0 && madeChange) {
		saveDraft("draft.txt", "teams.txt");
	}

	if(mbtnUp(SDL_BUTTON_LEFT)) {
		cameraPan = false;
		sourcePan = false;
	}

	drawDrawbox();
}

void saveTrays(FILE* fp, UiTray** trays, int numTrays, const char* key)
{
	for(int i = 0; i < numTrays; ++i) {
		UiTray* tray = trays[i];
		for(int j = 0; j < tray->numSlots; ++j) {
			UiTrayEntry* entry = tray->slots[j];
			char buf[512];
			size_t len = SDL_snprintf(buf, 512, "'%s' %d %d %d '%s'\n", tray->title, entry->id+1, entry->a, entry->b, mons[entry->id].name);
			fwrite(buf, 1, len, fp);
		}
	}
}

void saveTeamTrays(FILE* fp, PlayerTeam* teams, int numTeams, const char* key)
{
	for(int i = 0; i < numTeams; ++i) {
		UiTray* tray = teams[i].tray;
		if(!tray) continue;
		for(int j = 0; j < tray->numSlots; ++j) {
			UiTrayEntry* entry = tray->slots[j];
			char buf[512];
			size_t len = SDL_snprintf(buf, 512, "'%s' %d %d %d '%s'\n", tray->title, entry->id+1, entry->a, entry->b, mons[entry->id].name);
			fwrite(buf, 1, len, fp);
		}
	}
}

void saveTeams(FILE* fp, PlayerTeam* teams, int numTeams)
{
	for(int i = 0; i < numTeams; ++i) {
		PlayerTeam* team = &teams[i];
		char buf[512];
		size_t len = SDL_snprintf(buf, 512, "'%s' %d %d\n", team->name, team->points, team->icon);
		fwrite(buf, 1, len, fp);
	}
}

int loadTeams(char* text, size_t len, PlayerTeam* teams, int maxTeams)
{
	// world's worst deserialization code :) 
	char* head = text;
	char* tstart, *tend;

	char name[64];
	char points[64];
	char icon[64];
	int mode = 0;
	int numLoaded = 0;
	while(head[0]) {
		if(head[0] == ' ') head++;

		if(mode == 0) {
			if(head[0] == '\'') {
				tstart = head;
				mode = 1;
			}
		} else if (mode == 1) {
			if(head[0] == '\'') {
				tend = head;
				memset(name, 0, SDL_arraysize(name));
				strncpy(name, tstart + 1, i32min(tend - (tstart + 1), 63));
				mode = 2;
			}
		} else if(mode == 2)  {
			if(head[0] >= '0' && head[0] <= '9') {
				tstart = head;
			}
			while(head[0] >= '0' && head[0] <= '9') {
				head++;
			}
			tend = head - 1;

			memset(points, 0, SDL_arraysize(points));
			strncpy(points, tstart, i32min(tend + 1 - (tstart), 63));


			mode = 3;
		} else if(mode == 3) {
			if(head[0] >= '0' && head[0] <= '9') {
				tstart = head;
			}
			while(head[0] >= '0' && head[0] <= '9') {
				head++;
			}
			tend = head - 1;

			memset(icon, 0, SDL_arraysize(icon));
			strncpy(icon, tstart, i32min(tend + 1 - (tstart), 63));
			if(numLoaded < maxTeams) {
				teams[numLoaded].name = SDL_strdup(name);
				teams[numLoaded].points = atoi(points);
				teams[numLoaded].icon = atoi(icon);
				numLoaded++;
			} else {
				break;
			}

			mode = 4;
		} 

		if(mode == 4) {
			if(head[0] == '\n') {
				mode = 0;
			}
		}

		head++;
	}
	return numLoaded;
}

int loadTrays(char* text, size_t len, dict32* trayData, int** arrays, int** etrAarrays, int** etrBarrays, int* sizes)
{
	char* head = text;
	char* tstart, *tend;

	char key[64];
	char num[64];
	char etr_a[64];
	char etr_b[64];
	int mode = 0;
	int numLoaded = 0;
	bool gotKey = false, gotNum = false, gotA = false, gotB = false;
	while(head[0]) {
		if(head[0] == ' ') head++;

		if(mode == 0) {
			if(head[0] == '\'') {
				tstart = head;
				mode = 1;
			}
		} else if (mode == 1) {
			if(head[0] == '\'') {
				tend = head;
				memset(key, 0, SDL_arraysize(key));
				strncpy(key, tstart + 1, i32min(tend - (tstart + 1), 63));
				gotKey = true;
				mode = 2;
			}
		} else if(mode == 2)  {
			if(head[0] >= '0' && head[0] <= '9') {
				tstart = head;
			}
			while(head[0] >= '0' && head[0] <= '9') {
				head++;
			}
			tend = head - 1;

			memset(num, 0, SDL_arraysize(num));
			strncpy(num, tstart, i32min(tend + 1 - (tstart), 63));
			gotNum = true;

			mode = 3;
		}  else if (mode == 3) {
			if(head[0] >= '0' && head[0] <= '9') {
				tstart = head;
			}
			while(head[0] >= '0' && head[0] <= '9') {
				head++;
			}
			tend = head - 1;

			memset(etr_a, 0, SDL_arraysize(etr_a));
			strncpy(etr_a, tstart, i32min(tend + 1 - (tstart), 63));
			gotA = true;
			mode = 4;
		}  else if (mode == 4) {
			if(head[0] >= '0' && head[0] <= '9') {
				tstart = head;
			}
			while(head[0] >= '0' && head[0] <= '9') {
				head++;
			}
			tend = head - 1;

			memset(etr_b, 0, SDL_arraysize(etr_b));
			strncpy(etr_b, tstart, i32min(tend + 1 - (tstart), 63));
			gotB = true;

		}

		if(head[0] == '\n') {
			mode = 0;

			if(!gotKey || !gotNum)  {
				head++;
				gotKey = false;
				gotNum = false;
				gotA = false;
				gotB = false;
				continue;
			}

			int n = atoi(num);
			if(n <= numMons && n > 0) {
				if(monWasLoaded[n-1]) {
					head++;
					continue;
				} else {
					if(countWasLoaded)
						monWasLoaded[n-1] = 1;
				}
			}

			uint32_t index = 0;
			int ret = dict32Get(trayData, key, &index);

			int a = 0;
			int b = 0;

			if(etrAarrays) {
				if(gotA) {
					a = atoi(etr_a);
				}
			}

			if(etrBarrays) {
				if(gotB) {
					b = atoi(etr_b);
				}
			}

			if(ret == DICT32_OK) {
				if(sizes[index] >= 1024) {
					head++;
					continue;
				}
				arrays[index][sizes[index]] = atoi(num);
				if(etrAarrays) etrAarrays[index][sizes[index]] = a;
				if(etrBarrays) etrBarrays[index][sizes[index]] = b;
				sizes[index]++;
				numLoaded++;
			} else {
				// tray not present in pre-allocated set? insert
				int ret2 = dict32Add(&trayData, key, loadedTrays);
				(void)ret2;
				index = loadedTrays++;
				arrays[index][sizes[index]] = atoi(num);
				if(etrAarrays) etrAarrays[index][sizes[index]] = a;
				if(etrBarrays) etrBarrays[index][sizes[index]] = b;
				sizes[index]++;
				numLoaded++;
			}


			gotKey = false;
			gotNum = false;
			gotA = false;
			gotB = false;
		}

		head++;
	}
	return numLoaded;
}

void testRender(GameState* state, GameContext* game)
{
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
	bool ret = SDL_Init(SDL_INIT_VIDEO);
	if(!ret) {
		SDL_Log("Error: could not initialize SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	GameSettings settings = {1536, 720, "webdraft", false, false};
	Game = gameCreate(settings);
	SDL_SetDefaultTextureScaleMode(Game->renderer, SDL_SCALEMODE_NEAREST);

	Game->surface = SDL_LoadPNG_IO(SDL_IOFromMem(graphicsPng, SDL_arraysize(graphicsPng)), true);//;SDL_LoadPNG("graphics.png");
	Game->texture = SDL_CreateTextureFromSurface(Game->renderer, Game->surface);
	SDL_SetTextureBlendMode(Game->texture, SDL_BLENDMODE_BLEND);

	GameState* testState = gamestateCreate("Test", 0, sizeof(GameState));
	gamestateSetProcs(testState, testStart, testUpdate, nullptr, testRender, nullptr);
	gameRegister(Game, testState);

	gameStart(Game, "Test");

	return SDL_APP_CONTINUE;
}

int frameIndex = 0;

SDL_AppResult SDL_AppIterate(void *appstate)
{
	gamePreUpdate(Game);
	gameUpdate(Game);
	gamePostUpdate(Game);
	frameIndex++;
	return SDL_APP_CONTINUE;
}



SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	needUpdate = 3;
	if(event->type == SDL_EVENT_KEY_DOWN) uiHandleKeyDown(*event);
	if(event->type == SDL_EVENT_TEXT_INPUT) uiHandleTextInput(*event);
	return gameHandleEvent(Game, event);
}

void saveDraft(const char* draftname, const char* teamsname)
{
	FILE* fp = fopen(draftname, "wb");
	saveTrays(fp, trays, numTrays, "tray");
	saveTrays(fp, sourceTrays, numSourceTrays, "source");
	saveTeamTrays(fp, pteams, numTeams, "team");
	fclose(fp);

	fp = fopen(teamsname, "wb");
	saveTeams(fp, pteams, numTeams);
	fclose(fp);
}

typedef struct StatTotals
{
	int stats[6];
} StatTotals;

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	(void)appstate;
	(void)result;

	int teamTotals[16] = {0};
	StatTotals teamStats[16] = {0};

	for(int i = 0; i < numTeams; ++i) {
		for(int j = 0; j < 10; ++j)	 {
			UiTrayEntry* entry = pteams[i].tray->slots[j];
			if(!entry) continue;

			for(int k = 0; k < 6; ++k) {
				teamTotals[i] += mons[entry->id].stats[k];
				teamStats[i].stats[k] += mons[entry->id].stats[k];
			}
		}
	}

	const char* statnames[] = {"HP  :", "ATK  :", "DEF  :", "SpAtk:", "SpDef:", "Speed:"};

	for(int i = 0; i < numTeams; ++i) {
		printf("%s: %d", pteams[i].name, teamTotals[i]);
		for(int k = 0; k < 6; ++k) {
			printf("\t%s: %d\n", statnames[k], teamStats[i].stats[k]);
		}
	}

	if(editedPoints) {
		SDL_Log("writing new point totals");
		FILE* fp = fopen("points.txt", "wb");
		saveTrays(fp, trays, numTrays, "tray");
		saveTrays(fp, sourceTrays, numSourceTrays, "source");
		fclose(fp);
	} else {
		SDL_Log("point assignments unchanged");
		SDL_Log("writing draft and teams");
		saveDraft("draft.txt", "teams.txt");
	}
}
