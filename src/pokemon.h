#pragma once
#include <SDL3/SDL.h>
#include "wb_gamemath.h"

typedef struct MonData MonData;
extern MonData* globalMonData;

extern SDL_Texture* monTexture;

static inline
SDL_FRect getMonRect(int mon)
{
	int row = monTexture->w / 64;
	int y = mon / row;
	int x = mon % row;
	return (SDL_FRect){x * 64, y * 64, 64, 64};
}


enum 
{
	Type_None=-1,
	Type_Normal,
	Type_Fighting,
	Type_Flying,
	Type_Poison,
	Type_Ground,
	Type_Rock,
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
	Type_Fairy,
	Num_Types
};

const char* Mon_TypeNames[] = {
	"Normal",
	"Fighting",
	"Flying",
	"Poison",
	"Ground",
	"Rock",
	"Bug",
	"Ghost",
	"Steel",
	"Fire",
	"Water",
	"Grass",
	"Electric",
	"Psychic",
	"Ice",
	"Dragon",
	"Dark",
	"Fairy",
	"???",
	"???",
	"???",
};

enum 
{
	Stat_HP,
	Stat_Atk,
	Stat_Def,
	Stat_SpAtk,
	Stat_SpDef,
	Stat_Speed,
	Num_Stats,
};

const char* Mon_StatNames[] = {
	"HP",
	"Attack",
	"Defense",
	"SpAtk",
	"SpDef",
	"Speed"
};

const char* Mon_StatHeaderTitles[] = {
	"HP:   ",
	"Atk:  ",
	"Def:  ",
	"SpAtk:",
	"SpDef:",
	"Speed:"
};


static inline 
int getHiddenPowerType(int* dv)
{
	return (((dv[Stat_Atk] & 3) << 2) | (dv[Stat_Def] & 3)) + 1;
}

static inline 
int getHiddenPowerBp(int* dv)
{
	int v = (dv[Stat_SpAtk] & 8) >> 3;
	int w = (dv[Stat_Speed] & 8) >> 2;
	int x = (dv[Stat_Def] & 8) >> 1;
	int y = (dv[Stat_Atk] & 8);
	int z = dv[Stat_SpAtk] & 3;
	int base = 5 * (v + w + x + y) + z;
	base /= 2;
	base += 31;
	return base;
}


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

#include "pokemon_moves.h"

static const 
int2 Mon_HighlightedMoves[] = {
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

typedef struct MonHasMove
{
	int id;
	int numMoves;
	uint8_t moves[SDL_arraysize(Mon_HighlightedMoves)];
} MonHasMove;

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
	uint8_t reserved;
	int number;
} MoveDef;

typedef struct MonDef
{
	char name[16];
	int preevo;
	int number;
	int abilities[2];
	uint8_t types[2]; 
	uint8_t stats[6];
	uint16_t moves[100];
} MonDef;

typedef struct MonAbility
{
	char name[16];
	char desc[32];
} MonAbility;

enum
{
	Table_Generic,
	Table_Mons,
	Table_Moves,
	Table_Points,
	Table_Players,
	Table_Draft,
	Table_Abilities
};

typedef struct TableHeader
{
	uint32_t magic, version, kind, count;
} TableHeader;

typedef struct BattleMon
{
	MonDef* def;
	int level;
	int stats[6];
	int boosts[6];
	int statxp[6];
	int dvs[6];
	int moves[4];
} BattleMon;

typedef struct MonName
{
	char name[32];
} MonName;

typedef struct MonRef
{
	uint16_t id;
	uint8_t cost;
	uint8_t flags;
	int reserved;
} MonRef;

typedef struct PlayerTeam
{
	MonName title, owner;
	MonName nicknames[20];
	MonRef mons[20];
	int draftOrder, numMons;
} PlayerTeam;

typedef struct PlayerDatabase
{
	PlayerTeam* players;
	int numPlayers, maxPlayers;
} PlayerDatabase;
PlayerDatabase* createPlayerDB(int maxTeams);
int playerdbSave(PlayerDatabase* playerdb, const char* filename);
int playerdbLoad(PlayerDatabase* playerdb, const char* filename);


typedef struct MonPointDatabase
{
	int numTiers, maxTiers;
	int* pointCosts;
	int* numMonsPerTier;
} MonPointDatabase;

MonPointDatabase* createPointDB(MonData* data, int maxTiers);
int pointdbSave(MonData* data, MonPointDatabase* pointdb, const char* filename);
int pointdbLoad(MonData* data, MonPointDatabase* pointdb, const char* filename);

//int pointdbExportCSV(MonData* data, MonPointDatabase* pointdb, const char* filename);
int pointdbExportCSV(MonData* data, MonPointDatabase* pointdb, SDL_IOStream* stream);
int pointdbImportCSV(MonData* data, MonPointDatabase* pointdb, const char* filename);

typedef struct DraftBoard
{
	int flags;
	int startingPoints;
	int turnIndex;
	int reserved0;

	int reserved1[28];
} DraftBoard;

int draftSave(MonData* data, DraftBoard* board, PlayerDatabase* playerdb, MonPointDatabase* pointdb, const char* filename);
int draftLoad(MonData* data, DraftBoard* board, PlayerDatabase* playerdb, MonPointDatabase* pointdb, const char* filename);

typedef struct MonData
{
	MonDef* mons;
	MoveDef* moves;
	int* evoTree;
	MonHasMove* hasMoves;
	MonDef* sortableMons;
	MonPointDatabase* pointdb;
	PlayerDatabase* playerdb;
	DraftBoard* draftboard;
	MonAbility* abilities;

	int numMons;
	int numMoves;
	int numAbilities;
} MonData;

static inline
void setupMonHasMoves(MonData* md, int numMons)
{
	// TODO make move searching/matching a lil more generic
	// TODO export moves in game-order w/ index defines
	md->hasMoves = calloc(numMons, sizeof(MonHasMove));
	for(int i = 0; i < numMons; ++i) {
		MonDef* mon = &md->mons[i];
		md->hasMoves[i].id = i;
		for(int j = 0; j < 100; ++j) {
			for(int k = 0; k < SDL_arraysize(Mon_HighlightedMoves); ++k) {
				if(mon->moves[j] == Mon_HighlightedMoves[k].x) {
					MonHasMove* hasmove = &md->hasMoves[i];
					bool skip = false;
					for(int l = 0; l < hasmove->numMoves; ++l) {
						if(Mon_HighlightedMoves[hasmove->moves[l]].y == Mon_HighlightedMoves[k].y) {
							skip = true;
							break;
						}
					}

					if(skip) continue;

					hasmove->moves[hasmove->numMoves++] = k;
				}
			}
		}
	}
}

typedef struct DamageResult
{
	int2 hprange;
	float2 percrange;
	int2 hitrange;
	int allRolls[40];
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


